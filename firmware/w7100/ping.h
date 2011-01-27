#include "types.h"

#define BUF_LEN 32
#define PING_REQUEST 8
#define PING_REPLY 0
#define CODE_ZERO 0

#define PING_PORT       1000

typedef struct pingmsg
{
  uint8  Type; 		// 0 - Ping Reply, 8 - Ping Request
  uint8  Code;		// Always 0
  int16  CheckSum;	// Check sum
  int16  ID;	            // Identification 
  int16  SeqNum; 	// Sequence Number
  int8	 Data[BUF_LEN];// Ping Data  : 1452 = IP RAW MTU - sizeof(Type+Code+CheckSum+ID+SeqNum)
} PINGMSGR;


uint8 ping(SOCKET s, uint8 *addr, uint8 count, uint16 timeout );
