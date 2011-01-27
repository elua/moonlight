/* wiz.h */

/* W7100 access functions for the Ethernet TCP/IP core hardware.
 */

/* History:
 *  2009/09/19 DT   Add wiz_setSn_CR().
 *  2009/09/13 DT   Update wiz_read_buffer() and wiz_write_buffer() to use
 *                  sensible argument types, and have them return the updated
 *                  WIZnet pointer value.
 *  2009/09/06 DT   Copy from W7100 example code, reformat.
 */

/*****************************************************************************
 * Public constants, types and data
 */

#ifndef __WIZ_H__
#define __WIZ_H__

#include "types.h"
#include "wiz_config.h"

/*************************************************************/
/*          COMMON REGISTERS                                 */
/*************************************************************/

#define WIZ_COMM_REG(x) ((volatile uint8 xdata *)(WIZ_COMMON_BASE + x))

#define MR                  WIZ_COMM_REG(0x0001)        /* 1 byte */

/* Gateway IP Register address */
#define GAR0                WIZ_COMM_REG(0x0001)        /* 4 bytes */

/* Subnet mask Register address */
#define SUBR0               WIZ_COMM_REG(0x0005)        /* 4 bytes */

/* Source MAC Register address */
#define SHAR0               WIZ_COMM_REG(0x0009)        /* 6 bytes */

/* Source IP Register address */
#define SIPR0               WIZ_COMM_REG(0x000F)        /* 4 bytes */

/* Interrupt Register */
#define IR                  WIZ_COMM_REG(0x0015)        /* 1 byte */

/* Interrupt mask register */
#define IMR                 WIZ_COMM_REG(0x0016)        /* 1 byte */

/* Timeout register address( 1 is 100us ) */
#define RTR                 WIZ_COMM_REG(0x0017)        /* 2 bytes */

/* Retry count reigster */
#define RCR                 WIZ_COMM_REG(0x0019)        /* 1 byte */

/* Authentication type register address in PPPoE mode */
#define PATR0               WIZ_COMM_REG(0x001C)        /* 2 bytes */
#define PPPALGO             WIZ_COMM_REG(0x001E)        /* 1 byte */

/* Chip version register address */
#define VERSIONR            WIZ_COMM_REG(0x001F)        /* 1 byte */

/* PPP Link control protocol request timer */
#define PTIMER              WIZ_COMM_REG(0x0028)        /* 1 byte */

/* PPP LCP magic number register */
#define PMAGIC              WIZ_COMM_REG(0x0029)        /* 1 byte */

/* Unreachable IP register address in UDP mode */
#define UIPR0               WIZ_COMM_REG(0x002A)        /* 4 bytes */

/* Unreachable Port register address in UDP mode */
#define UPORT0              WIZ_COMM_REG(0x002E)        /* 2 bytes */

/* SOCKET Interrupt Register */
#define IR2                 WIZ_COMM_REG(0x0034)        /* 1 byte */

/*************************************************************/
/*          SOCKET REGISTERS                                 */
/*************************************************************/

/* Size and position of each channel's register map */
#define WIZ_SOCK_BASE   (WIZ_COMMON_BASE + 0x4000)
#define WIZ_SOCK_SIZE   ((int) 0x0100)
#define WIZ_SOCK_REG(ch, x) (WIZ_SOCK_BASE + ch * WIZ_SOCK_SIZE + x)

/* Socket Mode register */
#define Sn_MR(ch)           WIZ_SOCK_REG(ch, 0x0000)    /* 1 byte */

/* Channel Sn_CR register */
#define Sn_CR(ch)           WIZ_SOCK_REG(ch, 0x0001)    /* 1 byte */

/* Channel interrupt register */
#define Sn_IR(ch)           WIZ_SOCK_REG(ch, 0x0002)    /* 1 byte */

/* Channel status register */
#define Sn_SR(ch)           WIZ_SOCK_REG(ch, 0x0003)    /* 1 byte */

/* Source port register */
#define Sn_PORT0(ch)        WIZ_SOCK_REG(ch, 0x0004)    /* 2 bytes */

/* Peer MAC register address */
#define Sn_DHAR0(ch)        WIZ_SOCK_REG(ch, 0x0006)    /* 6 bytes */

/* Peer IP register address */
#define Sn_DIPR0(ch)        WIZ_SOCK_REG(ch, 0x000C)    /* 4 bytes */

/* Peer port register address */
#define Sn_DPORT0(ch)       WIZ_SOCK_REG(ch, 0x0010)    /* 2 bytes */

/* Maximum Segment Size(Sn_MSSR0) register address */
#define Sn_MSSR0(ch)        WIZ_SOCK_REG(ch, 0x0012)    /* 2 bytes */

/* Protocol of IP Header field register in IP raw mode */
#define Sn_PROTO(ch)        WIZ_SOCK_REG(ch, 0x0014)    /* 1 byte */

/* IP Type of Service(TOS) Register  */
#define Sn_TOS(ch)          WIZ_SOCK_REG(ch, 0x0015)    /* 1 byte */

