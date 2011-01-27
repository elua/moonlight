// A simple "bit streamer"

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include "type.h"

typedef u8 bs_data_type;
#define bs_data_size 8  

typedef struct 
{
  bs_data_type *start;
  u32 capacity, position;
  u8 bitpos;  
} BS_DESC;

BS_DESC* bs_init_write( u32 capacity );
int bs_read_bit( BS_DESC* pbs );
void bs_write_bit( BS_DESC* pbs, int val );
void bs_destroy( BS_DESC *pbs );

#endif
