// Common platform functions

#ifndef __COMMON_H__
#define __COMMON_H__

// Virtual timers data
#define VTMR_FIRST_ID           ( 32 )
#define VTMR_GET_ID( x )        ( ( x ) - VTMR_FIRST_ID )
#define TIMER_IS_VIRTUAL( x )   ( ( VTMR_NUM_TIMERS > 0 ) && ( ( x ) >= VTMR_FIRST_ID ) && ( ( x ) < VTMR_NUM_TIMERS + VTMR_FIRST_ID ) )

// Functions exported by the common platform layer
void cmn_platform_init();
void cmn_virtual_timer_cb();
void cmn_rx_handler( int usart_id, u8 data );

unsigned int intlog2( unsigned int v );

#endif // #ifndef __COMMON_H__
