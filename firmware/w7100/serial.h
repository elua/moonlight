#ifndef __SERIAL_H
#define __SERIAL_H

#include "type.h"

#define SER_RX_BUF_SIZE     4096
#define SER_RX_BUF_MASK     4095

#define SER_INF_TIMEOUT     0xFFFF
#define SER_NO_TIMEOUT      0x0000

// Serial receive modes
#define SER_MODE_RPC        0
#define SER_MODE_FWUPD      1

void ser_init();
void ser_write_byte( u8 c );
int ser_read_byte( u16 timeout );
u16 ser_read( u8 xdata *pbuf, u16 count, u16 timeout );
u16 ser_write( const u8 xdata *pbuf, u16 count );
u8 ser_got_rpc_request();
void ser_set_mode( u8 mode );
 
// Serial access functions (to be implemented by each platform)
ser_handler ser_open( const char *sername );
void ser_close( ser_handler id );
int ser_setup( ser_handler id, u32 baud, int databits, int parity, int stopbits );
//u32 ser_read( ser_handler id, u8* dest, u32 maxsize );
//int ser_read_byte( ser_handler id );
//u32 ser_write( ser_handler id, const u8 *src, u32 size );
//u32 ser_write_byte( ser_handler id, u8 fdata );
void ser_set_timeout_ms( ser_handler id, u32 timeout );

#endif  //  end __SERIAL
