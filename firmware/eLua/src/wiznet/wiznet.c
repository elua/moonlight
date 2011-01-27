// Remote file system implementation

#include <string.h>
#include <stdarg.h>
#include "type.h"
#include "eluarpc.h"
#include "rtype.h"
#include "wiznet.h"

// *****************************************************************************
// Operation: socket
// socket: s8 socket( u8 type, u16 port )

void wiznet_socket_write_response( u8 *p, s8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_SOCKET, ( int )result );
}

int wiznet_socket_read_response( const u8 *p, s8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_SOCKET, presult );
}

void wiznet_socket_write_request( u8 *p, u8 type, u16 port )
{
  eluarpc_gen_write( p, "och", ( int )WIZ_OP_SOCKET, type, port );
}

int wiznet_socket_read_request( const u8 *p, u8 *ptype, u16 *pport )
{
  return eluarpc_gen_read( p, "och", ( int )WIZ_OP_SOCKET, ptype, pport );
}

// *****************************************************************************
// Operation: write
// write: u16 write( u8 sockno, const void *buf, u16 count )

void wiznet_write_write_response( u8 *p, u16 result )
{
  eluarpc_gen_write( p, "rh", ( int )WIZ_OP_WRITE, result );
}

int wiznet_write_read_response( const u8 *p, u16 *presult )
{
  return eluarpc_gen_read( p, "rh", ( int )WIZ_OP_WRITE, presult );
}

void wiznet_write_write_request( u8 *p, u8 sockno, const void *buf, u16 count )
{
  eluarpc_gen_write( p, "ocP", ( int )WIZ_OP_WRITE, sockno, buf, count );
}

int wiznet_write_read_request( const u8 *p, u8 *psockno, const void **pbuf, u16 *pcount )
{
  return eluarpc_gen_read( p, "ocP", ( int )WIZ_OP_WRITE, psockno, pbuf, pcount );
}

// *****************************************************************************
// Operation: read
// read: u16 read( u8 sockno, void *buf, u16 count, u16 timeout, u8 type )

void wiznet_read_write_response( u8 *p, u16 readbytes )
{
  eluarpc_gen_write( p, "rP", ( int )WIZ_OP_READ, NULL, readbytes );  
}

int wiznet_read_read_response( const u8 *p, u8 **ppdata, u16 *preadbytes )
{
  return eluarpc_gen_read( p, "rP", ( int )WIZ_OP_READ, ppdata, preadbytes );  
}

void wiznet_read_write_request( u8 *p, u8 sockno, u16 count, u16 timeout )
{
  eluarpc_gen_write( p, "ochh", ( int )WIZ_OP_READ, sockno, count, timeout ); 
}

int wiznet_read_read_request( const u8 *p, u8 *psockno, u16 *pcount, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "ochh", ( int )WIZ_OP_READ, psockno, pcount, ptimeout );    
}

// *****************************************************************************
// Operation: sendto
// sendto: u16 sendto( u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout )

void wiznet_sendto_write_response( u8 *p, u16 result )
{
  eluarpc_gen_write( p, "rh", ( int )WIZ_OP_SENDTO, result );
}

int wiznet_sendto_read_response( const u8 *p, u16 *presult )
{
  return eluarpc_gen_read( p, "rh", ( int )WIZ_OP_SENDTO, presult ); 
}

void wiznet_sendto_write_request( u8 *p, u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout )
{
  eluarpc_gen_write( p, "oclhPh", ( int )WIZ_OP_SENDTO, sockno, dest, port, buf, count, timeout );
}

int wiznet_sendto_read_request( const u8 *p, u8 *psockno, u32 *pdest, u16 *pport, const void **pbuf, u16 *pcount, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "oclhPh", ( int )WIZ_OP_SENDTO, psockno, pdest, pport, pbuf, pcount, ptimeout );
}

// *****************************************************************************
// Operation: recvfrom
// recvfrom: u16 recvfrom( u8 sockno, u32 *pfrom, u16 *pport, void *buf, u16 count, u16 timeout, u8 type )

void wiznet_recvfrom_write_response( u8 *p, u32 from, u16 port, u16 readbytes )
{
  eluarpc_gen_write( p, "rPhl", ( int )WIZ_OP_RECVFROM, NULL, readbytes, port, from );  
}

