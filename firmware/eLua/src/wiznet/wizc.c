// Wiznet RPC client

#include "wizc.h"
#include "wiznet.h"
#include "platform.h"
#include "platform_conf.h"
#include "eluarpc.h"
#include "buf.h"
#include <string.h>
#include <stdio.h>

// ****************************************************************************
// Client local data

static u8 *wizc_buffer;
static int wizc_cgi_flag;

// ****************************************************************************
// Client helpers

// Send function (with escaping)
static u32 wizc_send( const u8 *p, u32 size )
{
  u32 i;
  
  // Send data, escaping the proper chars
  for( i = 0; i < size; i ++ )
    if( p[ i ] == WIZ_ESC_CHAR || p[ i ] == WIZ_EREQ_CHAR )
    {
      platform_uart_send( WIZ_UART_ID, WIZ_ESC_CHAR );
      platform_uart_send( WIZ_UART_ID, p[ i ] ^ WIZ_ESC_MASK );
    }
    else
      platform_uart_send( WIZ_UART_ID, p[ i ] );
  // Finally, send the "end of request" char
  platform_uart_send( WIZ_UART_ID, WIZ_EREQ_CHAR );
  return size;   
}

// Recv function
static u32 wizc_recv( u8 *p, u32 size )
{
  u32 cnt = 0;
  int data;

  while( size )
  {
    if( ( data = platform_uart_recv( WIZ_UART_ID, WIZ_TIMER_ID, WIZ_TIMEOUT ) ) == -1 )
      break;
    *p ++ = ( u8 )data;
    cnt ++;
    size --;
  }
  return cnt;
}

static int wizc_send_request()
{
  u16 temp16;

  // Send request
  if( eluarpc_get_packet_size( wizc_buffer, &temp16 ) == ELUARPC_ERR )
  {
    printf( "ERROR wizc_send_request(1): %u!\n", ( unsigned )temp16 );
    return WIZC_ERR;
  }
  if( wizc_send( wizc_buffer, temp16 ) != temp16 )
  {
    printf( "ERROR wizc_send_request(2): %u!\n", ( unsigned )temp16 );  
    return WIZC_ERR;
  }
  return WIZC_OK;
}

static int wizch_send_request_read_response()
{
  u16 temp16, t;
  
  if( wizc_send_request() == WIZC_OK )
  {
    // Get response
    // First the length, then the rest of the data
    if( ( t = wizc_recv( wizc_buffer, ELUARPC_START_OFFSET ) ) != ELUARPC_START_OFFSET )
    {
      printf( "ERROR wizch_send_request_read_response(1) %u %u\n", ( unsigned )t, ( unsigned )ELUARPC_START_OFFSET );  
      return WIZC_ERR;
    }
    if( eluarpc_get_packet_size( wizc_buffer, &temp16 ) == ELUARPC_ERR )
    {
      printf( "ERROR wizch_send_request_read_response(2) %u\n", ( unsigned )temp16 );    
      return WIZC_ERR;
    }
    if( ( t = wizc_recv( wizc_buffer + ELUARPC_START_OFFSET, temp16 - ELUARPC_START_OFFSET ) ) != temp16 - ELUARPC_START_OFFSET )
    {
      printf( "ERROR wizch_send_request_read_response(3) %u %u\n", ( unsigned )t, ( unsigned )( temp16 = ELUARPC_START_OFFSET ) );    
      return WIZC_ERR;
    }
    return WIZC_OK;
  }
  else
  {
    printf( "ERROR wizch_send_request_read_response(4)\n" );  
    return WIZC_ERR;
  }
}

// *****************************************************************************
// Client public interface

void wizc_setup( u8 *pbuf )
{
  if( buf_get_size( BUF_ID_UART, WIZ_UART_ID ) == 0 )
  {
    platform_uart_setup( WIZ_UART_ID, WIZ_UART_SPEED, 8, PLATFORM_UART_PARITY_NONE, PLATFORM_UART_STOPBITS_1 );
    platform_uart_set_buffer( WIZ_UART_ID, WIZ_BUFFER_SIZE );
  }
  wizc_buffer = pbuf; 
  wizc_cgi_flag = 0; 
}

s8 wizc_socket( u8 type, u16 port )
{
  s8 s;

  // Make the request
  wiznet_socket_write_request( wizc_buffer, type, port );

  // Send the request / get the respone
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;

  // Interpret the response
  if( wiznet_socket_read_response( wizc_buffer, &s ) == ELUARPC_ERR )
    return -1;
     
  return s;
}

s16 wizc_write( u8 sockno, const void *buf, u16 count )
{
  // Make the request
  wiznet_write_write_request( wizc_buffer, sockno, buf, count );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
  
  // Interpret the response
  if( wiznet_write_read_response( wizc_buffer, &count) == ELUARPC_ERR )
    return -1;
    
  return ( s16 )count;
}

s16 wizc_read( u8 sockno, u8 **pbuf, u16 count, u16 timeout )
{
  // Make the request
  wiznet_read_write_request( wizc_buffer, sockno, count, timeout );

  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;

  // Interpret the response
  if( wiznet_read_read_response( wizc_buffer, pbuf, &count ) == ELUARPC_ERR )
    return -1;
    
  return ( s16 )count;
}

