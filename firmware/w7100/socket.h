/* socket.h */

/* W7100 socket driver
 */

/* This module provides an abstract interface to the eight logical sockets
 * supported by the TCP/IP hardware.
 */

/* To do:
 * - Clean up the function names in this module.
 */

/* History:
 *  2009/10/01 DT   Add constants, types and functions so that users of this
 *                  module don't need to depend on the underlying wiz module.
 *  2009/09/24 DT   Clean up.
 *  2009/09/20 DT   Add socket_init().
 *  2009/09/07 DT   Start.
 */

/*****************************************************************************
 * Public constants, types and data
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "types.h"

typedef enum {
  SOCKET_ERROR,
  SOCKET_CLOSED,
  SOCKET_INITIALIZED,
  SOCKET_ESTABLISHED,
  SOCKET_CLOSE_WAIT,
  SOCKET_UDP,
} socket_state_t;
  
typedef enum {
  PROTOCOL_UDP,
  PROTOCOL_TCP,
  PROTOCOL_IP_RAW,
  PROTOCOL_MAC_RAW,
  PROTOCOL_PPPOE,
} socket_protocol_t;

/*****************************************************************************
 * Public Functions
 */

/*---------------------------------------------------------------------------*/
/* Initialize the network hardware
 */

 void socket_init (void);
/*---------------------------------------------------------------------------*/
/* Opens a socket(TCP or UDP or IP_RAW mode)
 */

 uint8 socket (SOCKET s,
                     socket_protocol_t protocol,
                     uint16 port,
                     uint8 flag);
/*---------------------------------------------------------------------------*/
/* Get the current state of the socket
 */

 socket_state_t socket_state (SOCKET s);
/*---------------------------------------------------------------------------*/
/* Close socket
 */

 void close (SOCKET s);
/*---------------------------------------------------------------------------*/
/* Establish TCP connection (Active connection)
 */

 uint8 connect (SOCKET s, uint8 * addr, uint16 port, uint16 timeout);
/*---------------------------------------------------------------------------*/
/* disconnect the connection
 */

 void disconnect (SOCKET s);
/*---------------------------------------------------------------------------*/
/* Establish TCP connection (Passive connection)
 */

 uint8 listen (SOCKET s);
/*---------------------------------------------------------------------------*/

 uint16 send (SOCKET s,	const uint8 * buf, uint16 len, uint16 timeout);
/*---------------------------------------------------------------------------*/

 uint16 recv (SOCKET s, uint8 * buf, uint16 len);
 void recv_flush(SOCKET s, uint16 len);
/*---------------------------------------------------------------------------*/
/* Send data (UDP/IP RAW)
 */

 uint16 sendto (SOCKET s, const uint8 * buf, uint16 len, uint8 * addr, uint16 port, uint16 timeout);
/*---------------------------------------------------------------------------*/
/* Receive data (UDP/IP RAW)
 */

 uint16 recvfrom (SOCKET s, uint8 * buf, uint16 len, uint8 * addr, uint16  *port);
/*---------------------------------------------------------------------------*/

 uint16 socket_recv_count (SOCKET s);
/*---------------------------------------------------------------------------*/

 void socket_connect_ack (SOCKET s);
/*---------------------------------------------------------------------------*/

#endif

