/*
********************************************************************************
File Include Section
********************************************************************************
*/
#include <stdio.h> 
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "socket.h"
#include "dns.h"
#include "socket.h"
#include "delay.h"
#include "wiznet.h"
#include "vtmr.h"
#include "wiz.h"
#include "settings.h"
#include "utils.h"

#define	MAX_BUF_SIZE	1024

/*
********************************************************************************
Local Variable Declaration Section
********************************************************************************
*/
uint8 xdata dns_buf[MAX_BUF_SIZE];
uint16   MSG_ID = 0x1122;

/*
********************************************************************************
Function Implementation Part
********************************************************************************
*/
uint16 get16(uint8 * s)
{
	uint16 i;
	i = *s++ << 8;
	i = i + *s;
	return i;
}
/*
********************************************************************************
*              CONVERT A DOMAIN NAME TO THE HUMAN-READABLE FORM
*
* Description : This function converts a compressed domain name to the human-readable form
* Arguments   : msg        - is a pointer to the reply message
*               compressed - is a pointer to the domain name in reply message.
*               buf        - is a pointer to the buffer for the human-readable form name.
*               len        - is the MAX. size of buffer.
* Returns     : the length of compressed message
* Note        : 
********************************************************************************
*/
int parse_name(uint8 * msg, uint8 * compressed, char * buf, uint16 len)
{
	uint16 xdata slen;			// Length of current segment
	uint8 xdata * cp;
	int xdata clen = 0;		// Total length of compressed name
	int xdata indirect = 0;	// Set if indirection encountered
	int xdata nseg = 0;		// Total number of segments in name

	cp = compressed;

	for (;;)
	{
		slen = *cp++;	// Length of this segment

		if (!indirect) clen++;

		if ((slen & 0xc0) == 0xc0)
		{
			if(!indirect) clen++;
			indirect = 1;
			// Follow indirection
			cp = &msg[((slen & 0x3f)<<8) + *cp];
			slen = *cp++;
		}

		if (slen == 0) break;	/* zero length == all done */

		len -= slen + 1;

		if (len < 0) return -1;

		if (!indirect) clen += slen;

		while (slen-- != 0) *buf++ = (char)*cp++;
		*buf++ = '.';
		nseg++;
	}

	if (nseg == 0)
	{
		/* Root name; represent as single dot */
		*buf++ = '.';
		len--;
	}

	*buf++ = '\0';
	len--;

	return clen;	/* Length of compressed message */
}



/*
********************************************************************************
*              PARSE QUESTION SECTION
*
* Description : This function parses the qeustion record of the reply message.
* Arguments   : msg - is a pointer to the reply message
*               cp  - is a pointer to the qeustion record.
* Returns     : a pointer the to next record.
* Note        : 
********************************************************************************
*/
uint8 * dns_question(uint8 * msg, uint8 * cp)
{
	int xdata len;
	char xdata name[MAX_BUF_SIZE];
	len = parse_name(msg, cp, name, MAX_BUF_SIZE);

	if (len == -1) return 0;

	cp += len;
	cp += 2;		/* type */
	cp += 2;		/* class */

	return cp;
}

