/* wiz_config.h */

/* W7100 Ethernet Interface configuration
 */

/* This file defines how the firmware running on the processor gets access to
 * the Ethernet TCP/IP core hardware. It does NOT have anything to do with the
 * logical configuration of the Ethernet interface, such as MAC address, IP
 * address, etc.
 */

/* History:
 *  2009/09/06 DT   Copy from W7100 example code, reformat.
 */

#ifndef	_WIZ_CONFIG_H_
#define	_WIZ_CONFIG_H_

/*****************************************************************************
 * Public constants, types and data
 */

#define	WIZ_N_SOCKETS		8	/**< Maxmium number of socket  */

/**
* WIZ_xxx__ : define option for iinchip driver
*/
/* #define WIZ_DEBUG /* involve debug code in driver (socket.c) */

/* involve interrupt service routine (socket.c) */
#define WIZ_USE_INTERRUPT 0

/**
* WIZ_MAP_xxx__ : define memory map for iinchip
*/
#define WIZ_COMMON_BASE 0x0000
#define WIZ_MAP_TXBUF__ (WIZ_COMMON_BASE + 0x8000) /* Internal Tx buffer address of the iinchip */
#define WIZ_MAP_RXBUF__ (WIZ_COMMON_BASE + 0xc000) /* Internal Rx buffer address of the iinchip */

#if WIZ_USE_INTERRUPT
#define WIZ_ISR_DISABLE()	(EINT5 = 0)
#define WIZ_ISR_ENABLE()	(EINT5 = 1)
#else
#define WIZ_ISR_DISABLE()	
#define WIZ_ISR_ENABLE()	
#endif

/*---------------------------------------------------------------------------*/
#endif
