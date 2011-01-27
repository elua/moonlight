#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "socket.h"
#include "dhcp_app.h"
#include "types.h"
#include "W7100.h"
#include "wiz.h"
#include "settings.h"
#include "vtmr.h"

RIP_MSG xdata MSG;
					
extern uint8 xdata Debug_Off;
extern uint8 xdata Enable_DHCP_Timer;
uint8 xdata txsize[WIZ_N_SOCKETS] = {1,2,8,1,1,1,1,1};
uint8 xdata rxsize[WIZ_N_SOCKETS] = {1,2,8,1,1,1,1,1};

uint8 xdata OLD_SIP[4];
uint8 xdata DHCPS_IP[4];
uint8 xdata tmp_DHCPS_IP[4];
uint8 xdata ServerIP[4], MyIP[4];
uint16 xdata S_port;				// DHCP Server Port Number
uint8 xdata Device_Name[14];

un_l2cval xdata lease_time;
volatile uint32 xdata my_time;
uint32 xdata next_time;

uint8 xdata retry_count = 0;
uint8 xdata DHCP_Timeout = 0;

uint32 xdata DHCP_XID;
uint8 xdata SameIPCnt;
uint8 xdata dhcp_state;

void send_DHCP_DISCOVER(SOCKET s)
{
	uint8 xdata addr[4];
	uint16 xdata k = 0;

  memset( &MSG, 0, sizeof( MSG ) );
	MSG.op = DHCP_BOOTREQUEST;
	MSG.htype = DHCP_HTYPE10MB;
	MSG.hlen = DHCP_HLENETHERNET;
	MSG.hops = DHCP_HOPS; // Client set to zero
	MSG.xid = DHCP_XID; //Transaction ID
	MSG.secs = DHCP_SECS; //Filled in by client, address acquisition or renewal process
	MSG.flags = DHCP_FLAGSBROADCAST; 
	memcpy( MSG.chaddr, settings_get()->mac, 6 );
	
	// MAGIC_COOKIE
	MSG.OPT[k++] = MAGIC0; MSG.OPT[k++] = MAGIC1;
	MSG.OPT[k++] = MAGIC2; MSG.OPT[k++] = MAGIC3;
	MSG.OPT[k++] = dhcpMessageType;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = DHCP_DISCOVER;
	MSG.OPT[k++] = dhcpClientIdentifier;
	MSG.OPT[k++] = 0x07;
	MSG.OPT[k++] = 0x01;
	memcpy( MSG.OPT + k, settings_get()->mac, 6 );
	k += 6;
	MSG.OPT[k++] = hostName;
	MSG.OPT[k++] = 14;
	memcpy( MSG.OPT + k, Device_Name, 14 );
	k += 14;
	MSG.OPT[k++] = dhcpParamRequest;
	MSG.OPT[k++] = 0x05;
	MSG.OPT[k++] = subnetMask;
	MSG.OPT[k++] = routersOnSubnet;
	MSG.OPT[k++] = dns;
	MSG.OPT[k++] = dhcpT1value;
	MSG.OPT[k++] = dhcpT2value;
	MSG.OPT[k++] = endOption;

	// DST IP : BroadCasting
	S_port = DHCP_SERVER_PORT;
	memset( addr, 0xFF, 4 );

#ifdef DHCP_DEBUG
	printf("send_DHCP_DISCOVER : \r\n");
#endif

	// send MSG to Broadcasting
	sendto(s, (uint8 *)(&MSG.op), RIP_MSG_SIZE, addr, S_port, VTMR_INF_TIMEOUT );
}

