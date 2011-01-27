#ifndef __DHCP_H
#define __DHCP_H

#include "types.h"

// DHCP state machine.
#define	STATE_DHCP_DISCOVER		1
#define	STATE_DHCP_REQUEST		2
#define	STATE_DHCP_LEASED		3
#define	STATE_DHCP_REREQUEST	4
#define	STATE_DHCP_RELEASE		5

#define	MAX_DHCP_RETRY		10
#define	DHCP_WAIT_TIME		5

/* UDP port numbers for DHCP */
#define	DHCP_SERVER_PORT	67	/* from server to client */
#define DHCP_CLIENT_PORT	68	/* from client to server */

/* DHCP message OP code */
#define DHCP_BOOTREQUEST	1
#define DHCP_BOOTREPLY		2

/* DHCP message type */
#define	DHCP_DISCOVER		1
#define DHCP_OFFER			2
#define	DHCP_REQUEST		3
#define	DHCP_DECLINE		4
#define	DHCP_ACK  			5
#define DHCP_NAK  			6
#define	DHCP_RELEASE		7
#define DHCP_INFORM  		8

/* DHCP RETRANSMISSION TIMEOUT (microseconds) */
#define DHCP_INITIAL_RTO    ( 4*1000000)
#define DHCP_MAX_RTO        (64*1000000)

#define DHCP_HTYPE10MB		1
#define DHCP_HTYPE100MB		2

#define DHCP_HLENETHERNET	6
#define DHCP_HOPS			0
//#define DHCP_XID 			0x12345678  // Client Unique ID
#define DHCP_SECS			0
#define DHCP_FLAGSBROADCAST	0x8000
#define MAGIC_COOKIE		0x63825363
#define DEFAULT_LEASETIME	0xffffffff	/* infinite lease time */

/* DHCP option and value (cf. RFC1533) */
typedef enum _OPTION
{
    padOption = 0,
    subnetMask =1,
    timerOffset =2,
    routersOnSubnet=3,
    timeServer=4,
    nameServer=5,
    dns=6,
    logServer=7,
    cookieServer=8,
    lprServer=9,
    impressServer=10,
    resourceLocationServer=11,
    hostName=12,
    bootFileSize=13,
    meritDumpFile=14,
    domainName=15,
    swapServer=16,
    rootPath=17,
    extentionsPath=18,
    IPforwarding=19,
    nonLocalSourceRouting=20,
    policyFilter=21,
    maxDgramReasmSize=22,
    defaultIPTTL=23,
    pathMTUagingTimeout=24,
    pathMTUplateauTable=25,
    ifMTU=26,
    allSubnetsLocal=27,
    broadcastAddr=28,
    performMaskDiscovery=29,
    maskSupplier=30,
    performRouterDiscovery=31,
    routerSolicitationAddr=32,
    staticRoute=33,
    trailerEncapsulation=34,
    arpCacheTimeout=35,
    ethernetEncapsulation=36,
    tcpDefaultTTL=37,
    tcpKeepaliveInterval=38,
    tcpKeepaliveGarbage=39,
    nisDomainName=40,
    nisServers=41,
    ntpServers=42,
    vendorSpecificInfo=43,
    netBIOSnameServer=44,
    netBIOSdgramDistServer=45,
    netBIOSnodeType=46,
    netBIOSscope=47,
    xFontServer=48,
    xDisplayManager=49,
    dhcpRequestedIPaddr=50,
    dhcpIPaddrLeaseTime=51,
    dhcpOptionOverload=52,
    dhcpMessageType=53,
    dhcpServerIdentifier=54,
    dhcpParamRequest=55,
    dhcpMsg=56,
    dhcpMaxMsgSize=57,
    dhcpT1value=58,
    dhcpT2value=59,
    dhcpClassIdentifier=60,
    dhcpClientIdentifier=61,
    endOption = 255
}OPTION;

#define OPT_SIZE		312	//DHCP total size : 544
#define RIP_MSG_SIZE	(240 + OPT_SIZE)
#define POS_OPTION		(240)

typedef struct _RIP_MSG{
	uint8  op;  //Message op code / message type. 1 = BOOTREQUEST, 2 = BOOTREPLY
	uint8  htype;  //Hardware address type (e.g., '1' = 10Mb Ethernet)
	uint8  hlen; //Hardware address length (e.g. '6' for 10Mb Ethernet)
	uint8  hops;	//Client sets to zero, optionally used by relay agents when booting via a relay agent.		
	uint32  xid; //Transaction ID.   
	uint16  secs; //Seconds passed since client began the request process
	uint16  flags; 		
	uint8  ciaddr[4]; //Client IP address
	uint8  yiaddr[4]; //Your(Client) IP address
	uint8  siaddr[4]; //Server IP address
	uint8  giaddr[4]; //Relay agent IP address		
	uint8  chaddr[16]; // Client hardware address		
	uint8  sname[64];	//Optional server host name	
	uint8  file[128];		//Boot file name
	uint8  OPT[OPT_SIZE]; //Optional parameters
}RIP_MSG;

// DEFINE DHCP MACGIC COOKIE
#define MAGIC0	 0x63
#define MAGIC1	 0x82
#define MAGIC2	 0x53
#define MAGIC3	 0x63

void send_DHCP_DISCOVER(SOCKET s);
char parseDHCPMSG(SOCKET s,uint16 length);
char DHCP_SetIP();
void check_dhcp(void);
void send_DHCP_REQUEST(SOCKET s, uint8 REREQ);
void set_network(void);

#define SOCK_DHCP			0

void Init_Net_Default(void);
void Init_Net(void);
void Init_Timer0(void);

#endif
