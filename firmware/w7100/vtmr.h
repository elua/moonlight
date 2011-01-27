// Virtual timers

#ifndef __VTMR_H__
#define __VTMR_H__

#include "type.h"
#include "machine.h"

// "No timeout" and "infinite timeout"
#define VTMR_NO_TIMEOUT         0
#define VTMR_INF_TIMEOUT        0xFFFF

// Maximum number of virtual timers
#define VTMR_MAX_TIMERS         4
// VTMR0 is always the delay timer
#define VTMR_DELAY              0
// VTMR1 is always the UART timer
#define VTMR_UART               1
// VTMR2 is always the NET timer
#define VTMR_NET                2

// VTMR flags
#define VTMR_F_ENABLED          0x01
#define VTMR_F_ONESHOT          0x02
#define VTMR_F_EXPIRED          0x80

// VTMR types
#define VTMR_TYPE_PERIODIC      0
#define VTMR_TYPE_ONESHOT       1

// Base period of the VTMR clock (ms)
#define VTMR_BASE_PERIOD_MS     8
#define VTMR_BASE_PERIOD_SHIFT  3
// Base frequency of the VTMR clock
#define VTMR_BASE_FREQ_HZ       ( 1000 / VTMR_BASE_PERIOD_MS )

// Actual Timer0 counter
#define TIMER_BASE_CLOCK_HZ     ( CPU_FREQ_HZ / 12 )
#define TIMER0_BASE_COUNT		    ( 0xFFFF - ( VTMR_BASE_PERIOD_MS * TIMER_BASE_CLOCK_HZ ) / 1000 )
#define TIMER0_BASE_CNTH        ( ( TIMER0_BASE_COUNT >> 8 ) & 0xFF )
#define TIMER0_BASE_CNTL        ( TIMER0_BASE_COUNT & 0xFF )

// Virtual timer counter type
typedef u16 vtmr_counter_t;

// Timer structure
typedef struct 
{
  volatile vtmr_counter_t counter;
  vtmr_counter_t limit;
  volatile u8 flags;
} VTMR;

// Public functions
void vtmr_init();
void vtmr_set_timeout( unsigned id, u16 ms );
void vtmr_setup( unsigned id, int type, u16 ms );
void vtmr_enable( unsigned id );
void vtmr_disable( unsigned id );
vtmr_counter_t vtmr_read( unsigned id );
void vtmr_delay( u16 ms );
int vtmr_is_expired( unsigned id );

#endif