void send_DHCP_REQUEST(SOCKET s, uint8 REREQ)
{
	uint8 xdata addr[4];
	uint16 xdata k = 0;

	memset( &MSG, 0, sizeof( MSG ) );
	MSG.op = DHCP_BOOTREQUEST;
	MSG.htype = DHCP_HTYPE10MB;
	MSG.hlen = DHCP_HLENETHERNET;
	MSG.hops = DHCP_HOPS;
	MSG.xid = DHCP_XID;
	MSG.secs = DHCP_SECS;
	MSG.flags = DHCP_FLAGSBROADCAST;

	memcpy( MSG.chaddr, settings_get()->mac, 6 );

	// MAGIC_COOKIE
	MSG.OPT[k++] = MAGIC0;
	MSG.OPT[k++] = MAGIC1;
	MSG.OPT[k++] = MAGIC2;
	MSG.OPT[k++] = MAGIC3;

	// Option Request Param.
	MSG.OPT[k++] = dhcpMessageType;
	MSG.OPT[k++] = 0x01;
	MSG.OPT[k++] = DHCP_REQUEST;
	MSG.OPT[k++] = dhcpClientIdentifier;
	MSG.OPT[k++] = 0x07;
	MSG.OPT[k++] = 0x01;
	memcpy( MSG.OPT + k, settings_get()->mac, 6 );
	k += 6;
	MSG.OPT[k++] = dhcpRequestedIPaddr;
	MSG.OPT[k++] = 0x04;
	memcpy( MSG.OPT + k, REREQ != 0 ? MyIP : settings_get()->ip, 4 );
	k += 4;
	MSG.OPT[k++] = dhcpServerIdentifier;
	MSG.OPT[k++] = 0x04;
	memcpy( MSG.OPT + k, REREQ != 0 ? ServerIP : DHCPS_IP, 4 );
	k += 4;
	MSG.OPT[k++] = hostName;
	MSG.OPT[k++] = 14;
	memcpy( MSG.OPT + k, Device_Name, 14 );
	k += 14;
	
	MSG.OPT[k++] = dhcpParamRequest;
	MSG.OPT[k++] = 0x05;
	MSG.OPT[k++] = subnetMask;
	MSG.OPT[k++] = routersOnSubnet;

	MSG.OPT[k++] = dns;
	MSG.OPT[k++] = dhcpT1value;
	MSG.OPT[k++] = dhcpT2value;
	MSG.OPT[k++] = endOption;

	/* DST IP : BroadCasting*/
	S_port = DHCP_SERVER_PORT;
	if(REREQ != 0) 
		memcpy( addr, ServerIP, 4 );
	else
		memset( addr, 0xFF, 4 );
	
#ifdef DHCP_DEBUG
	printf("send_DHCP_REQUEST \r\n");
#endif	

		// send MSG to Broadcasting
	sendto(s, (uint8 *)(&MSG.op), RIP_MSG_SIZE, addr, S_port, VTMR_INF_TIMEOUT );
}


char parseDHCPMSG(SOCKET s, uint16 length)
{
	uint8 xdata ServerAddrIn[4];
	uint16 xdata  ServerPort;
	uint16 xdata len;
	uint8 xdata type, opt_len;
	uint8 xdata * p;
	uint8 xdata * e;			 

	len = 0;
	len = recvfrom(s, (uint8*)&MSG.op, length, ServerAddrIn, &ServerPort);
#ifdef _DHCP_DEBUG_										  
	printf("ServerPort: %d \r\n", (int)ServerPort);	
	printf("Message Type %.2bx \r\n ", MSG.op );
#endif

	if (ServerPort == DHCP_SERVER_PORT)
  {
    if( memcmp( MSG.chaddr, settings_get()->mac, 6 ) )
      return 0;
		
		//DHCP Server IP
		memcpy( DHCPS_IP, MSG.siaddr, sizeof( DHCPS_IP ) );
		memcpy( settings_get()->ip, MSG.yiaddr, 4 );
		memcpy( settings_get()->gw, MSG.giaddr, 4 );				
	}

	type = 0;
	p = (uint8*)(&MSG.op);
	p = p + POS_OPTION;
	e = p + (len - POS_OPTION);

	while ( p < e )
	{
 		switch ( *p++ )
		{
			case endOption :
				return type;
				
       		case padOption :
				opt_len = 0;
				break;

     		case dhcpMessageType :
				opt_len = *p++;
				type = *p;
#ifdef _DHCP_DEBUG_
	printf("dhcpMessageType%.2bu \r\n", dhcpMessageType);
#endif
				break;

       		case subnetMask :
			    opt_len = *p++;
				memcpy( settings_get()->mask, p, 4 );
				
#ifdef _DHCP_DEBUG_
	printf("subnnetMask %.2bu.%.2bu.%.2bu.%.2bu\r\n", 
	SubMask[0], SubMask[1],SubMask[2],SubMask[3]);
#endif
				
				break;

       		case routersOnSubnet :
				opt_len = *p++;
				memcpy( settings_get()->gw, p, 4 );
				
#ifdef _DHCP_DEBUG_
	printf("routersOnSubnet %.2bu.%.2bu.%.2bu.%.2bu\r\n", 
	Gateway[0], Gateway[1], Gateway[2], Gateway[3]);
#endif
				
				break;

	       	case dns :
				opt_len = *p++;
				memcpy( settings_get()->dns, p, 4 );
				
#ifdef _DHCP_DEBUG
	printf("dns %.2bu.%.2bu.%.2bu.%.2bu\r\n", 
	DNS[0], DNS[1], DNS[2], DNS[3]);
#endif

				break;
		case dhcpIPaddrLeaseTime :  // unit sec.
			//p++;
			opt_len = *p++;
			memcpy( lease_time.cVal, p, sizeof( lease_time.cVal ) );
			
#ifdef _DHCP_DEBUG
	printf("lease time = %lu\r\n",lease_time.lVal);
#endif
		
			break;

		case dhcpServerIdentifier :
			opt_len = *p++;
			memcpy( DHCPS_IP, p, sizeof( DHCPS_IP ) );
		
#ifdef _DHCP_DEBUG
printf("dhcpServerIdentifier %.2bu.%.2bu.%.2bu.%.2bu\r\n", 
	DHCPS_IP[0], DHCPS_IP[1], DHCPS_IP[2], DHCPS_IP[3]);
#endif
		
			break;
				
		default :
			opt_len = *p++;
			
#ifdef _DHCP_DEBUG_
	printf("opt_len %.2bu \r\n", opt_len);
#endif

			break;
		}
		p += opt_len;
	}
    return	type;
}

