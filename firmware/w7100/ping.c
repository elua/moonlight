#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "types.h"
#include "ping.h"
#include "sockutil.h"
#include "delay.h" 
#include "socket.h"
#include "vtmr.h"
#include "wiz.h"
#include "w7100.h"
#include "lcd.h"

static PINGMSGR PingRequest;	 // Variable for Ping Request
static uint16 RandomID = 0x1234; 
static uint16 RandomSeqNum = 0x4321;
static const uint16 ping_port = 3000;
static PINGMSGR pingmsg;
char gstr[ 41 ];
                               
static uint8 ping_request(SOCKET s, uint8 *addr, uint16 timeout ){
  uint16 xdata i;

	/* make header of the ping-request  */
	PingRequest.Type = PING_REQUEST;                      // Ping-Request 
	PingRequest.Code = CODE_ZERO;	                   // Always '0'
	PingRequest.ID = htons(RandomID++);	       // set ping-request's ID to random integer value
	PingRequest.SeqNum = htons(RandomSeqNum++);// set ping-request's sequence number to ramdom integer value      
	//size = 32;                                 // set Data size

	/* Fill in Data[]  as size of BIF_LEN (Default = 32)*/
  	for(i = 0 ; i < BUF_LEN; i++){	                                
		PingRequest.Data[i] = (i) % 8;		  //'0'~'8' number into ping-request's data 	
	}
	 /* Do checksum of Ping Request */
	PingRequest.CheckSum = 0;		               // value of checksum before calucating checksum of ping-request packet
	PingRequest.CheckSum = htons(checksum((uint8*)&PingRequest,sizeof(PingRequest)));  // Calculate checksum
	
     /* sendto ping_request to destination */
	if(sendto(s,(uint8 *)&PingRequest,sizeof(PingRequest),addr,PING_PORT,timeout)==0)
    return 0;
	return 1;
} // ping request

static uint8 ping_reply(SOCKET s, uint8 *addr, uint16 rlen){
	 
	 uint16 tmp_checksum;	
	 uint16 len;		
   u8 recaddr[ 4 ];
   u16 port;

  /* receive data from a destination */
   	len = recvfrom(s, (uint8*)&pingmsg,rlen,recaddr,&port);  

  	/* check the Type, If TYPE is 0, the received msg is the ping reply msg.*/ 
  	if(pingmsg.Type == PING_REPLY) {
  			/* check Checksum of Ping Reply */
  			tmp_checksum = pingmsg.CheckSum;
  			pingmsg.CheckSum = 0;
  			pingmsg.CheckSum = checksum((uint8*)&pingmsg,len);        	
        if( tmp_checksum == pingmsg.CheckSum && !memcmp( recaddr, addr, 4 ) && pingmsg.ID == PingRequest.ID && pingmsg.SeqNum == PingRequest.SeqNum )
  			  return 1;
  	}
  
  	return 0;
}// ping_reply

 uint8 ping(SOCKET s, uint8 *addr, uint8 count, uint16 timeout )
{
	uint16 rlen;
  uint16 replies = 0;
  uint8 i;
	 	 
	for( i = 0; i < count; i ++ )
  {
    // Create socket
		wiz_write8(Sn_PROTO(s), IPPROTO_ICMP);              // set ICMP Protocol
    socket( s, PROTOCOL_IP_RAW, ping_port, 0 );
	  while (wiz_getSn_SR(s)!=SOCK_IPRAW ); 

    // Send request / read reply with timeout
    if( ping_request(s, addr, timeout) )
    {
      vtmr_set_timeout( VTMR_NET, timeout );
      vtmr_enable( VTMR_NET );
      while( 1 )
      {
			  if ( (rlen = wiz_getSn_RX_RSR(s) ) > 0)
        {
  				if( ping_reply(s, addr, rlen ) )
            replies ++;
          break;					   
			  }
        if( vtmr_is_expired( VTMR_NET ) )
          break;
	    }
      vtmr_disable( VTMR_NET );
    }
    close(s);
  }
  return replies;
}














