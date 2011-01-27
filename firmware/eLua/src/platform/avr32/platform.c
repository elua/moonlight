// Platform-dependent functions

#include "platform.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"
#include "stacks.h"
#include <reent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "utils.h"
#include "platform_conf.h"
#include "common.h"
#include "buf.h"

// Platform-specific includes
#include <avr32/io.h>
#include "compiler.h"
#include "flashc.h"
#include "pm.h"
#include "board.h"
#include "usart.h"
#include "gpio.h"
#include "tc.h"
#include "intc.h"
#include "sdramc.h"

// ****************************************************************************
// Platform initialization

extern int pm_configure_clocks( pm_freq_param_t *param );

// Virtual timers support
#if VTMR_NUM_TIMERS > 0
#define VTMR_CH     (2)

__attribute__((__interrupt__)) static void tmr_int_handler()
{
  volatile avr32_tc_t *tc = &AVR32_TC;
  
  tc_read_sr( tc, VTMR_CH );
  cmn_virtual_timer_cb();
}                                
#endif

static const u32 uart_base_addr[ NUM_UART ] = { AVR32_USART0_ADDRESS, AVR32_USART1_ADDRESS, AVR32_USART2_ADDRESS, AVR32_USART3_ADDRESS };

// Buffered UART support
#ifdef BUF_ENABLE_UART
__attribute__((__interrupt__)) static void uart_rx_handler()
{
  int c;
  t_buf_data temp;
  volatile avr32_usart_t *pusart = ( volatile avr32_usart_t* )uart_base_addr[ CON_UART_ID ];    
  
  usart_read_char( pusart, &c );
  temp = ( t_buf_data )c;
  buf_write( BUF_ID_UART, CON_UART_ID, &temp );
}
#endif

extern void alloc_init();

int platform_init()
{
  pm_freq_param_t pm_freq_param =
  {
    REQ_CPU_FREQ,
    REQ_PBA_FREQ,
    FOSC0,
    OSC0_STARTUP, 
  };
  tc_waveform_opt_t tmropt = 
  {
    0,                                 // Channel selection.
        
    TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
    TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
    TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
    TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.
                        
    TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
    TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
    TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
    TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).
                                        
    TC_WAVEFORM_SEL_UP_MODE,           // Waveform selection: Up mode
    FALSE,                             // External event trigger enable.
    0,                                 // External event selection.
    TC_SEL_NO_EDGE,                    // External event edge selection.
    FALSE,                             // Counter disable when RC compare.
    FALSE,                             // Counter clock stopped with RC compare.
                                                                
    FALSE,                             // Burst signal selection.
    FALSE,                             // Clock inversion.
    TC_CLOCK_SOURCE_TC1                // Internal source clock 1 (32768Hz)
  };
  volatile avr32_tc_t *tc = &AVR32_TC;
  unsigned i;
         
  Disable_global_interrupt();  
  INTC_init_interrupts();
    
  // Setup clocks
  if( PM_FREQ_STATUS_FAIL == pm_configure_clocks( &pm_freq_param ) )
    return PLATFORM_ERR;  
  // Select the 32-kHz oscillator crystal
  pm_enable_osc32_crystal (&AVR32_PM );
  // Enable the 32-kHz clock
  pm_enable_clk32_no_wait( &AVR32_PM, AVR32_PM_OSCCTRL32_STARTUP_0_RCOSC );    
  
  // Initialize external memory
  sdramc_init( REQ_CPU_FREQ );
  
  // Setup UART for eLua
  platform_uart_setup( CON_UART_ID, CON_UART_SPEED, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1 );  
#if defined( BUF_ENABLE_UART ) && defined( CON_BUF_SIZE )
  // Enable buffering on the console UART
  buf_set( BUF_ID_UART, CON_UART_ID, CON_BUF_SIZE, BUF_DSIZE_U8 );
  // Set interrupt handler and interrupt flag on UART
  INTC_register_interrupt( &uart_rx_handler, CON_UART_IRQ, AVR32_INTC_INT0 );  
  volatile avr32_usart_t *pusart = ( volatile avr32_usart_t* )uart_base_addr[ CON_UART_ID ];      
  pusart->ier = AVR32_USART_IER_RXRDY_MASK;  
  Enable_global_interrupt();
#endif
    
  // Setup timers
  for( i = 0; i < 3; i ++ )
  {
    tmropt.channel = i;
    tc_init_waveform( tc, &tmropt );
  }
  
  // Setup timer interrupt for the virtual timers if needed