void check_retry(void)
{
	if (retry_count < MAX_DHCP_RETRY) {
		if (next_time < my_time) {
			my_time = 0;
			next_time = my_time + DHCP_WAIT_TIME;
			retry_count++;
			switch ( dhcp_state ) {
				case STATE_DHCP_DISCOVER :
					send_DHCP_DISCOVER(SOCK_DHCP);
					break;
		
				case STATE_DHCP_REQUEST :
					send_DHCP_REQUEST(SOCK_DHCP, 0);
					break;

				case STATE_DHCP_REREQUEST :
					for(;socket(SOCK_DHCP, PROTOCOL_UDP, DHCP_CLIENT_PORT, 0x0)!=1;);
					DHCP_XID ++;
					send_DHCP_REQUEST(SOCK_DHCP, 1);
					break;
			}
		}
	} else {
		printf("Time out.\r\n");
		DHCP_Timeout = 1;
		
		my_time = 0;
		next_time = my_time + DHCP_WAIT_TIME;
		retry_count = 0;
		send_DHCP_DISCOVER(SOCK_DHCP);
		dhcp_state = STATE_DHCP_DISCOVER;
		
	}
}

void check_dhcp(void) 
{
	uint16 xdata len;
	uint8 xdata type ;

	len = 0;
	len = wiz_getSn_RX_RSR(SOCK_DHCP);

	if (len > 0) {
	
	 type = parseDHCPMSG(SOCK_DHCP, len);
#ifdef _DHCP_DEBUG
printf("Type = %d (%d) \r\n",(int)type, (int)DHCP_OFFER);
printf("dhcp_state : %d\r\n",(int)dhcp_state );
#endif

	}
	
	switch ( dhcp_state ) {
		case STATE_DHCP_DISCOVER : 
			if (type == DHCP_OFFER) {

#ifdef DHCP_DEBUG
	printf("Receive DHCP_Offer\r\n");
#endif
				send_DHCP_REQUEST(SOCK_DHCP, 0);
				dhcp_state = STATE_DHCP_REQUEST;
				printf("state_dhcp_request %d",(int)dhcp_state);
				tmp_DHCPS_IP[3] = DHCPS_IP[3];
				tmp_DHCPS_IP[2] = DHCPS_IP[2];
				tmp_DHCPS_IP[1] = DHCPS_IP[1];
				tmp_DHCPS_IP[0] = DHCPS_IP[0];

#ifdef DHCP_DEBUG
	printf("state : STATE_DHCP_REQUEST\r\n");
#endif
				
			} else check_retry(); // resend DISCOVER message
		break;

		case STATE_DHCP_REQUEST :
			if (type == DHCP_ACK) {
#ifdef DHCP_DEBUG
	printf(" Receive ACK\r\n");
#endif
				
					Init_Net();

          memcpy( ServerIP, DHCPS_IP, 4 );
          memcpy( MyIP, settings_get()->ip, 4 );

					dhcp_state = STATE_DHCP_LEASED;
					my_time = 0;
					next_time = my_time + DHCP_WAIT_TIME;
					retry_count = 0;
#ifdef DHCP_DEBUG
	printf("state : STATE_DHCP_LEASED\r\n");
#endif
				
				
			} else if (type == DHCP_NAK) {
			 //avoid to NAK caused from DHCP Server which didn't offer to Client
			if( !(tmp_DHCPS_IP[3] == DHCPS_IP[3] &&
				tmp_DHCPS_IP[2] == DHCPS_IP[2] &&
				tmp_DHCPS_IP[1] == DHCPS_IP[1]&&
				tmp_DHCPS_IP[0] == DHCPS_IP[0]) )
			 break;
		

#ifdef DHCP_DEBUG
	printf(" Receive NACK");
#endif
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				retry_count = 0;
				dhcp_state = STATE_DHCP_DISCOVER;
			} else check_retry();	// resend REQUEST message
		break;

		case STATE_DHCP_LEASED :
			if ((lease_time.lVal != 0xffffffff) && ((lease_time.lVal/2) < my_time)) {
#ifdef DHCP_DEBUG
	printf(">>>> STATE_DHCP_LEASED\r\n");
#endif
				
				type = 0;
        memcpy( OLD_SIP, settings_get()->ip, 4 );
				
				for(;socket(SOCK_DHCP, PROTOCOL_UDP, DHCP_CLIENT_PORT, 0x0)!=1;);
				DHCP_XID++;
				send_DHCP_REQUEST(SOCK_DHCP, 1);
				
				dhcp_state = STATE_DHCP_REREQUEST;
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
			}
			break;

		case STATE_DHCP_REREQUEST :
#ifdef DHCP_DEBUG
	printf(" < STATE_DHCP_REREQUEST >\r\n");
	printf("type : %.2bu\r\n",type);
#endif
			if (type == DHCP_ACK) {
				retry_count = 0;
				
				if (memcmp(OLD_SIP, settings_get()->ip, 4 )) {
					my_time = 0;
					Init_Net(); // re-setting
				} 
				
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				dhcp_state = STATE_DHCP_LEASED;
			} 
			else if (type == DHCP_NAK) {
				my_time = 0;
				next_time = my_time + DHCP_WAIT_TIME;
				retry_count = 0;
				dhcp_state = STATE_DHCP_DISCOVER;
				printf("state : STATE_DHCP_DISCOVER\r\n");
			} 
			else {
#ifdef DHCP_DEBUG
	printf(" Check Retry \r\n");
#endif
				check_retry();
			}
			
		break;

		case STATE_DHCP_RELEASE :
		break;

		default :
		break;
	}
}

