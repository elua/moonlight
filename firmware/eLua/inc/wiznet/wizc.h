// Wiznet RPC client

#ifndef __WIZC_H__
#define __WIZC_H__

#include "type.h"

// Error codes
#define WIZC_ERR   0
#define WIZC_OK    1

// Public functions - networking
void wizc_setup( u8 *pbuf );
s8 wizc_socket( u8 type, u16 port );
s16 wizc_write( u8 sockno, const void *buf, u16 count );
s16 wizc_read( u8 sockno, u8 **pbuf, u16 count, u16 timeout );
s16 wizc_sendto( u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout );
s16 wizc_recvfrom( u8 sockno, u32 *pfrom, u16 *pport, u8** pbuf, u16 count, u16 timeout );
s8 wizc_close( u8 sockno );
u32 wizc_lookup( const char* hostname, u16 timeout );
s8 wizc_ping( u32 ip, u8 count, u16 timeout );
s8 wizc_connect( u8 sockno, u32 destip, u16 port );
s8 wizc_http_request( u8 sockno, const char* url, u16 timeout );

// Application web
u8 wizc_app_init( const char *pname );
u8 wizc_app_set_page( const char* ppage, u16 len );
u8 wizc_app_set_arg( const char* parg );
char* wizc_app_get_cgi();
u8 wizc_app_end_page();
void wizc_set_cgi_flag( int flag );
int wizc_get_cgi_flag();

#endif // #ifdef __WIZC_H__