/* IP Time to live(TTL) Register  */
#define Sn_TTL(ch)          WIZ_SOCK_REG(ch, 0x0016)    /* 1 byte */

/* Receive memory size reigster */
#define Sn_RXMEM_SIZE(ch)   WIZ_SOCK_REG(ch, 0x001E)    /* 1 byte */

/* Transmit memory size reigster */
#define Sn_TXMEM_SIZE(ch)   WIZ_SOCK_REG(ch, 0x001F)    /* 1 byte */

/* Transmit free memory size register */
#define Sn_TX_FSR0(ch)      WIZ_SOCK_REG(ch, 0x0020)    /* 2 bytes */

/* Transmit memory read pointer register address */
#define Sn_TX_RD0(ch)       WIZ_SOCK_REG(ch, 0x0022)    /* 2 bytes */

/* Transmit memory write pointer register address */
#define Sn_TX_WR0(ch)       WIZ_SOCK_REG(ch, 0x0024)    /* 2 bytes */

/* Received data size register */
#define Sn_RX_RSR0(ch)      WIZ_SOCK_REG(ch, 0x0026)    /* 2 bytes */

/* Read point of Receive memory */
#define Sn_RX_RD0(ch)       WIZ_SOCK_REG(ch, 0x0028)    /* 2 bytes */

/* Write point of Receive memory */
#define Sn_RX_WR0(ch)       WIZ_SOCK_REG(ch, 0x002A)    /* 2 bytes */

/* Socket interrupt mask register */
#define Sn_IMR(ch)          WIZ_SOCK_REG(ch, 0x002C)    /* 1 byte */

/* Frag field value in IP header register */
#define Sn_FRAG0(ch)        WIZ_SOCK_REG(ch, 0x002D)    /* 2 bytes */

/*************************************************************/
/*          BIT of REGISTERS                                 */
/*************************************************************/
/* MODE register bits */
#define MR_RST          0x80    /**< reset */
#define MR_PB           0x10    /**< ping block */
#define MR_PPPOE        0x08    /**< enable pppoe */

/* IR register bits */
#define IR_CONFLICT     0x80    /**< check ip confict */
#define IR_UNREACH      0x40    /**< get the destination unreachable message in UDP sending */
#define IR_PPPoE        0x20    /**< get the PPPoE close message */

/* IMR, IR2 register bits */
#define IR_SOCK(ch)    (0x01 << ch) /**< check socket interrupt */

/* Sn_MR bits */
#define Sn_MR_MULTI     0x80    /**< support multicasting */
#define Sn_MR_ND        0x20    /**< No Delayed Ack(TCP) flag */
/* Sn_MR values */
#define Sn_MR_PPPOE     0x05    /**< PPPoE */
#define Sn_MR_MACRAW    0x04    /**< MAC LAYER RAW SOCK */
#define Sn_MR_IPRAW     0x03    /**< IP LAYER RAW SOCK */
#define Sn_MR_UDP       0x02    /**< UDP */
#define Sn_MR_TCP       0x01    /**< TCP */
#define Sn_MR_CLOSE     0x00    /**< unused socket */

/* Sn_CR values */
#define Sn_CR_OPEN      0x01    /**< initialize or open socket */
#define Sn_CR_LISTEN    0x02    /**< wait connection request in tcp mode(Server mode) */
#define Sn_CR_CONNECT   0x04    /**< send connection request in tcp mode(Client mode) */
#define Sn_CR_DISCON    0x08    /**< send closing reqeuset in tcp mode */
#define Sn_CR_CLOSE     0x10    /**< close socket */
#define Sn_CR_SEND      0x20    /**< update txbuf pointer, send data */
#define Sn_CR_SEND_MAC  0x21    /**< send data with MAC address, so without ARP process */
#define Sn_CR_SEND_KEEP 0x22    /**<  send keep alive message */
#define Sn_CR_RECV      0x40    /**< update rxbuf pointer, recv data */

#define Sn_CR_PCON      0x23    /**< ADSL connection begins by transmitting PPPoE discovery packet */
#define Sn_CR_PDISCON   0x24    /**< close ADSL connection */
#define Sn_CR_PCR       0x25    /**< In each phase, it transmits REQ message */
#define Sn_CR_PCN       0x26    /**< In each phase, it transmits NAK message */
#define Sn_CR_PCJ       0x27    /**< In each phase, it transmits REJECT message */

/* Sn_IR, Sn_IMR values */
#define Sn_IR_PRECV     0x80    /**< PPP receive interrupt */
#define Sn_IR_PFAIL     0x40    /**< PPP fail interrupt */
#define Sn_IR_PNEXT     0x20    /**< PPP next phase interrupt */
#define Sn_IR_SEND_OK   0x10    /**< complete sending */
#define Sn_IR_TIMEOUT   0x08    /**< assert timeout */
#define Sn_IR_RECV      0x04    /**< receiving data */
#define Sn_IR_DISCON    0x02    /**< closed socket */
#define Sn_IR_CON       0x01    /**< established connection */

