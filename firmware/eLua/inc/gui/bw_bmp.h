// 2 color (black and white) bitmap functions

#ifndef __BW_BMP_H__
#define __BW_BMP_H__

#include "type.h"

typedef struct 
{
  u16 w, h;
  const u32* data;
} BW_BMP;

#endif
