/* wiz.c */

/* W7100 TCP/IP core access
 */

/*
 * (c)COPYRIGHT WIZnet Inc. ALL RIGHT RESERVED
 *
 * FileName : wiz.c
 * Revision History :
 * ----------  -------    ------------------------------------------------
 *   Date      version      Description
 * ----------  -------    ------------------------------------------------
 * 08/01/2009  1.0        Release Version
 * ----------  -------    ------------------------------------------------
 */

/* History:
 *  2009/09/19 DT   Add wiz_setSn_CR().
 *                  Fix misconception about how the buffer pointers get
 *                  updated.
 *  2009/09/13 DT   Update wiz_read_buffer() and wiz_write_buffer() to use
 *                  sensible argument types, and have them return the updated
 *                  WIZnet pointer value.
 *                  Clean up the code in wiz_set_memsize().
 *  2009/09/06 DT   Copy from W7100 example code, reformat.
 */

/*****************************************************************************
 * Global information
 */

#include <string.h>
#include <stdio.h>

#include "W7100.h"
#include "types.h"
#include "wizmemcpy.h"
#include "wiz_config.h"
#include "wiz.h"
#include "stm32conf.h"

/*****************************************************************************
 * Private constants, types, data and functions
 */

#if WIZ_USE_INTERRUPT
static vuint8 idata wiz_i_status[WIZ_N_SOCKETS];
#endif

static struct {
  uint16 ssize;         /**< Max Tx buffer size by each channel */
  uint16 sbufbase;      /**< Tx buffer base address by each channel */
  uint16 rsize;         /**< Max Rx buffer size by each channel */
  uint16 rbufbase;      /**< Rx buffer base address by each channel */
} xdata wiz_ch[WIZ_N_SOCKETS];

// Destination unreachable flag (set in the interrupt handler)
static volatile uint8 idata wiz_dest_unreachable;

/*****************************************************************************
 * Public function bodies
 */