/*
********************************************************************************
*              PARSE ANSER SECTION
*
* Description : This function parses the answer record of the reply message.
* Arguments   : msg - is a pointer to the reply message
*               cp  - is a pointer to the answer record.
*               pip - write the host IP here
* Returns     : a pointer the to next record.
* Note        : 
********************************************************************************
*/
uint8 * dns_answer(uint8 * msg, uint8 * cp, uint8* pip)
{
	int xdata len, type;
	char xdata name[MAX_BUF_SIZE];

	len = parse_name(msg, cp, name, MAX_BUF_SIZE);

	if (len == -1) return 0;

	cp += len;
	type = get16(cp);
	cp += 2;		/* type */
	cp += 2;		/* class */
	cp += 4;		/* ttl */
	cp += 2;		/* len */

	switch (type)
	{
	case TYPE_A:	/* Just read the address directly into the structure */
		pip[0] = *cp++;
		pip[1] = *cp++;
		pip[2] = *cp++;
		pip[3] = *cp++;
		break;
	case TYPE_CNAME:
	case TYPE_MB:
	case TYPE_MG:
	case TYPE_MR:
	case TYPE_NS:
	case TYPE_PTR:
		/* These types all consist of a single domain name */
		/* convert it to ascii format */
		len = parse_name(msg, cp, name, MAX_BUF_SIZE);
		if(len == -1) return 0;

		cp += len;
		break;
	case TYPE_HINFO:
		len = *cp++;
		cp += len;

		len = *cp++;
		cp += len;
		break;
	case TYPE_MX:
		cp += 2;
		/* Get domain name of exchanger */
		len = parse_name(msg, cp, name, MAX_BUF_SIZE);
		if(len == -1) return 0;

		cp += len;
		break;
	case TYPE_SOA:
		/* Get domain name of name server */
		len = parse_name(msg, cp, name, MAX_BUF_SIZE);
		if(len == -1) return 0;

		cp += len;

		/* Get domain name of responsible person */
		len = parse_name(msg, cp, name, MAX_BUF_SIZE);
		if(len == -1) return 0;

		cp += len;

		cp += 4;
		cp += 4;
		cp += 4;
		cp += 4;
		cp += 4;
		break;
	case TYPE_TXT:
		/* Just stash */
		break;
	default:
		/* Ignore */
		break;
	}

	return cp;
}

/*
********************************************************************************
*              PARSE THE DNS REPLY
*
* Description : This function parses the reply message from DNS server.
* Arguments   : dhdr - is a pointer to the header for DNS message
*               buf  - is a pointer to the reply message.
*               len  - is the size of reply message.
*               pip - write the host IP here
* Returns     : None
* Note        : 
********************************************************************************
*/
uint8 parseMSG(struct dhdr * dhdr, uint8 * buf, uint8* pip)
{
	uint16 xdata tmp;
	uint16 xdata i;
	uint8 xdata * msg;
	uint8 xdata * cp;

	msg = buf;
	memset(dhdr, 0, sizeof(*dhdr));

	dhdr->id = get16(&msg[0]);
	tmp = get16(&msg[2]);
	if (tmp & 0x8000) dhdr->qr = 1;

	dhdr->opcode = (tmp >> 11) & 0xf;

	if (tmp & 0x0400) dhdr->aa = 1;
	if (tmp & 0x0200) dhdr->tc = 1;
	if (tmp & 0x0100) dhdr->rd = 1;
	if (tmp & 0x0080) dhdr->ra = 1;

	dhdr->rcode = tmp & 0xf;
	dhdr->qdcount = get16(&msg[4]);
	dhdr->ancount = get16(&msg[6]);
	dhdr->nscount = get16(&msg[8]);
	dhdr->arcount = get16(&msg[10]);

	/* Now parse the variable length sections */
	cp = &msg[12];

	/* Question section */
	for (i = 0; i < dhdr->qdcount; i++)
	{
		cp = dns_question(msg, cp);
	}

	/* Answer section */
	for (i = 0; i < dhdr->ancount; i++)
	{
		cp = dns_answer(msg, cp, pip);
	}

	/* Name server (authority) section */
	for (i = 0; i < dhdr->nscount; i++)
	{
		;
	}

	/* Additional section */
	for (i = 0; i < dhdr->arcount; i++)
	{
		;
	}
	if(dhdr -> rcode == 0) return 1;	//Means No Error
	else return 0;

}

/*
********************************************************************************
*              PUT NETWORK BYTE ORDERED INT.
*
* Description : This function copies uint16 to the network buffer with network byte order.
* Arguments   : s - is a pointer to the network buffer.
*               i - is a unsigned integer.
* Returns     : a pointer to the buffer.
* Note        : Internal Function
********************************************************************************
*/
uint8 * put16(uint8 * s, uint16 i)
{
	*s++ = i >> 8;
	*s++ = i;
	return s;
}

