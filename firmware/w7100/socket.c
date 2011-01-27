/* socket.c */

/* W7100 socket driver
 */

/* This module provides an abstract interface to the eight logical sockets
 * supported by the TCP/IP hardware.
 *
 * The implementation is based on the original WIZnet sample code, but
 * cleaned up considerably, taking advantages of some additional features
 * in the low-level hardware driver (the wiz module).
 */

/* History:
 *  2009/10/01 DT   Add constants, types and functions so that users of this
 *                  module don't need to depend on the underlying wiz module.
 *  2009/09/20 DT   Add socket_init().
 *                  Read actual hardware IR register for Sn_IR_SEND_OK status
 *                  in send() and sendto().
 *  2009/09/19 DT   Use wiz_setSn_CR() for all writes to socket command
 *                  registers, which blocks until the command is complete.
 *  2009/09/13 DT   Update wiz_read_buffer() and wiz_write_buffer() to use
 *                  sensible argument types, and have them return the updated
 *                  WIZnet pointer value.
 *  2009/09/07 DT   Update the names of the functions in the wiz module.
 */

/*****************************************************************************
 * Global information
 */

#include <stdio.h>

#include "types.h"
#include "W7100.h"
#include "lcd.h"
#include "wiz_config.h"
#include "wiz.h"
#include "socket.h"
#include "vtmr.h"

/*****************************************************************************
 * Private constants, types, data and functions
 */

uint16 xdata local_port = 10000;

/*****************************************************************************
 * Public function bodies
 */

/*---------------------------------------------------------------------------*/
void socket_init (void)
{
#if WIZ_USE_INTERRUPT
  uint8 xdata i;

  /* Enable all interrupts in the TCP/IP core and then enable that interrupt
   * at the CPU.
   */
	wiz_write8 (IMR, 0xff);
	for (i = 0; i < WIZ_N_SOCKETS; i++) wiz_write8 (Sn_IMR(i), 0x1f);	
  PINT5 = 0;
	WIZ_ISR_ENABLE();
#endif	
}

/*---------------------------------------------------------------------------*/
/**
@brief  This Socket function initialize the channel in perticular mode, and set the port and wait for W7100 done it.
@return   1 for sucess else 0.
*/
uint8 socket (
  SOCKET s,
  socket_protocol_t protocol,
  uint16 port,
  uint8 flag)
{
  uint8 xdata wiz_protocol;

  switch (protocol) {
  case PROTOCOL_UDP:     wiz_protocol = Sn_MR_UDP;    break;
  case PROTOCOL_TCP:     wiz_protocol = Sn_MR_TCP;    break;
  case PROTOCOL_IP_RAW:  wiz_protocol = Sn_MR_IPRAW;  break;
  case PROTOCOL_MAC_RAW: wiz_protocol = Sn_MR_MACRAW; break;
  case PROTOCOL_PPPOE:   wiz_protocol = Sn_MR_PPPOE;  break;
  default:               return 0;
  }

  close (s);
  wiz_write8 (Sn_MR(s), wiz_protocol | flag);
  if (port != 0) {
    wiz_write16 (Sn_PORT0(s), port);
  } else {
    local_port++; /* if don't set the source port, set local_port number. */
    wiz_write16 (Sn_PORT0(s), local_port);
  }
  wiz_setSn_CR (s, Sn_CR_OPEN); /* run sockinit Sn_CR */
  return 1;
}