#if VTMR_NUM_TIMERS > 0
  INTC_register_interrupt( &tmr_int_handler, AVR32_TC_IRQ2, AVR32_INTC_INT0 );  
  tmropt.channel = VTMR_CH;
  tmropt.wavsel = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER;
  tc_init_waveform( tc, &tmropt );
  tc_interrupt_t tmrint = 
  {
    0,              // External trigger interrupt.
    0,              // RB load interrupt.
    0,              // RA load interrupt.
    1,              // RC compare interrupt.
    0,              // RB compare interrupt.
    0,              // RA compare interrupt.
    0,              // Load overrun interrupt.
    0               // Counter overflow interrupt.
  };
  tc_write_rc( tc, VTMR_CH, 32768 / VTMR_FREQ_HZ );
  tc_configure_interrupts( tc, VTMR_CH, &tmrint );
  Enable_global_interrupt();
  tc_start( tc, VTMR_CH );  
#endif

  cmn_platform_init();
    
  // All done  
  return PLATFORM_OK;
} 

// ****************************************************************************
// PIO functions

/* Note about AVR32 GPIO
PIOs on AVR32 are a weird deal. They aren't really organized in ports, but 
rather they are numbered from 0 to a maximum number (which also give the total
number of GPIOs in the system) then you do operations on that pin number. 
We need to organize this in ports though, so here's our organization:
PA: 31 bits, direct mapping to AVR GPIO0 ... GPIO30
PB: 32 bits, direct mapping to AVR GPIO32 ... GPIO63
PC: 6 bits, direct mapping to AVR GPIO64 ... GPIO69
PX: this is where all hell breaks loose. PX seems to be a quite random mapping
between the rest of the GPIOs and some "port" that has 40 bits and a very 
imaginative mapping to the GPIOs (PX0 is GPIO100, PX1 is GPIO99, PX2 is GPIO98 ...
and PX11 is GPIO109, just to give a few examples). So let's make some sense out of
this. We define two pseudo ports: a 32 bits one (GPIO70-GPIO101) and an 8 bits one 
(GPIO102-GPIO109).
PD: 32 bits, GPIO70-GPIO101
PE:  8 bits, GPIO102-GPIO109
(note that GPIO32 doesn't seem to exist at all in the system).
Beceause these are pseudo-ports, operations on them will be slower than operation
on the hardware ports (PA, PB, PC).
*/

// Port data
#define PA            0
#define PB            1
#define PC            2
#define PD            3
#define PE            4
// Reg types for our helper function
#define PIO_REG_PVR   0
#define PIO_REG_OVR   1
#define PIO_REG_GPER  2
#define PIO_REG_ODER  3
#define PIO_REG_PUER  4

#define GPIO          AVR32_GPIO

// Helper function: for a given port, return the address of a specific register (value, direction, pullup ...)
static volatile unsigned long* platform_pio_get_port_reg_addr( unsigned port, int regtype )
{
  volatile avr32_gpio_port_t *gpio_port = &GPIO.port[ port ];
  
  switch( regtype )
  {
    case PIO_REG_PVR:
      return ( unsigned long * )&gpio_port->pvr;
    case PIO_REG_OVR:
      return &gpio_port->ovr;
    case PIO_REG_GPER:
      return &gpio_port->gper;
    case PIO_REG_ODER:
      return &gpio_port->oder;
    case PIO_REG_PUER:
      return &gpio_port->puer;
  }
  // Should never get here
  return ( unsigned long* )&gpio_port->pvr;
}

// Helper function: get port value, get direction, get pullup, ...
static pio_type platform_pio_get_port_reg( unsigned port, int reg )
{
  pio_type v;
  volatile unsigned long *pv = platform_pio_get_port_reg_addr( port, reg );
  
  switch( port )
  {
    case PA:   // PA - 31 bits
      return *pv & 0x7FFFFFFF;
      
    case PB:   // PB - 32 bits
      return *pv;
      
    case PC:   // PC - 6 bits
      return *pv & 0x3F;      
      
    case PD:   // PD - pseudo port (70-101, has 26 bits on P2 and 6 bits on P3)
      pv = platform_pio_get_port_reg_addr( 2, reg );
      v = ( *pv & 0xFFFFFFC0 ) >> 6;
      pv = platform_pio_get_port_reg_addr( 3, reg );
      return ( ( *pv & 0x3F ) << 26 ) | v;
      
    case PE:   // PE - pseudo port (102-109, 8 bits on P3)
      pv = platform_pio_get_port_reg_addr( 3, reg );
      return ( *pv & 0x3FC0 ) >> 6;
  }
  // Will never get here
  return 0;
}

