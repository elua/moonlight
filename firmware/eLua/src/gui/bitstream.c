// A simple "bit streamer"

#include "bitstream.h"
#include "type.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

BS_DESC* bs_init( u32 capacity )
{
  BS_DESC *d;
  
  if( ( d = malloc( sizeof( BS_DESC ) ) ) == NULL )
    return NULL;
  d->capacity = capacity;
  capacity = ( capacity / bs_data_size ) + ( ( capacity & ( bs_data_size - 1 ) ) ? 1 : 0 );
  if( ( d->start = malloc( capacity ) ) == NULL )
  {
    free( d );
    return NULL;
  }
  memset( d->start, 0, capacity );
  d->position = d->bitpos = 0;
  return d; 
}

static void bs_advance( BS_DESC *pbs )
{
  pbs->bitpos ++;
  if( pbs->bitpos == bs_data_size )
  {
    pbs->bitpos = 0;
    pbs->position ++;
  }
}

int bs_read_bit( BS_DESC* pbs )
{
  int bit = pbs->start[ pbs->position ] & ( 1 << pbs->bitpos ) ? 1 : 0;
  
  bs_advance( pbs );
  return bit;
}

void bs_write_bit( BS_DESC* pbs, int val )
{
  u32 mask = 1 << pbs->bitpos;
  
  if( val )
    pbs->start[ pbs->position ] |= mask;
  else
    pbs->start[ pbs->position ] &= ~mask;
  bs_advance( pbs );
}

void bs_destroy( BS_DESC *pbs )
{
  free( pbs->start );
  free( pbs );
}