/*---------------------------------------------------------------------------*/
socket_state_t socket_state (SOCKET s)
{
  switch (wiz_getSn_SR (s)) {

  case SOCK_CLOSED:
    return SOCKET_CLOSED;

  case SOCK_INIT:
    return SOCKET_INITIALIZED;

  case SOCK_ESTABLISHED:
    /* check and clear Sn_IR_CON bit
     */
    if (wiz_getSn_IR(s) & Sn_IR_CON) wiz_setSn_IR (s, Sn_IR_CON);
    return SOCKET_ESTABLISHED;

  case SOCK_CLOSE_WAIT:
    return SOCKET_CLOSE_WAIT;

  case SOCK_UDP:
    return SOCKET_UDP;

  default:
    return SOCKET_ERROR;
  }
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function close the socket and parameter is "s" which represent the socket number
*/
void close (SOCKET s)
{
  wiz_setSn_CR (s, Sn_CR_CLOSE);
  wiz_setSn_IR (s, 0xff); /* clear all socket interrupts */
}


/*---------------------------------------------------------------------------*/
/**
@brief  This function established  the connection for the channel in passive (server) mode. This function waits for the request from the peer.
@return  1 for success else 0.
*/
uint8 listen (SOCKET s)  /**< s : socket number */
{
  if (wiz_getSn_SR (s) == SOCK_INIT) {
    wiz_setSn_CR (s, Sn_CR_LISTEN);
    return 1;
  }

  return 0;
}


/*---------------------------------------------------------------------------*/
/**
@brief  This function established  the connection for the channel in Active (client) mode.
    This function waits for the untill the connection is established.

@return  1 for success else 0.
*/
uint8 connect (SOCKET s, uint8 *addr, uint16 port, uint16 timeout)
{
  if (
    ((addr[0] == 0xFF) && (addr[1] == 0xFF) && (addr[2] == 0xFF)
     && (addr[3] == 0xFF))
    || ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00)
        && (addr[3] == 0x00))
    || (port == 0x00)
    )
     return 0;

  /* set destination IP */
  wiz_write32 (Sn_DIPR0(s), addr);
  wiz_write16 (Sn_DPORT0(s), port);
  // Wait for connection with timeout
  wiz_write8 (Sn_CR(s), Sn_CR_CONNECT);
  // Wait for the specified timeout on the NET timer
  vtmr_set_timeout( VTMR_NET, timeout );
  vtmr_enable( VTMR_NET );
  while( !vtmr_is_expired( VTMR_NET ) )
  {
//    if( wiz_is_dest_unreachable() )
//    {
//      wiz_clear_dest_unreachable();
//      return 0;
//    }
    if( wiz_read8( Sn_CR( s ) ) == 0 )
      break;
  }
  vtmr_disable( VTMR_NET );
  if( wiz_read8( Sn_CR( s ) ) != 0 )
    return 0;
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function used for disconnect the socket and parameter is "s" which represent the socket number
@return  1 for success else 0.
*/
void disconnect (SOCKET s)
{
  wiz_setSn_CR (s, Sn_CR_DISCON);
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function used to send the data in TCP mode
@return  1 for success else 0.
*/
uint16 send (
  SOCKET s,             /**< the socket index */
  const uint8 * buf,    /**< a pointer to data */
  uint16 len,           /**< the data size to be send */
  uint16 timeout
  )
{
  uint8  xdata status = 0;
  uint16 xdata ptr = 0;
  uint16 xdata freesize = 0;
  uint16 xdata ret = 0;
  uint16 xdata ssize = wiz_ssize (s);

  /* check size not to exceed MAX size. */
  ret = (len > ssize) ? ssize : len;

  /* if freebuf is available, start. */
  do {
    freesize = wiz_getSn_TX_FSR (s);
    status = wiz_getSn_SR (s);
    if ((status != SOCK_ESTABLISHED) && (status != SOCK_CLOSE_WAIT)) {
      ret = 0;
      break;
    }
  } while (freesize < ret);

  /* read TX write pointer */
  ptr = wiz_read16 (Sn_TX_WR0(s));

  /* copy data */
  ptr = wiz_write_buffer (s, (uint16) buf, ptr, len);

  /* Update TX write pointer */
  wiz_write16 (Sn_TX_WR0(s), ptr);

  /* Excute SEND command */
  wiz_setSn_CR (s, Sn_CR_SEND);

  /* wait for completion */
  vtmr_set_timeout( VTMR_NET, timeout );
  vtmr_enable( VTMR_NET );
  while( !vtmr_is_expired( VTMR_NET ) )
  {
    if( ( wiz_read8( Sn_IR( s ) ) & Sn_IR_SEND_OK ) == Sn_IR_SEND_OK )
      break;
    if( wiz_getSn_SR( s ) == SOCK_CLOSED ) 
    {
      close( s );
      vtmr_disable( VTMR_NET );
      return 0;
    }
  }
  vtmr_disable( VTMR_NET );
  if( ( wiz_read8( Sn_IR( s ) ) & Sn_IR_SEND_OK ) != Sn_IR_SEND_OK )
    ret = 0;

  wiz_setSn_IR (s, Sn_IR_SEND_OK);  /* clear send_ok interrupt */

  return ret;
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function is an application I/F function which is used to receive the data in TCP mode.
    It continues to wait for data as much as the application wants to receive.

@return  received data size for success else -1.
*/
uint16 recv (
  SOCKET s,             /**< socket index */
  uint8 * buf,          /**< a pointer to copy the data to be received */
  uint16 len            /**< the data size to be read */
  )
{
  uint16 xdata ptr = 0;

  if (len > 0) {
    /* read RX read pointer */
    ptr = wiz_read16 (Sn_RX_RD0(s));

    /* copy data */
    ptr = wiz_read_buffer (s, ptr, (uint16) buf, len);

    /* Update RX read pointer */
    wiz_write16 (Sn_RX_RD0(s), ptr);

    /* Excute RECV command */
    wiz_setSn_CR (s, Sn_CR_RECV);

    return len;
  }

  return 0;
}

// As above, but ignore the received data
void recv_flush (
  SOCKET s,             /**< socket index */
  uint16 len            /**< the data size to be read and ignored*/
  )
{
  uint16 xdata ptr = 0;

  if (len > 0) {
    /* read RX read pointer */
    ptr = wiz_read16 (Sn_RX_RD0(s));

    /* Update RX read pointer */
    wiz_write16 (Sn_RX_RD0(s), ptr);

    /* Excute RECV command */
    wiz_setSn_CR (s, Sn_CR_RECV);
  }
}


/*---------------------------------------------------------------------------*/
/**
@brief  This function is an application I/F function which is used to send the data for other then TCP mode.
    Unlike TCP transmission, The peer's destination address and the port is needed.

@return  This function return send data size for success else -1.
*/
uint16 sendto (
  SOCKET s,             /**< socket index */
  const uint8 * buf,    /**< a pointer to the data */
  uint16 len,           /**< the data size to send */
  uint8 * addr,         /**< the peer's Destination IP address */
  uint16 port,          /**< the peer's destination port number */
  uint16 timeout       /**< operation timeout */
  )
{
  uint16 xdata ret = 0;
  uint16 xdata ptr = 0;
  uint16 xdata ssize = wiz_ssize (s);

  /* check size not to exceed MAX size. */
  ret = (len > ssize) ? ssize : len;

  if (
    ((addr[0] == 0x00) && (addr[1] == 0x00)
     && (addr[2] == 0x00) && (addr[3] == 0x00))
    || ((port == 0x00))
    || (ret == 0)
    )
  {
    ret = 0;
  } else {
    wiz_write32 (Sn_DIPR0(s), addr);
    wiz_write16 (Sn_DPORT0(s), port);

     /* read TX write pointer */
    ptr = wiz_read16 (Sn_TX_WR0(s));

    /* copy data */
    ptr = wiz_write_buffer (s, (uint16) buf, ptr, len);

    /* Update TX write pointer */
    wiz_write16 (Sn_TX_WR0(s), ptr);

    /* Excute SEND command */
    wiz_setSn_CR (s, Sn_CR_SEND);

    /* wait for completion */
    vtmr_set_timeout( VTMR_NET, timeout );
    vtmr_enable( VTMR_NET );
    while ((
      /* Note special case here: We don't use the ISR to read the IR;
       * we read the hardware register directly!
       */
#if 0
      wiz_getSn_IR (s)
#else
      wiz_read8 (Sn_IR(s))
#endif
      & Sn_IR_SEND_OK) != Sn_IR_SEND_OK) {
      if (wiz_getSn_IR(s) & Sn_IR_TIMEOUT) {
        wiz_setSn_IR (s, Sn_IR_TIMEOUT);  /* clear TIMEOUT Interrupt */
        ret = 0;
        break;
      }
      else if( wiz_is_dest_unreachable() )
      {
        // Destination unreachable, abort
        wiz_clear_dest_unreachable();
        ret = 0;
        break;
      }
      else if( vtmr_is_expired( VTMR_NET ) )
      {
        vtmr_disable( VTMR_NET );
        ret = 0;
        break;
      }
    }

    wiz_setSn_IR (s, Sn_IR_SEND_OK);  /* clear send_ok interrupt */
  }

  return ret;
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function is an application I/F function which is used to receive the data in other then
  TCP mode. This function is used to receive UDP, IP_RAW and MAC_RAW mode, and handle the header as well.

@return  This function return received data size for success else -1.
*/
uint16 recvfrom (
  SOCKET s,             /**< the socket number */
  uint8 * buf,          /**< a pointer to copy the data to be received */
  uint16 len,           /**< the data size to read */
  uint8 * addr,         /**< a pointer to store the peer's IP address */
  uint16 *port          /**< a pointer to store the peer's port number. */
  )
{
  uint8 xdata head[8];
  uint16 xdata data_len = 0;
  uint16 xdata ptr = 0;

  if (len > 0) {
    ptr = wiz_read16 (Sn_RX_RD0(s));

    switch (wiz_read8 (Sn_MR(s)) & 0x07) {

    case Sn_MR_UDP:
      /* read peer's IP address, port number, length. */
      ptr = wiz_read_buffer (s, ptr, (uint16) head, 8);
      addr[0] = head[0];
      addr[1] = head[1];
      addr[2] = head[2];
      addr[3] = head[3];
      *port = (head[4] << 8) + head[5];
      data_len = (head[6] << 8) + head[7];
      if( data_len > len )
        data_len = len;
      ptr = wiz_read_buffer (s, ptr, (uint16) buf, data_len); /* data copy. */
      wiz_write16 (Sn_RX_RD0(s), ptr);
      break;

    case Sn_MR_IPRAW:
      ptr = wiz_read_buffer (s, ptr, (uint16) head, 6);
      addr[0] = head[0];
      addr[1] = head[1];
      addr[2] = head[2];
      addr[3] = head[3];
      data_len = (head[4] << 8) + head[5];
      if( data_len > len )
        data_len = len;
      ptr = wiz_read_buffer (s, ptr, (uint16) buf, data_len); /* data copy. */
      wiz_write16 (Sn_RX_RD0(s), ptr);
      break;

    case Sn_MR_MACRAW:
      ptr = wiz_read_buffer (s, ptr, (uint16) head, 2);
      data_len = (head[0] << 8) + head[1] - 2;
      ptr = wiz_read_buffer (s, ptr, (uint16) buf, data_len);
      wiz_write16 (Sn_RX_RD0(s), ptr);
      if( data_len > len )
        data_len = len;
      break;

    default:
      break;
    }

    wiz_setSn_CR (s, Sn_CR_RECV);
  }

  return data_len;
}

/*---------------------------------------------------------------------------*/
uint16 socket_recv_count (SOCKET s)
{
  return wiz_getSn_RX_RSR (s);
}

/*---------------------------------------------------------------------------*/
void socket_connect_ack (SOCKET s)
{
  wiz_setSn_IR (s, Sn_IR_CON);
}

/*---------------------------------------------------------------------------*/