/*---------------------------------------------------------------------------*/
/**
@brief  This function reads the value from W7100 registers.
*/
uint8 wiz_read8 (uint16 addr)
{
  register wizdata;
  uint8 tmpEA;

  tmpEA = EA;
  STM32_CTS_PIN = 1;
  EA = 0;
  DPX0 = 0xFE;
  wizdata = *((vuint8 xdata*)(addr));
  DPX0 = 0x00;
  EA = tmpEA;
  if( tmpEA )
  {
    while( RI );
    STM32_CTS_PIN = 0;
  }
  return wizdata;
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function writes the data into W7100 registers.
*/
void wiz_write8 (uint16 addr, uint8 wizdata)
{
  uint8 tmpEA;

  tmpEA = EA;
  STM32_CTS_PIN = 1;
  EA = 0;
  DPX0 = 0xFE;
  *((vuint8 xdata*)(addr)) = wizdata;
  DPX0 = 0x00;
  EA = tmpEA;
  if( tmpEA )
  {
    while( RI );
    STM32_CTS_PIN = 0;
  }
}

/*---------------------------------------------------------------------------*/
uint16 wiz_read16 (uint16 addr)
{
  uint8 hi_byte = wiz_read8 (addr);
  uint8 lo_byte = wiz_read8 (addr+1);
  return (hi_byte << 8) + lo_byte;
}

/*---------------------------------------------------------------------------*/
void wiz_write16 (uint16 addr, uint16 wizdata)
{
  wiz_write8 (addr,   (uint8) (wizdata >> 8));
  wiz_write8 (addr+1, (uint8) (wizdata & 0xFF));
}

/*---------------------------------------------------------------------------*/
void wiz_read32 (uint16 addr, uint8 *wizdata)
{
  wizdata[0] = wiz_read8 (addr);
  wizdata[1] = wiz_read8 (addr+1);
  wizdata[2] = wiz_read8 (addr+2);
  wizdata[3] = wiz_read8 (addr+3);
}

/*---------------------------------------------------------------------------*/
void wiz_write32 (uint16 addr, uint8 *wizdata)
{
  wiz_write8 (addr,   wizdata[0]);
  wiz_write8 (addr+1, wizdata[1]);
  wiz_write8 (addr+2, wizdata[2]);
  wiz_write8 (addr+3, wizdata[3]);
}

/*---------------------------------------------------------------------------*/
void wiz_read48 (uint16 addr, uint8 *wizdata)
{
  wizdata[0] = wiz_read8 (addr);
  wizdata[1] = wiz_read8 (addr+1);
  wizdata[2] = wiz_read8 (addr+2);
  wizdata[3] = wiz_read8 (addr+3);
  wizdata[4] = wiz_read8 (addr+4);
  wizdata[5] = wiz_read8 (addr+5);
}

/*---------------------------------------------------------------------------*/
void wiz_write48 (uint16 addr, uint8 *wizdata)
{
  wiz_write8 (addr,   wizdata[0]);
  wiz_write8 (addr+1, wizdata[1]);
  wiz_write8 (addr+2, wizdata[2]);
  wiz_write8 (addr+3, wizdata[3]);
  wiz_write8 (addr+4, wizdata[4]);
  wiz_write8 (addr+5, wizdata[5]);
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function set the transmit & receive buffer size as per the channels is used

Maximum memory size for Tx, Rx in the W7100 is 16K Bytes,\n
In the range of 16KBytes, the memory size could be allocated dynamically by each channel.\n
Be attentive to sum of memory size shouldn't exceed 16Kbytes\n
and to data transmission and receiption from non-allocated channel may cause some problems.\n
If the 16KBytes memory is already  assigned to centain channel, \n
other channels couldn't be used, for there's no available memory.\n
*/
void wiz_set_memsize (uint8* tx_size, uint8* rx_size)
{
  int16 xdata i;
  int16 xdata ssum;
  int16 xdata rsum;
  int16 xdata ssize;
  int16 xdata rsize;

  ssum = 0;
  rsum = 0;

  /* Set the size and base address of Tx & Rx memory by each channel
   */
  for (i = 0 ; i < WIZ_N_SOCKETS; i++) {

    switch( tx_size[i] ) {
    case 1:  ssize = 0x0400; break;
    case 2:  ssize = 0x0800; break;
    case 4:  ssize = 0x1000; break;
    case 8:  ssize = 0x2000; break;
    default: ssize =      0; break;
    }

    switch( rx_size[i] ) {
    case 1:  rsize = 0x0400; break;
    case 2:  rsize = 0x0800; break;
    case 4:  rsize = 0x1000; break;
    case 8:  rsize = 0x2000; break;
    default: rsize =      0; break;
    }

#if 0
    /* TBD: if any channel's size is *larger* than a previous channel's,
     * then it may be necessary to adjust the base address so that it falls
     * on a properly-aligned boundary for the current channel's size.
     */
    if (ssum & (ssize - 1)) {
      /* misaligned! */
      ssum += ssize;
      ssum &= ~(ssize - 1);
    }
    if (rsum & (rsize - 1)) {
      /* misaligned! */
      rsum += rsize;
      rsum &= ~(rsize - 1);
    }

    /* Now check whether there's still enough room for the specified buffer.
     */
    if (ssum + ssize > 0x4000) {
      tx_size[i] = 0;
      ssize = 0;
    }
    if (rsum + rsize > 0x4000) {
      rx_size[i] = 0;
      rsize = 0;
    }
#endif

    wiz_ch[i].ssize = ssize;
    wiz_ch[i].rsize = rsize;

    wiz_ch[i].sbufbase = WIZ_MAP_TXBUF__ + ssum;
    wiz_ch[i].rbufbase = WIZ_MAP_RXBUF__ + rsum;

    wiz_write8 (Sn_TXMEM_SIZE(i), tx_size[i]);
    wiz_write8 (Sn_RXMEM_SIZE(i), rx_size[i]);

#if 0
    /* Initialize the hardware pointers to the buffer base addresses
     * TBD: I'm not sure if this is necessary (none of the sample code
     * does it), but it's worth a shot to see if it clears up some weird
     * buffer problems.
     */
    wiz_write16 (Sn_TX_WR0(i), wiz_ch[i].sbufbase);
    wiz_write16 (Sn_TX_RD0(i), wiz_ch[i].sbufbase);
    wiz_write16 (Sn_RX_WR0(i), wiz_ch[i].rbufbase);
    wiz_write16 (Sn_RX_RD0(i), wiz_ch[i].rbufbase);
#endif

    ssum += ssize;
    rsum += rsize;
  }
}

/*---------------------------------------------------------------------------*/
uint8 wiz_getSn_IR (SOCKET s)
{
#if WIZ_USE_INTERRUPT
  return wiz_i_status[s];
#else
  return wiz_read8 (Sn_IR(s));
#endif
}

/*---------------------------------------------------------------------------*/
void wiz_setSn_IR (SOCKET s, uint8 val)
{
#if WIZ_USE_INTERRUPT
  /* NOTE: If value has Sn_IR_SEND_OK set,
   * we must do the write to the hardware!
   */
  if (val & Sn_IR_SEND_OK) {
    wiz_write8 (Sn_IR(s), val);
  }

  /* TBD: protect with critical section!
   */
  wiz_i_status[s] = wiz_i_status[s] & (~val);
#else
  wiz_write8 (Sn_IR(s), val);
#endif
}

/*---------------------------------------------------------------------------*/
void wiz_setSn_CR (SOCKET s, uint8 val)
{
  wiz_write8 (Sn_CR(s), val);
  while (wiz_read8 (Sn_CR(s))) ;
}

/*---------------------------------------------------------------------------*/
uint8 wiz_getSn_SR (SOCKET s)
{
  uint8 xdata sr  = 0xff;
  uint8 xdata sr1 = 0xff;

  while(1)
  {
    sr = (uint8) wiz_read8 (Sn_SR(s));
    if (sr == sr1) break;
    sr1 = sr;
  }
  return sr;
}

/*---------------------------------------------------------------------------*/
/**
@brief  get socket TX free buf size

This gives free buffer size of transmit buffer. This is the data size that user can transmit.
User shuold check this value first and control the size of transmitting data
*/
uint16 wiz_getSn_TX_FSR (SOCKET s)
{
  uint16 xdata val  = 0;
  uint16 xdata val1 = 0;

  while (1) {
    val = wiz_read16 (Sn_TX_FSR0(s));
    if (val == val1) break;
    val1 = val;
  }
  return val;
}

/*---------------------------------------------------------------------------*/
/**
@brief   get socket RX recv buf size

This gives size of received data in receive buffer.
*/
uint16 wiz_getSn_RX_RSR (SOCKET s)
{
  uint16 xdata val  = 0;
  uint16 xdata val1 = 0;

  while(1) {
    val = wiz_read16 (Sn_RX_RSR0(s));
    if (val == val1) break;
    val1 = val;
  }
  return val;
}

/*---------------------------------------------------------------------------*/
uint16 wiz_ssize (SOCKET s)
{
  return wiz_ch[s].ssize;
}

/*---------------------------------------------------------------------------*/
/**
@brief   This function is being called by send() and sendto() function also.

This function read the Tx write pointer register and after copy the data in
buffer update the Tx write pointer register. User should read upper byte
first and lower byte later to get proper value.

Returns the updated destination pointer value.

It turns out that although the destination pointer is a 16-bit value that gets
"wrapped" as needed to form the actual memory addresses, the value that gets
written back to the TX_WR register is the full 16-bit value.
*/
uint16 wiz_write_buffer (SOCKET s, uint16 src, uint16 dst, uint16 len)
{
  uint16 xdata size;
  uint16 xdata dst_mask;
  uint16 xdata new_dst = dst + len;

  dst_mask = dst & (wiz_ch[s].ssize - 1);
  dst = wiz_ch[s].sbufbase + dst_mask;

  if (dst_mask + len > wiz_ch[s].ssize) {
    size = wiz_ch[s].ssize - dst_mask;
    wizmemcpy ((0x000000 + src), (0xFE0000 + dst), size);
    src += size;
    size = len - size;
    dst = wiz_ch[s].sbufbase;
    wizmemcpy ((0x000000 + src), (0xFE0000 + dst), size);
    dst += size;
  } else {
    wizmemcpy((0x000000 + src), (0xFE0000 + dst), len);
    dst += len;
  }
  return new_dst;
}

/*---------------------------------------------------------------------------*/
/**
@brief  This function is being called by recv() also.

This function read the Rx read pointer register and after copy the data
from receive buffer update the Rx write pointer register. User should read
upper byte first and lower byte later to get proper value.

Returns the updated source pointer value.

It turns out that although the source pointer is a 16-bit value that gets
"wrapped" as needed to form the actual memory addresses, the value that gets
written back to the RX_RD register is the full 16-bit value.
*/
uint16 wiz_read_buffer (SOCKET s, uint16 src, uint16 dst, uint16 len)
{
  uint16 xdata size;
  uint16 xdata src_mask;
  uint16 xdata new_src = src + len;

  src_mask = src & (wiz_ch[s].rsize - 1);
  src = wiz_ch[s].rbufbase + src_mask;

  if ((src_mask + len) > wiz_ch[s].rsize) {
    size = wiz_ch[s].rsize - src_mask;
    wizmemcpy ((0xFE0000 + src), (0x000000 + dst), size);
    dst += size;
    size = len - size;
    src = wiz_ch[s].rbufbase;
    wizmemcpy ((0xFE0000 + src), (0x000000 + dst), size);
    src += size;
  } else {
    wizmemcpy ((0xFE0000 + src), (0x000000 + dst), len);
    src += len;
  }
  return new_src;
}

// Returns the "destination unreachable" indication
uint8 wiz_is_dest_unreachable()
{
  return wiz_dest_unreachable;
}

// Clear the "destination unreachable" indication
void wiz_clear_dest_unreachable()
{
  wiz_dest_unreachable = 0;
}

/*---------------------------------------------------------------------------*/

/*****************************************************************************
 * Private function bodies
 */

#if WIZ_USE_INTERRUPT
/*---------------------------------------------------------------------------*/
uint8 wiz_read8_isr (uint16 addr)
{
  register wizdata;

  DPX0 = 0xFE;
  wizdata = *((vuint8 xdata*)(addr));
  DPX0 = 0x00;
  return wizdata;
}

/*---------------------------------------------------------------------------*/
void wiz_write8_isr (uint16 addr, uint8 wizdata)
{
  DPX0 = 0xFE;
  *((vuint8 xdata*)(addr)) = wizdata;
  DPX0 = 0x00;
}

/*---------------------------------------------------------------------------*/
/**
@brief  Socket interrupt routine
*/
static void iinchip_irq (void) interrupt 11 using 3
{
  uint8 idata int_val;
  uint8 idata i;
  uint8 idata tmp_STATUS;

  int_val = wiz_read8_isr (IR);
  if (int_val & IR_UNREACH)
    wiz_dest_unreachable = 1;
  wiz_write8_isr (IR, int_val);

  int_val = wiz_read8_isr (IR2);

  for (i=0; i<WIZ_N_SOCKETS; i++) {
    if (int_val & IR_SOCK(i)) {
      tmp_STATUS= wiz_read8_isr (Sn_IR(i));
      wiz_i_status[i] |= (tmp_STATUS & 0xEF);
      wiz_write8_isr (Sn_IR(i), (tmp_STATUS & 0xEF));
    }
  }
  wiz_write8_isr (IR2, int_val);

  EIF &= 0xF7;  /* Clear TCPIPCore Interrupt Flag */
}
#endif

/*---------------------------------------------------------------------------*/