/* Sn_SR values */
#define SOCK_CLOSED     0x00    /**< closed */
#define SOCK_INIT       0x13    /**< init state */
#define SOCK_LISTEN     0x14    /**< listen state */
#define SOCK_SYNSENT    0x15    /**< connection state */
#define SOCK_SYNRECV    0x16    /**< connection state */
#define SOCK_ESTABLISHED 0x17   /**< success to connect */
#define SOCK_FIN_WAIT   0x18    /**< closing state */
#define SOCK_CLOSING    0x1A    /**< closing state */
#define SOCK_TIME_WAIT  0x1B    /**< closing state */
#define SOCK_CLOSE_WAIT 0x1C    /**< closing state */
#define SOCK_LAST_ACK   0x1D    /**< closing state */
#define SOCK_UDP        0x22    /**< udp socket */
#define SOCK_IPRAW      0x32    /**< ip raw mode socket */
#define SOCK_MACRAW     0x42    /**< mac raw mode socket */
#define SOCK_PPPOE      0x5F    /**< pppoe socket */

/* IP PROTOCOL */
#define IPPROTO_IP         0    /**< Dummy for IP */
#define IPPROTO_ICMP       1    /**< Control message protocol */
#define IPPROTO_IGMP       2    /**< Internet group management protocol */
#define IPPROTO_GGP        3    /**< Gateway^2 (deprecated) */
#define IPPROTO_TCP        6    /**< TCP */
#define IPPROTO_PUP       12    /**< PUP */
#define IPPROTO_UDP       17    /**< UDP */
#define IPPROTO_IDP       22    /**< XNS idp */
#define IPPROTO_ND        77    /**< UNOFFICIAL net disk protocol */
#define IPPROTO_RAW      255    /**< Raw IP packet */

/*****************************************************************************
 * Public Functions
 */

/*---------------------------------------------------------------------------*/

 uint8 wiz_read8 (uint16 addr);
/*---------------------------------------------------------------------------*/

 void wiz_write8 (uint16 addr, uint8 wizdata);
/*---------------------------------------------------------------------------*/

 uint16 wiz_read16 (uint16 addr);
/*---------------------------------------------------------------------------*/

 void wiz_write16 (uint16 addr, uint16 wizdata);
/*---------------------------------------------------------------------------*/

 void wiz_read32 (uint16 addr, uint8 *wizdata);
/*---------------------------------------------------------------------------*/

 void wiz_write32 (uint16 addr, uint8 *wizdata);
/*---------------------------------------------------------------------------*/

 void wiz_read48 (uint16 addr, uint8 *wizdata);
/*---------------------------------------------------------------------------*/

 void wiz_write48 (uint16 addr, uint8 *wizdata);
/*---------------------------------------------------------------------------*/
/* setting TX/RX buffer size */

 void wiz_set_memsize (uint8* tx_size, uint8* rx_size);
/*---------------------------------------------------------------------------*/
/* Use this function instead of wiz_read8(),
 * particularly when using the interrupt.
 */

 uint8 wiz_getSn_IR (uint8 s);
/*---------------------------------------------------------------------------*/
/* Use this function instead of wiz_write8(),
 * particularly when using the interrupt.
 */

 void wiz_setSn_IR (uint8 s, uint8 val);
/*---------------------------------------------------------------------------*/
/* get socket status */

/* This function polls the register until it gets the same value twice in
 * a row. (TBD: why???)
 */

 uint8 wiz_getSn_SR (SOCKET s);
/*---------------------------------------------------------------------------*/
/* This function writes a command to Sn_CR, and then blocks until that register
 * becomes zero again.
 */

 void wiz_setSn_CR (SOCKET s, uint8 val);
/*---------------------------------------------------------------------------*/
/* get socket TX free buffer size */

/* This function polls the register until it gets the same value twice in
 * a row. (TBD: why???)
 */

 uint16 wiz_getSn_TX_FSR (SOCKET s);
/*---------------------------------------------------------------------------*/
/* get socket RX recv buffer size */

/* This function polls the register until it gets the same value twice in
 * a row. (TBD: why???)
 */

 uint16 wiz_getSn_RX_RSR (SOCKET s);
/*---------------------------------------------------------------------------*/

 uint16 wiz_ssize (SOCKET s);
/*---------------------------------------------------------------------------*/
/* Transfer a buffer from CPU memory to WIZnet memory.
 * Returns the updated value for the dst argument.
 */

 uint16 wiz_write_buffer (SOCKET s, uint16 src, uint16 dst, uint16 len);
/*---------------------------------------------------------------------------*/
/* Transfer a buffer from WIZnet memory to CPU memory.
 * Returns the updated value for the src argument.
 */

 uint16 wiz_read_buffer (SOCKET s, uint16 src, uint16 dst, uint16 len);
/*---------------------------------------------------------------------------*/

/* Return/clear the "destination unreachable" indication
 */

 uint8 wiz_is_dest_unreachable();
 void wiz_clear_dest_unreachable();
/*---------------------------------------------------------------------------*/

#endif