int wiznet_recvfrom_read_response( const u8 *p, u32 *pfrom, u16 *pport, u8 **ppdata, u16 *preadbytes )
{
  return eluarpc_gen_read( p, "rPhl", ( int )WIZ_OP_RECVFROM, ppdata, preadbytes, pport, pfrom ); 
}

void wiznet_recvfrom_write_request( u8 *p, u8 sockno, u16 count, u16 timeout )
{
  eluarpc_gen_write( p, "ochh", ( int )WIZ_OP_RECVFROM, sockno, count, timeout ); 
}

int wiznet_recvfrom_read_request( const u8 *p, u8 *psockno, u16 *pcount, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "ochh", ( int )WIZ_OP_RECVFROM, psockno, pcount, ptimeout );   
}

// *****************************************************************************
// Operation: close
// close: u8 close( u8 sockno )

void wiznet_close_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_CLOSE, ( int )result );
}

int wiznet_close_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_CLOSE, presult );
}

void wiznet_close_write_request( u8 *p, u8 sockno )
{
  eluarpc_gen_write( p, "oc", ( int )WIZ_OP_CLOSE, sockno );
}

int wiznet_close_read_request( const u8 *p, u8 *psockno )
{
  return eluarpc_gen_read( p, "oc", ( int )WIZ_OP_CLOSE, psockno );
}

// *****************************************************************************
// Operation: lookup
// lookup: u32 lookup( const char* hostname, u16 timeout )

void wiznet_lookup_write_response( u8 *p, u32 result )
{
  eluarpc_gen_write( p, "rl", ( int )WIZ_OP_LOOKUP, result );
}

int wiznet_lookup_read_response( const u8 *p, u32 *presult )
{
  return eluarpc_gen_read( p, "rl", ( int )WIZ_OP_LOOKUP, presult );
}

void wiznet_lookup_write_request( u8 *p, const char *hostname, u16 timeout )
{
  eluarpc_gen_write( p, "oPh", ( int )WIZ_OP_LOOKUP, hostname, strlen( hostname ) + 1, timeout );    
}

int wiznet_lookup_read_request( const u8* p, const char **phostname, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "oPh", ( int )WIZ_OP_LOOKUP, phostname, NULL, ptimeout );
}

// *****************************************************************************
// Operation: ping
// ping: u8 ping( u32 ip, u8 count, u16 timeout )

void wiznet_ping_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_PING, ( int )result );
}

int wiznet_ping_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_PING, presult );
}

void wiznet_ping_write_request( u8 *p, u32 ip, u8 count, u16 timeout )
{
  eluarpc_gen_write( p, "olch", ( int )WIZ_OP_PING, ip, count, timeout );    
}

int wiznet_ping_read_request( const u8* p, u32 *pip, u8 *pcount, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "olch", ( int )WIZ_OP_PING, pip, pcount, ptimeout );
}

// *****************************************************************************
// Operation: connect
// connect: u8 connect( u8 sockno, u32 destip )

void wiznet_connect_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_CONNECT, ( int )result );
}

int wiznet_connect_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_CONNECT, presult );
}

void wiznet_connect_write_request( u8 *p, u8 sockno, u32 destip, u16 port )
{
  eluarpc_gen_write( p, "oclh", ( int )WIZ_OP_CONNECT, sockno, destip, port );
}

int wiznet_connect_read_request( const u8 *p, u8 *psockno, u32 *pdestip, u16 *pport )
{
  return eluarpc_gen_read( p, "oclh", ( int )WIZ_OP_CONNECT, psockno, pdestip, pport );
}

// *****************************************************************************
// Operation: http_request
// http_request: u8 http_request( u8 sockno, const char* url )

void wiznet_http_request_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_HTTP_REQUEST, ( int )result );
}

int wiznet_http_request_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_HTTP_REQUEST, presult );
}

void wiznet_http_request_write_request( u8 *p, u8 sockno, const char* url, u16 timeout )
{
  eluarpc_gen_write( p, "ocPh", ( int )WIZ_OP_HTTP_REQUEST, sockno, url, strlen( url ) + 1, timeout );
}