// Helper function: set port value, set direction, set pullup ...
static void platform_pio_set_port_reg( unsigned port, pio_type val, int reg )
{
  volatile unsigned long *pv = platform_pio_get_port_reg_addr( port, reg );
    
  switch( port )
  {
    case PA:   // PA - 31 bits
      *pv = val & 0x7FFFFFFF;
      break;
            
    case PB:   // PB - 32 bits
      *pv = val;
      break;
      
    case PC:   // PC - 6 bits
      *pv = ( *pv & ~0x3F ) | ( val & 0x3F );
      break;
      
    case PD:  // PD - pseudo port (70-101, has 26 bits on P2 and 6 bits on P3)
      pv = platform_pio_get_port_reg_addr( 2, reg );
      *pv = ( *pv & ~0xFFFFFFC0 ) | ( val << 6 );
      pv = platform_pio_get_port_reg_addr( 3, reg );
      *pv = ( *pv & ~0x3F ) | ( val >> 26 );
      break;
    
    case PE:  // PE - pseudo port (102-109, 8 bits on P3)
      pv = platform_pio_get_port_reg_addr( 3, reg );
      *pv = ( *pv & ~0x3FC0 ) | ( ( val & 0xFF ) << 6 );   
      break;
  }
}

pio_type platform_pio_op( unsigned port, pio_type pinmask, int op )
{
  pio_type retval = 1;
  
  switch( op )
  {
    case PLATFORM_IO_PORT_SET_VALUE:    
      platform_pio_set_port_reg( port, pinmask, PIO_REG_OVR );
      break;
      
    case PLATFORM_IO_PIN_SET:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_OVR ) | pinmask, PIO_REG_OVR );
      break;
      
    case PLATFORM_IO_PIN_CLEAR:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_OVR ) & ~pinmask, PIO_REG_OVR );
      break;
      
    case PLATFORM_IO_PORT_DIR_INPUT:
      pinmask = 0xFFFFFFFF;      
    case PLATFORM_IO_PIN_DIR_INPUT:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_ODER ) & ~pinmask, PIO_REG_ODER );
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_GPER ) | pinmask, PIO_REG_GPER );
      break;
      
    case PLATFORM_IO_PORT_DIR_OUTPUT:      
      pinmask = 0xFFFFFFFF;
    case PLATFORM_IO_PIN_DIR_OUTPUT:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_ODER ) | pinmask, PIO_REG_ODER );
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_GPER ) | pinmask, PIO_REG_GPER );    
      break;      
            
    case PLATFORM_IO_PORT_GET_VALUE:
      retval = platform_pio_get_port_reg( port, pinmask == PLATFORM_IO_READ_IN_MASK ? PIO_REG_PVR : PIO_REG_OVR );
      break;
      
    case PLATFORM_IO_PIN_GET:
      retval = platform_pio_get_port_reg( port, PIO_REG_PVR ) & pinmask ? 1 : 0;
      break;
      
    case PLATFORM_IO_PIN_PULLUP:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_PUER ) | pinmask, PIO_REG_PUER );
      break;
      
    case PLATFORM_IO_PIN_NOPULL:
      platform_pio_set_port_reg( port, platform_pio_get_port_reg( port, PIO_REG_PUER ) & ~pinmask, PIO_REG_PUER );    
      break;
      
    default:
      retval = 0;
      break;
  }
  return retval;
}

// ****************************************************************************
// UART functions


static const gpio_map_t uart_pins = 
{
  // UART 0
  { AVR32_USART0_RXD_0_0_PIN, AVR32_USART0_RXD_0_0_FUNCTION },
  { AVR32_USART0_TXD_0_0_PIN, AVR32_USART0_TXD_0_0_FUNCTION },
  // UART 1
  { AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION },
  { AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION },
  // UART 2
  { AVR32_USART2_RXD_0_0_PIN, AVR32_USART2_RXD_0_0_FUNCTION },
  { AVR32_USART2_TXD_0_0_PIN, AVR32_USART2_TXD_0_0_FUNCTION },
  // UART 3
  { AVR32_USART3_RXD_0_0_PIN, AVR32_USART3_RXD_0_0_FUNCTION },
  { AVR32_USART3_TXD_0_0_PIN, AVR32_USART3_TXD_0_0_FUNCTION }
};

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{
  volatile avr32_usart_t *pusart = ( volatile avr32_usart_t* )uart_base_addr[ id ];  
  usart_options_t opts;
  
  opts.channelmode = USART_NORMAL_CHMODE;
  opts.charlength = databits;
  opts.baudrate = baud;
  
  // Set stopbits
  if( stopbits == PLATFORM_UART_STOPBITS_1 )
    opts.stopbits = USART_1_STOPBIT;
  else if( stopbits == PLATFORM_UART_STOPBITS_1_5 )
    opts.stopbits = USART_1_5_STOPBITS;
  else
    opts.stopbits = USART_2_STOPBITS;    
    
  // Set parity
  if( parity == PLATFORM_UART_PARITY_EVEN )
    opts.paritytype = USART_EVEN_PARITY;
  else if( parity == PLATFORM_UART_PARITY_ODD )
    opts.paritytype = USART_ODD_PARITY;
  else
    opts.paritytype = USART_NO_PARITY;  
    
  // Set actual interface
  gpio_enable_module(uart_pins + id * 2, 2 );
  usart_init_rs232( pusart, &opts, REQ_PBA_FREQ );  
  
  // [TODO] Return actual baud here
  return baud;
}