s16 wizc_sendto( u8 sockno, u32 dest, u16 port, const void *buf, u16 count, u16 timeout )
{
  // Make the request
  wiznet_sendto_write_request( wizc_buffer, sockno, dest, port, buf, count, timeout );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
  
  // Interpret the response
  if( wiznet_sendto_read_response( wizc_buffer, &count) == ELUARPC_ERR )
    return -1;
    
  return ( s16 )count;
}

s16 wizc_recvfrom( u8 sockno, u32 *pfrom, u16 *pport, u8** pbuf, u16 count, u16 timeout )
{
  // Make the request
  wiznet_recvfrom_write_request( wizc_buffer, sockno, count, timeout );

  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;

  // Interpret the response
  if( wiznet_recvfrom_read_response( wizc_buffer, pfrom, pport, pbuf, &count ) == ELUARPC_ERR )
    return -1;
    
  return ( s16 )count;
}

s8 wizc_close( u8 sockno )
{
  // Make the request
  wiznet_close_write_request( wizc_buffer, sockno );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
    
  // Interpret the response
  if( wiznet_close_read_response( wizc_buffer, &sockno ) == ELUARPC_ERR )
    return -1;
    
  return ( s8 )sockno;  
}

u32 wizc_lookup( const char* hostname, u16 timeout )
{
  u32 temp;
  
  // Make the request
  wiznet_lookup_write_request( wizc_buffer, hostname, timeout );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return 0;
    
  // Interpret the response
  if( wiznet_lookup_read_response( wizc_buffer, &temp ) == ELUARPC_ERR )
    return 0;
    
  return temp;  
}

s8 wizc_ping( u32 ip, u8 count, u16 timeout )
{
  // Make the request
  wiznet_ping_write_request( wizc_buffer, ip, count, timeout );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
    
  // Interpret the response
  if( wiznet_ping_read_response( wizc_buffer, &count ) == ELUARPC_ERR )
    return -1;
    
  return ( s16 )count;  
}

s8 wizc_connect( u8 sockno, u32 destip, u16 port )
{
  // Make the request
  wiznet_connect_write_request( wizc_buffer, sockno, destip, port );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
    
  // Interpret the response
  if( wiznet_connect_read_response( wizc_buffer, &sockno ) == ELUARPC_ERR )
    return -1;
    
  return ( s8 )sockno;  
}

s8 wizc_http_request( u8 sockno, const char* url, u16 timeout )
{  
  // Make the request
  wiznet_http_request_write_request( wizc_buffer, sockno, url, timeout );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return -1;
    
  // Interpret the response
  if( wiznet_http_request_read_response( wizc_buffer, &sockno ) == ELUARPC_ERR )
    return -1;
    
  return ( s8 )sockno;
}

// *****************************************************************************
// Application web

u8 wizc_app_init( const char *pname )
{
  u8 temp;
  
  // Make the request
  wiznet_app_init_write_request( wizc_buffer, pname );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return 0;
    
  // Interpret the response
  if( wiznet_app_init_read_response( wizc_buffer, &temp ) == ELUARPC_ERR )
    return 0;
  
  return temp;
}

u8 wizc_app_set_page( const char* ppage, u16 len )
{
  u8 temp;
  
  // Make the request
  wiznet_app_set_page_write_request( wizc_buffer, ppage, len );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return 0;
    
  // Interpret the response
  if( wiznet_app_set_page_read_response( wizc_buffer, &temp ) == ELUARPC_ERR )
    return 0;
  
  return temp;
}

u8 wizc_app_set_arg( const char* parg )
{
  u8 temp;
  
  // Make the request
  wiznet_app_set_arg_write_request( wizc_buffer, parg );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return 0;
    
  // Interpret the response
  if( wiznet_app_set_arg_read_response( wizc_buffer, &temp ) == ELUARPC_ERR )
    return 0;
  
  return temp;
}

char* wizc_app_get_cgi()
{
  char *cgidata;
  u8 temp;
  
  // Make the request
  wiznet_app_get_cgi_write_request( wizc_buffer );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return NULL;
    
  // Interpret the response
  if( wiznet_app_get_cgi_read_response( wizc_buffer, &temp, &cgidata ) == ELUARPC_ERR )
    return NULL;

  wizc_cgi_flag = 0;    
  return temp == 0 ? NULL : cgidata;
}

u8 wizc_app_end_page()
{
  u8 temp;
  
  // Make the request
  wiznet_app_end_page_write_request( wizc_buffer );
  
  // Send the request / get the response
  if( wizch_send_request_read_response() == WIZC_ERR )
    return 0;
    
  // Interpret the response
  if( wiznet_app_end_page_read_response( wizc_buffer, &temp ) == ELUARPC_ERR )
    return 0;
    
  return temp;
}

void wizc_set_cgi_flag( int flag )
{
  wizc_cgi_flag = flag;
}

int wizc_get_cgi_flag()
{
  return wizc_cgi_flag;
}
