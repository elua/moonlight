// Serial multiplexer definitions 

#ifndef __SERMUX_H__
#define __SERMUX_H__

#define SERVICE_ID_FIRST  0xD0
#define SERVICE_ID_LAST   0xD7
#define SERVICE_MAX ( SERVICE_ID_LAST - SERVICE_ID_FIRST + 1 )

#define ESCAPE_CHAR       0xC0
#define FORCE_SID_CHAR    0xFF

#define ESCAPE_XOR_MASK   0x20
#define ESC_MASK          0x100

#endif