void platform_uart_send( unsigned id, u8 data )
{
  volatile avr32_usart_t *pusart = ( volatile avr32_usart_t* )uart_base_addr[ id ];  
  
  usart_putchar( pusart, data );
}    

int platform_s_uart_recv( unsigned id, s32 timeout )
{
  volatile avr32_usart_t *pusart = ( volatile avr32_usart_t* )uart_base_addr[ id ];  
  int temp;

  if( timeout == 0 )
  {
    if( usart_read_char( pusart, &temp ) != USART_SUCCESS )
      return -1;
    else
      return temp;
  }
  else
    return usart_getchar( pusart );
}

// ****************************************************************************
// Timer functions

static const u16 clkdivs[] = { 0xFFFF, 2, 8, 32, 128 };

// Helper: get timer clock
static u32 platform_timer_get_clock( unsigned id )
{
  volatile avr32_tc_t *tc = &AVR32_TC;
  unsigned int clksel = tc->channel[ id ].CMR.waveform.tcclks;
        
  return clksel == 0 ? 32768 : REQ_PBA_FREQ / clkdivs[ clksel ];
}

// Helper: set timer clock
static u32 platform_timer_set_clock( unsigned id, u32 clock )
{
  unsigned i, mini;
  volatile avr32_tc_t *tc = &AVR32_TC;
  volatile unsigned long *pclksel = &tc->channel[ id ].cmr;
  
  for( i = mini = 0; i < 5; i ++ )
    if( ABSDIFF( clock, i == 0 ? 32768 : REQ_PBA_FREQ / clkdivs[ i ] ) < ABSDIFF( clock, mini == 0 ? 32768 : REQ_PBA_FREQ / clkdivs[ mini ] ) )
      mini = i;
  *pclksel = ( *pclksel & ~0x07 ) | mini;
  return mini == 0 ? 32768 : REQ_PBA_FREQ / clkdivs[ mini ];
}

void platform_s_timer_delay( unsigned id, u32 delay_us )
{
  volatile avr32_tc_t *tc = &AVR32_TC;  
  u32 freq;
  timer_data_type final;
  volatile int i;
  volatile const avr32_tc_sr_t *sr = &tc->channel[ id ].SR;
      
  freq = platform_timer_get_clock( id );
  final = ( ( u64 )delay_us * freq ) / 1000000;
  if( final > 0xFFFF )
    final = 0xFFFF;
  tc_start( tc, id );
  i = sr->covfs;
  for( i = 0; i < 200; i ++ );
  while( ( tc_read_tc( tc, id ) < final ) && !sr->covfs );  
}

u32 platform_s_timer_op( unsigned id, int op, u32 data )
{
  u32 res = 0;
  volatile int i;
  volatile avr32_tc_t *tc = &AVR32_TC;    
  
  switch( op )
  {
    case PLATFORM_TIMER_OP_START:
      res = 0;
      tc_start( tc, id );
      for( i = 0; i < 200; i ++ );      
      break;
      
    case PLATFORM_TIMER_OP_READ:
      res = tc_read_tc( tc, id );
      break;
      
    case PLATFORM_TIMER_OP_GET_MAX_DELAY:
      res = platform_timer_get_diff_us( id, 0, 0xFFFF );
      break;
      
    case PLATFORM_TIMER_OP_GET_MIN_DELAY:
      res = platform_timer_get_diff_us( id, 0, 1 );
      break;      
      
    case PLATFORM_TIMER_OP_SET_CLOCK:
      res = platform_timer_set_clock( id, data );
      break;
      
    case PLATFORM_TIMER_OP_GET_CLOCK:
      res = platform_timer_get_clock( id );
      break;
  }
  return res;
}

// ****************************************************************************
// CPU functions

void platform_cpu_enable_interrupts()
{
  Enable_global_interrupt();
}

void platform_cpu_disable_interrupts()
{
  Disable_global_interrupt();
}