/*
********************************************************************************
*              MAKE DNS QUERY MESSAGE
*
* Description : This function makes DNS query message.
* Arguments   : op   - Recursion desired
*               name - is a pointer to the domain name.
*               buf  - is a pointer to the buffer for DNS message.
*               len  - is the MAX. size of buffer.
* Returns     : the pointer to the DNS message.
* Note        : 
********************************************************************************
*/
int dns_makequery(uint16 op, uint8 * name, uint8 * buf, uint16 len)
{
	uint8 xdata *cp;
	char xdata *cp1, *dname;
	char xdata sname[MAX_BUF_SIZE];
//	char xdata *dname;
	uint16 xdata p, dlen;
//	uint16 xdata dlen;

	cp = buf;

	MSG_ID++;
	cp = put16(cp, MSG_ID);
	p  = (op << 11) | 0x0100;			/* Recursion desired */
	cp = put16(cp, p);
	cp = put16(cp, 1);
	cp = put16(cp, 0);
	cp = put16(cp, 0);
	cp = put16(cp, 0);

	strcpy(sname, name);
	dname = sname;
	dlen = strlen(dname);
	for (;;)
	{
		/* Look for next dot */
		cp1 = strchr(dname, '.');

		if (cp1 != NULL) len = cp1 - dname;	/* More to come */
		else len = dlen;			/* Last component */

		*cp++ = len;				/* Write length of component */
		if (len == 0) break;

		/* Copy component up to (but not including) dot */
		strncpy((char *)cp, dname, len);
		cp += len;
		if (cp1 == NULL)
		{
			*cp++ = 0;			/* Last one; write null and finish */
			break;
		}
		dname += len+1;
		dlen -= len+1;
	}

	cp = put16(cp, 0x0001);				/* type */
	cp = put16(cp, 0x0001);				/* class */

	return ((int)((uint16)(cp) - (uint16)(buf)));
}

/*
********************************************************************************
*              MAKE DNS QUERY AND PARSE THE REPLY
*
* Description : This function makes DNS query message and parses the reply from DNS server.
* Arguments   : name - is a pointer to the domain name.
* Returns     : if succeeds : 1, fails : 0
* Note        : 
********************************************************************************
*/
uint8 dns_query(uint8 s, uint8* pip, const char * name, u16 timeout)
{
	struct dhdr xdata dhp;
	uint8 xdata ip[4];
	uint16 xdata len = 0, port;
	uint16 xdata cnt = 0;
	u8 execute_request = 1;

  // First check if the name isn't an IP itself
  if( utils_string_to_ip( name, pip ) )
    return 1;

	for(;socket(s, PROTOCOL_UDP, 5000, 0)!=1;);
	len = dns_makequery(0, (uint8*)name, dns_buf, MAX_BUF_SIZE);
	if( sendto(s, dns_buf, len, settings_get()->dns, IPPORT_DOMAIN, timeout ) != len )
    return 0;
  // Wait for data
  vtmr_set_timeout( VTMR_NET, timeout );
  vtmr_enable( VTMR_NET );
  while( !vtmr_is_expired( VTMR_NET ) )
    if( wiz_getSn_RX_RSR( s ) > 0 )
      break;
  vtmr_disable( VTMR_NET );
  if( wiz_getSn_RX_RSR( s ) == 0 )
    execute_request = 0;      
	if( execute_request )
	{
    len = wiz_getSn_RX_RSR( s );
    if (len > MAX_BUF_SIZE) 
      len = MAX_BUF_SIZE;
    len = recvfrom(s, dns_buf, len, ip, &port);
  }
  else
    len = 0;
  close( s );
  return len == 0 ? 0 : parseMSG(&dhp, dns_buf, pip);	// Convert to local format
}