int wiznet_http_request_read_request( const u8 *p, u8 *psockno, const char **purl, u16 *ptimeout )
{
  return eluarpc_gen_read( p, "ocPh", ( int )WIZ_OP_HTTP_REQUEST, psockno, purl, NULL, ptimeout );
}

// *****************************************************************************
// Operation: app_init
// app_init: u8 app_init( const char* appname )

void wiznet_app_init_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_APP_INIT, ( int )result );
}

int wiznet_app_init_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_APP_INIT, presult );
}

void wiznet_app_init_write_request( u8 *p, const char* appname )
{
  eluarpc_gen_write( p, "oP", ( int )WIZ_OP_APP_INIT, appname, strlen( appname ) + 1 );
}

int wiznet_app_init_read_request( const u8 *p, const char **pappname )
{
  return eluarpc_gen_read( p, "oP", ( int )WIZ_OP_APP_INIT, pappname, NULL );
}

// *****************************************************************************
// Operation: app_set_page
// app_set_page: u8 app_set_page( const char* ppage )

void wiznet_app_set_page_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_APP_SET_PAGE, ( int )result );
}

int wiznet_app_set_page_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_APP_SET_PAGE, presult );
}

void wiznet_app_set_page_write_request( u8 *p, const char *apppage, u16 len )
{
  eluarpc_gen_write( p, "oP", ( int )WIZ_OP_APP_SET_PAGE, apppage, len );
}

int wiznet_app_set_page_read_request( const u8 *p, const char **apppage )
{
  return eluarpc_gen_read( p, "oP", ( int )WIZ_OP_APP_SET_PAGE, apppage, NULL );
}

// *****************************************************************************
// Operation: app_set_arg
// app_set_arg: u8 app_set_arg( const char* parg )

void wiznet_app_set_arg_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_APP_SET_ARG, ( int )result );
}

int wiznet_app_set_arg_read_response( const u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_APP_SET_ARG, presult );
}

void wiznet_app_set_arg_write_request( u8 *p, const char *arg )
{
  eluarpc_gen_write( p, "oP", ( int )WIZ_OP_APP_SET_ARG, arg, strlen( arg ) + 1 );
}

int wiznet_app_set_arg_read_request( const u8 *p, const char **parg )
{
  return eluarpc_gen_read( p, "oP", ( int )WIZ_OP_APP_SET_ARG, parg, NULL );
}

// *****************************************************************************
// Operation: app_get_cgi
// app_get_cgi: char* app_get_cgi()

void wiznet_app_get_cgi_write_response( u8 *p, u8 result, char *cgidata )
{
  eluarpc_gen_write( p, "rcP", ( int )WIZ_OP_APP_GET_CGI, ( int )result, cgidata, strlen( cgidata ) + 1 );
}

int wiznet_app_get_cgi_read_response( const u8 *p, u8 *presult, char **pcgidata )
{
  return eluarpc_gen_read( p, "rcP", ( int )WIZ_OP_APP_GET_CGI, presult, pcgidata, NULL ); 
}

void wiznet_app_get_cgi_write_request( u8 *p )
{
  eluarpc_gen_write( p, "o", ( int )WIZ_OP_APP_GET_CGI );
}

int wiznet_app_get_cgi_read_request( const u8* p )
{
  return eluarpc_gen_read( p, "o", ( int )WIZ_OP_APP_GET_CGI );
}

// *****************************************************************************
// Operation: app_end_page
// app_end_page: u8 app_end_page()

void wiznet_app_end_page_write_response( u8 *p, u8 result )
{
  eluarpc_gen_write( p, "rc", ( int )WIZ_OP_APP_END_PAGE, ( int )result );
}

int wiznet_app_end_page_read_response( u8 *p, u8 *presult )
{
  return eluarpc_gen_read( p, "rc", ( int )WIZ_OP_APP_END_PAGE, presult );
}

void wiznet_app_end_page_write_request( u8 *p )
{
  eluarpc_gen_write( p, "o", ( int )WIZ_OP_APP_END_PAGE );
}

int wiznet_app_end_page_read_request( const u8 *p )
{
  return eluarpc_gen_read( p, "o", ( int )WIZ_OP_APP_END_PAGE );
}
