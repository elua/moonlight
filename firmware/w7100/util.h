#ifndef __UTILS_H
#define __UTILS_H
#include "types.h"

#define LOW(uint16)   ((uint8)uint16)
#define HIGH(uint16)  ((uint8)(uint16>>8))

unsigned char gethex(uint8 b0, uint8 b1);
unsigned char gethex3(uint8 b0, uint8 b1,uint8 b2);

#endif
