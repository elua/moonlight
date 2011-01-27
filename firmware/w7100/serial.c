#include "W7100.h"
#include "serial.h"
#include "type.h"
#include "vtmr.h"
#include "wiznet.h"
#include "stm32conf.h"
#include <stdio.h>

// Serial buffers and state variables
static u8 xdata ser_rx_buf[ SER_RX_BUF_SIZE ];
static volatile u16 idata ser_rx_w_idx, ser_tx_r_idx;
static u16 idata ser_rx_r_idx, ser_tx_w_idx;
static volatile u8 idata ser_tx_in_progress;
static u8 idata ser_had_escape;
static u8 idata ser_c;
static u8 idata ser_enqueue_char;
static volatile u8 idata ser_num_rpc_requests;
static u8 idata ser_mode;

// Serial interrupt handler
void serial_interrupt_handler() interrupt 4 using 2
{
  // Check RX
	if( RI )
	{
		RI = 0;
    ser_c = SBUF;
    ser_enqueue_char = 1;
    if( ser_mode == SER_MODE_RPC  )
    {
      if( ser_had_escape )
      {
        ser_had_escape = 0;
        ser_c = ser_c ^ WIZ_ESC_MASK;
      }
      else if( ser_c == WIZ_ESC_CHAR )
      {
        ser_had_escape = 1;
        ser_enqueue_char = 0;
      }
      else if( ser_c == WIZ_EREQ_CHAR )
      { 
        ser_num_rpc_requests ++;
        ser_enqueue_char = 0;
      }
    } 
    if( ser_enqueue_char )
    {
		  ser_rx_buf[ ser_rx_w_idx ] = ser_c;
      ser_rx_w_idx = ( ser_rx_w_idx + 1 ) & SER_RX_BUF_MASK;
    }
	}

  if( TI )
  {
    TI = 0;
    ser_tx_in_progress = 0;
  }
}

void ser_init()
{
  // Init local state
  ser_tx_in_progress = 0;
  ser_rx_r_idx = ser_rx_w_idx = 0;
  ser_tx_r_idx = ser_tx_w_idx = 0;
  ser_had_escape = 0;
  ser_num_rpc_requests = 0;
  ser_mode = SER_MODE_RPC;

  // Setup serial timer
  vtmr_setup( VTMR_UART, VTMR_TYPE_ONESHOT, 1000 );

  // Setup actual UART
	ET1 = 0;		/* TIMER1 INT DISABLE */
	TMOD = 0x20;  	/* TIMER MODE 2 */
	PCON |= 0x80;		/* SMOD = 1 */
	TH1 = 0xFC;		/* X2 115200(SMOD=1) at 88.4736 MHz */
	TR1 = 1;		/* START THE TIMER1 */	
	SCON = 0x50;		/* SERIAL MODE 1, REN=1, TI=0, RI=0 */	
  PS = 1;     /* high priority interrupt */
	ES = 1;	 	/* Serial interrupt enable */
}

char putchar( char c )  
{
#if 0
  ser_write_byte ( ( u8 )c );
#else
  c = c;
#endif
  return c;
}

// Even parity lookup table
static const u8 ptable[] = 
{
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0
};

void ser_write_byte( u8 c )
{
  ser_tx_in_progress = 1;
  if( ser_mode == SER_MODE_FWUPD )
    TB8 = ptable[ c ];
  else
    while( STM32_RTS_PIN == 1 );
  SBUF = c;
  while( ser_tx_in_progress );
}

u16 ser_write( const u8 xdata *pbuf, u16 len )
{
  u16 xdata i;

  for( i = 0; i < len; i ++, pbuf ++ )
  {
    ser_tx_in_progress = 1;
    if( ser_mode == SER_MODE_FWUPD )
      TB8 = ptable[ *pbuf ];
    else
      while( STM32_RTS_PIN == 1 );
    SBUF = *pbuf;
    while( ser_tx_in_progress );
  }
  return len;
}

static int ser_read_byte_from_buffer()
{
  u8 xdata temp;

  temp = ser_rx_buf[ ser_rx_r_idx ];
  ser_rx_r_idx = ( ser_rx_r_idx + 1 ) & SER_RX_BUF_MASK;
  return ( int )temp;
}

int ser_read_byte( u16 timeout )
{
  int xdata res = -1;

  vtmr_set_timeout( VTMR_UART, timeout );
  vtmr_enable( VTMR_UART );
  while( !vtmr_is_expired( VTMR_UART ) )
    if( ser_rx_r_idx != ser_rx_w_idx )
      break;
  vtmr_disable( VTMR_UART );
  if( ser_rx_r_idx != ser_rx_w_idx )
    res = ser_read_byte_from_buffer();
  return res;
}

u16 ser_read( u8 xdata *pbuf, u16 count, u16 timeout )
{
  u16 xdata bread = 0;
  int c;

  while( count )
  {
    if( ( c = ser_read_byte( timeout ) ) == -1 )
      break;
    *pbuf ++ = c;
    bread ++;
    count --;
  }
  return bread;
}

u8 ser_got_rpc_request()
{
  if( ser_num_rpc_requests > 0 )
  {
    STM32_CTS_PIN = 1;
    ES = 0;
    ser_num_rpc_requests --;
    ES = 1;
    while( RI );
    STM32_CTS_PIN = 0;
    return 1;
  }
  return 0;
}

void ser_set_mode( u8 mode )
{
  ser_mode = mode;
  if( mode == SER_MODE_FWUPD )
    SCON = 0xD0;
  else
    SCON = 0x50;  
}
