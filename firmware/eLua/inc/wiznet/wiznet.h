// Remote networking implementation

#ifndef __WIZNET_H__
#define __WIZNET_H__

#include "type.h"

// Operation IDs
enum
{
  WIZ_OP_SOCKET,
  WIZ_OP_FIRST = WIZ_OP_SOCKET,
  WIZ_OP_WRITE,
  WIZ_OP_READ,
  WIZ_OP_SENDTO,
  WIZ_OP_RECVFROM,
  WIZ_OP_LOOKUP,
  WIZ_OP_PING,
  WIZ_OP_CLOSE,
  WIZ_OP_CONNECT,
  WIZ_OP_HTTP_REQUEST,
  WIZ_OP_APP_INIT,
  WIZ_OP_APP_SET_PAGE,
  WIZ_OP_APP_SET_ARG,
  WIZ_OP_APP_GET_CGI,
  WIZ_OP_APP_END_PAGE,
  WIZ_OP_LAST = WIZ_OP_APP_END_PAGE
};

// Socket types
#define WIZ_SOCK_NONE       0
#define WIZ_SOCK_TCP        1
#define WIZ_SOCK_UDP        2
#define WIZ_SOCK_IPRAW      3

// Character escaping
#define WIZ_ESC_CHAR        0xC0
#define WIZ_EREQ_CHAR       0xF0
#define WIZ_ESC_MASK        0x20

#define WIZ_HAS_CGI_CHAR    0xF0

// Protocol packet size
// WARNING! WIZNET_BUFFER_SIZE must be the same as WIZ_BUFFER_SIZE in platform_conf.h!
#define WIZNET_BUFFER_SIZE            512
#define WIZ_SENDTO_REQUEST_EXTRA      ( ELUARPC_START_OFFSET + ELUARPC_START_SIZE + ELUARPC_OP_ID_SIZE + ELUARPC_U8_SIZE + ELUARPC_U32_SIZE + ELUARPC_U16_SIZE + ELUARPC_SMALL_PTR_HEADER_SIZE + ELUARPC_U16_SIZE + ELUARPC_END_SIZE )
#define WIZNET_PACKET_SIZE            ( WIZNET_BUFFER_SIZE - WIZ_SENDTO_REQUEST_EXTRA )
#define WIZNET_APP_PAGE_BUF_OFFSET    ( ELUARPC_START_OFFSET + ELUARPC_START_SIZE + ELUARPC_OP_ID_SIZE + ELUARPC_SMALL_PTR_HEADER_SIZE )

// Operation timeouts
#define WIZ_INF_TIMEOUT     0xFFFF
#define WIZ_NO_TIMEOUT      0x0000

// Error messages for http_req
#define WIZ_HTTP_REQ_OK               0
#define WIZ_HTTP_REQ_ERR_LOOKUP       1
#define WIZ_HTTP_REQ_ERR_URL          2
#define WIZ_HTTP_REQ_ERR_CONNECT      3
#define WIZ_HTTP_REQ_ERR_SEND         4
#define WIZ_HTTP_REQ_ERR_TOO_LONG     5

// Function: s8 socket( u8 sockno, u8 type, u16 port );
void wiznet_socket_write_response( u8 *p, s8 result );
int wiznet_socket_read_response( const u8 *p, s8 *presult );
void wiznet_socket_write_request( u8 *p, u8 type, u16 port );
int wiznet_socket_read_request( const u8 *p, u8 *ptype, u16 *pport );

// Function: u16 write( u8 sockno, const void *buf, u16 count )
void wiznet_write_write_response( u8 *p, u16 result );
int wiznet_write_read_response( const u8 *p, u16 *presult );
void wiznet_write_write_request( u8 *p, u8 sockno, const void *buf, u16 count );
int wiznet_write_read_request( const u8 *p, u8 *psockno, const void **pbuf, u16 *pcount );

// Function: u16 read( u8 sockno, void *pbuf, u16 count, u16 timeout, u8 type )
void wiznet_read_write_response( u8 *p, u16 readbytes );
int wiznet_read_read_response( const u8 *p, u8 **ppdata, u16 *preadbytes );
void wiznet_read_write_request( u8 *p, u8 sockno, u16 count, u16 timeout );
int wiznet_read_read_request( const u8 *p, u8 *psockno, u16 *pcount, u16 *ptimeout );