void Set_Device_Name(void)
{
  memset( Device_Name, 0, 14 );
  strcpy( Device_Name, "moonlight" );
}

char DHCP_SetIP()
{
	Set_Device_Name();

	DHCP_XID = 0x12345665;

	for(;socket(SOCK_DHCP, PROTOCOL_UDP, DHCP_CLIENT_PORT, 0x0)!=1;);
	
	send_DHCP_DISCOVER(SOCK_DHCP);
	dhcp_state = STATE_DHCP_DISCOVER;
	
	SameIPCnt = 0;
	my_time = 0;
	next_time = my_time + DHCP_WAIT_TIME;
	retry_count = 0;
	Enable_DHCP_Timer = 1;
	
	while (dhcp_state != STATE_DHCP_LEASED) {
		check_dhcp();
		if (SameIPCnt >= 2) {
#ifdef DHCP_DEBUG
		printf("Fail:SameIP\r\n");
#endif
			return(0);
		}
		if (DHCP_Timeout == 1) return(0);
	}
	
	return 1;
}

void Init_Net_Default(void)
{
  u8 i;

	wiz_write8(MR,MR_RST);

	//MAC
  for( i = 0; i < 6; i ++ )
    wiz_write8( ( u16 )SHAR0 + i, settings_get()->mac[ i ] );

	//Gateway
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )GAR0 + i, 0 );

  // Subnet mask
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )SUBR0 + i, 0 );

  // Ip
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )SIPR0 + i, 0 );

	//Retransmission Time-out Register (delay 600ms)
	wiz_write8(RTR,(uint8)((6000 & 0xff00) >> 8)); // 600ms
	wiz_write8((RTR + 1),(uint8)(6000 & 0x00ff));

	//Retry Count Register
	wiz_write8(RCR,3);

	wiz_set_memsize(txsize,rxsize);
}


void Init_Net(void)
{  
  u8 i;
  									 
	wiz_write8(MR,MR_RST);

	//MAC
  for( i = 0; i < 6; i ++ )
    wiz_write8( ( u16 )SHAR0 + i, settings_get()->mac[ i ] );

	//Gateway
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )GAR0 + i, settings_get()->gw[ i ] );

	//Subnet Mask
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )SUBR0 + i, settings_get()->mask[ i ] );

	//Local IP address
  for( i = 0; i < 4; i ++ )
    wiz_write8( ( u16 )SIPR0 + i, settings_get()->ip[ i ] );

	//Retransmission Time-out Register (delay 600ms)
	wiz_write8(RTR,(uint8)((20000 & 0xff00) >> 8));	// RTR : 2s
	wiz_write8((RTR + 1),(uint8)(20000 & 0x00ff));

	//Retry Count Register
	wiz_write8(RCR,3);

	wiz_set_memsize(txsize,rxsize);

}
