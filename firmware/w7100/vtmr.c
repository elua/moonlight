// Virtual timers

#include "vtmr.h"
#include "w7100.h"
#include <stdio.h>
#include "lcd.h"

static VTMR idata vtmr_timers[ VTMR_MAX_TIMERS ];

// Timer0 interrupt handler
void timer0_int_handler() interrupt 1	using 1	
{
  u8 idata i;

  TR0 = 0;
  // Iterate through all running counters
  for( i = 0; i < VTMR_MAX_TIMERS; i ++ )
  {
    if( vtmr_timers[ i ].flags & VTMR_F_ENABLED )
    {
      vtmr_timers[ i ].counter ++;
      if( vtmr_timers[ i ].counter >= vtmr_timers[ i ].limit )
      {
        vtmr_timers[ i ].counter = 0;
        if( vtmr_timers[ i ].flags & VTMR_F_ONESHOT )
        {
          vtmr_timers[ i ].flags &= ( u8 )~VTMR_F_ENABLED;
          vtmr_timers[ i ].flags |= VTMR_F_EXPIRED;
        }
      }
    }
  }
  TH0 = TIMER0_BASE_CNTH;  
  TL0 = TIMER0_BASE_CNTL;
  TR0 = 1;
}

// Timer initialization
void vtmr_init()
{
  unsigned xdata i;

  // Setup timer data structures
  for( i = 0; i < VTMR_MAX_TIMERS; i ++ )
  {
    vtmr_timers[ i ].counter = vtmr_timers[ i ].limit = 0;
    vtmr_timers[ i ].flags = 0;
  }

  // Setup the fixed delay timer
  vtmr_setup( VTMR_DELAY, VTMR_TYPE_ONESHOT, 1000 );

  // Setup actual timer
  // Stop interrupts
  EA = 0;
  // Stop timer
  TR0 = 0;
  // Clear overflow flag	
  TF0 = 0;
  // 16-bit mode
  TMOD = ( TMOD & ~0x03 ) | 0x01;
  // Initial timer value
  TH0 = TIMER0_BASE_CNTH;
  TL0 = TIMER0_BASE_CNTL;
  // Priority: 0
  PT0 = 0;
  // Enable timer interrupt
  ET0 = 1;
  // Enable all interrupts
  EA = 1;
  // Start timer
  TR0 = 1;
}

void vtmr_set_timeout( unsigned id, u16 ms )
{
  ET0 = 0;
  if( ms == VTMR_NO_TIMEOUT || ms == VTMR_INF_TIMEOUT )
    vtmr_timers[ id ].limit = ms;
  else
  {
    if( ( vtmr_timers[ id ].limit = ( ms >> VTMR_BASE_PERIOD_SHIFT ) ) == 0 )
      vtmr_timers[ id ].limit = 1;
  }
  vtmr_timers[ id ].counter = 0;
  vtmr_timers[ id ].flags &= ~VTMR_F_EXPIRED;
  ET0 = 1;
}

void vtmr_setup( unsigned id, int type, u16 ms )
{
  ET0 = 0;
  if( ms == VTMR_NO_TIMEOUT || ms == VTMR_INF_TIMEOUT )
    vtmr_timers[ id ].limit = ms;
  else
  {
    if( ( vtmr_timers[ id ].limit = ( ms >> VTMR_BASE_PERIOD_SHIFT ) ) == 0 )
      vtmr_timers[ id ].limit = 1;
  }
  vtmr_timers[ id ].counter = 0;
  vtmr_timers[ id ].flags = type == VTMR_TYPE_ONESHOT ? VTMR_F_ONESHOT : 0;
  ET0 = 1;  
}

void vtmr_enable( unsigned id )
{
  ET0 = 0;
  if( vtmr_timers[ id ].limit != VTMR_NO_TIMEOUT && vtmr_timers[ id ].limit != VTMR_INF_TIMEOUT )
    vtmr_timers[ id ].flags |= VTMR_F_ENABLED;
  else 
  {
    vtmr_timers[ id ].flags &= ~VTMR_F_ENABLED;    
    if( vtmr_timers[ id ].limit == VTMR_NO_TIMEOUT )
      vtmr_timers[ id ].flags |= VTMR_F_EXPIRED;
    else
      vtmr_timers[ id ].flags &= ~VTMR_F_EXPIRED;    
  }
  ET0 = 1;
}

void vtmr_disable( unsigned id )
{
  ET0 = 0;
  vtmr_timers[ id ].flags &= ~VTMR_F_ENABLED;
  ET0 = 1;
}

vtmr_counter_t vtmr_read( unsigned id )
{
  vtmr_counter_t xdata tmp;

  ET0 = 0;
  tmp = vtmr_timers[ id ].counter;
  ET0 = 1;
  return tmp;
}

int vtmr_is_expired( unsigned id )
{
  return ( vtmr_timers[ id ].flags & VTMR_F_EXPIRED ) != 0;
}

void vtmr_delay( u16 ms )
{
  vtmr_set_timeout( VTMR_DELAY, ms );
  vtmr_enable( VTMR_DELAY );
  while( !vtmr_is_expired( VTMR_DELAY ) );
}
  