// Function: u16 sendto( u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout )
void wiznet_sendto_write_response( u8 *p, u16 result );
int wiznet_sendto_read_response( const u8 *p, u16 *presult );
void wiznet_sendto_write_request( u8 *p, u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout );
int wiznet_sendto_read_request( const u8 *p, u8 *psockno, u32 *pdest, u16 *pport, const void **pbuf, u16 *pcount, u16 *ptimeout );

// Function: u16 recvfrom( u8 sockno, u32 *pfrom, u16 *pport, void *buf, u16 count, u16 timeout )
void wiznet_recvfrom_write_response( u8 *p, u32 from, u16 port, u16 readbytes );
int wiznet_recvfrom_read_response( const u8 *p, u32 *pfrom, u16 *pport, u8 **ppdata, u16 *preadbytes );
void wiznet_recvfrom_write_request( u8 *p, u8 sockno, u16 count, u16 timeout );
int wiznet_recvfrom_read_request( const u8 *p, u8 *psockno, u16 *pcount, u16 *ptimeout );

// Function: u8 close( u8 sockno )
void wiznet_close_write_response( u8 *p, u8 result );
int wiznet_close_read_response( const u8 *p, u8 *presult );
void wiznet_close_write_request( u8 *p, u8 sockno );
int wiznet_close_read_request( const u8 *p, u8 *psockno );

// Function: u32 lookup( const char* hostname, u16 timeout )
void wiznet_lookup_write_response( u8 *p, u32 result );
int wiznet_lookup_read_response( const u8 *p, u32 *presult );
void wiznet_lookup_write_request( u8 *p, const char *hostname, u16 timeout );
int wiznet_lookup_read_request( const u8* p, const char **phostname, u16 *ptimeout );

// Function: u8 ping( u32 ip, u8 count, u16 timeout )
void wiznet_ping_write_response( u8 *p, u8 result );
int wiznet_ping_read_response( const u8 *p, u8 *presult );
void wiznet_ping_write_request( u8 *p, u32 ip, u8 count, u16 timeout );
int wiznet_ping_read_request( const u8* p, u32 *pip, u8 *pcount, u16 *ptimeout );

// Function: u8 connect( u8 sockno, u32 destip, u16 port )
void wiznet_connect_write_response( u8 *p, u8 result );
int wiznet_connect_read_response( const u8 *p, u8 *presult );
void wiznet_connect_write_request( u8 *p, u8 sockno, u32 destip, u16 port );
int wiznet_connect_read_request( const u8 *p, u8 *psockno, u32 *pdestip, u16 *pport );

// Function: u8 http_request( u8 sockno, const char* url )
void wiznet_http_request_write_response( u8 *p, u8 result );
int wiznet_http_request_read_response( const u8 *p, u8 *presult );
void wiznet_http_request_write_request( u8 *p, u8 sockno, const char* url, u16 timeout );
int wiznet_http_request_read_request( const u8 *p, u8 *psockno, const char **purl, u16 *ptimeout );  

// Function u8 app_init( const char* appname );
void wiznet_app_init_write_response( u8 *p, u8 result );
int wiznet_app_init_read_response( const u8 *p, u8 *presult );
void wiznet_app_init_write_request( u8 *p, const char* appname );
int wiznet_app_init_read_request( const u8*p, const char **pappname );

// Function: u8 app_set_page( const char* ppage )
void wiznet_app_set_page_write_response( u8 *p, u8 result );
int wiznet_app_set_page_read_response( const u8 *p, u8 *presult );
void wiznet_app_set_page_write_request( u8 *p, const char *apppage, u16 len );
int wiznet_app_set_page_read_request( const u8 *p, const char **papppage );

// Function: u8 app_set_arg( const char* parg )
void wiznet_app_set_arg_write_response( u8 *p, u8 result );
int wiznet_app_set_arg_read_response( const u8 *p, u8 *presult );
void wiznet_app_set_arg_write_request( u8 *p, const char *arg );
int wiznet_app_set_arg_read_request( const u8 *p, const char **parg );

// Function: char* app_get_cgi()
void wiznet_app_get_cgi_write_response( u8 *p, u8 result, char *cgidata );
int wiznet_app_get_cgi_read_response( const u8 *p, u8 *presult, char **pcgidata );
void wiznet_app_get_cgi_write_request( u8 *p );
int wiznet_app_get_cgi_read_request( const u8* p );

// Function: u8 app_end_page()
void wiznet_app_end_page_write_response( u8 *p, u8 result );
int wiznet_app_end_page_read_response( u8 *p, u8 *presult );
void wiznet_app_end_page_write_request( u8 *p );
int wiznet_app_end_page_read_request( const u8 *p );

#endif
