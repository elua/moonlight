// WizNET RPC server

#include "wizs.h"
#include "type.h"
#include "eluarpc.h"
#include "wiznet.h"
#include "serial.h"
#include "stdio.h"
#include "socket.h"
#include "w7100.h"
#include "lcd.h"
#include "vtmr.h"
#include "dns.h"
#include "ping.h"
#include "wiz.h"
#include "stm32conf.h"
#include "delay.h"
#include "httpd.h"
#include <string.h>
#include <ctype.h>

// ****************************************************************************
// Internal variables

// Our largest buffer is for the 'sendto' operation
static u8 wiz_buffer[ WIZNET_BUFFER_SIZE ];

// Server handler protortype
typedef int ( *p_server_handler )();

#define WIZS_SERVICE_SOCKET         1
#define WIZS_SEND_TIMEOUT           4000

// Application web data
static char wizs_app_page[ WIZ_APP_MAX_PAGE + 1 ];
static char wizs_app_args[ WIZ_APP_MAX_NUM_ARGS ][ WIZ_APP_MAX_ARG_LEN + 1 ];
static char wizs_app_name[ WIZ_APP_MAX_NAME_LEN + 1 ];
static char wizs_app_cgi_data[ WIZ_APP_MAX_CGI_LEN + 1 ];
static unsigned wizs_app_crt_arg;
static u8 wizs_app_finished;

// ****************************************************************************
// Helpers

// Flush all data in serial port
static void flush_serial()
{
  while( ser_read_byte( SER_NO_TIMEOUT ) != -1 );
}

// Read a packet from the serial port
static int read_request_packet()
{
  u16 temp16;
  u16 readbytes;

  // First read the length
  if( ( readbytes = ser_read( wiz_buffer, ELUARPC_START_OFFSET, SER_INF_TIMEOUT ) ) != ELUARPC_START_OFFSET )
  {
    flush_serial();
    return 0;
  }

  if( eluarpc_get_packet_size( wiz_buffer, &temp16 ) == ELUARPC_ERR )
  {
    flush_serial();
    return 0;
  }

  // Then the rest of the data
  if( ( readbytes = ser_read( wiz_buffer + ELUARPC_START_OFFSET, temp16 - ELUARPC_START_OFFSET, SER_INF_TIMEOUT ) ) != temp16 - ELUARPC_START_OFFSET )
  {
    flush_serial();
    return 0;
  }
  return 1;
}

static void wiz_ser_write_byte( u8 d )
{
  if( d == WIZ_ESC_CHAR || d == WIZ_HAS_CGI_CHAR )
  {
    ser_write_byte( WIZ_ESC_CHAR );
    ser_write_byte( d ^ WIZ_ESC_MASK );
  }
  else
    ser_write_byte( d );  
}

static void wiz_ser_write( const u8* p, u16 size )
{
  unsigned i;

  for( i = 0; i < size; i ++ )
    wiz_ser_write_byte( p[ i ] );
}

// Send a packet to the serial port
static void send_response_packet()
{
  u16 temp16;

  // Send request
  if( eluarpc_get_packet_size( wiz_buffer, &temp16 ) != ELUARPC_ERR )
    wiz_ser_write( wiz_buffer, temp16 );
}

// Convert a 32-bit BE number into an IP address
static void server_u32_to_ip( u32 ip, u8* pdest )
{
  pdest[ 0 ] = ip >> 24;
  pdest[ 1 ] = ( ip >> 16 ) & 0xFF;
  pdest[ 2 ] = ( ip >> 8 ) & 0xFF;
  pdest[ 3 ] = ip & 0xFF;
}

// Convert an IP address into a 32-bit BE number
static u32 server_ip_to_u32( u8 *ip )
{
  u32 res = 0;

  res = ( u32 )ip[ 0 ] << 24;
  res |= ( u32 )ip[ 1 ] << 16;
  res |= ( u32 )ip[ 2 ] << 8;
  res |= ( u32 )ip[ 3 ];
  return res;
}

// ****************************************************************************
// Server request handlers

 static char gstr[ 41 ];

static int server_socket()
{
  u8 type, sockno;
  u16 port;

  // Validate request
  if( wiznet_socket_read_request( wiz_buffer, &type, &port ) == ELUARPC_ERR )
    return 0;

  // Find free socket
  for( sockno = WIZ_FIRST_APP_SOCKET; sockno <= WIZ_LAST_APP_SOCKET; sockno ++ )
    if( socket_state( sockno ) == SOCKET_CLOSED )
      break;
  if( socket_state( sockno ) == SOCKET_CLOSED )
  {
    // Change type
    if( type == WIZ_SOCK_TCP )
      type = PROTOCOL_TCP;
    else if( type == WIZ_SOCK_UDP )
      type = PROTOCOL_UDP;
    else
      type = PROTOCOL_IP_RAW;
    
    // Execute request
    if( socket( sockno, type, port, 0 ) == 0 )
      sockno = ( u8 )-1;
    else
      sockno -= WIZ_FIRST_APP_SOCKET;
  }
  else
    sockno = ( u8 )-1;

  // Send back response
  wiznet_socket_write_response( wiz_buffer, sockno );
  return 1;
}

static int server_write()
{
  u8 sockno;
  const void *buf;
  u16 count;

  // Validate request
  if( wiznet_write_read_request( wiz_buffer, &sockno, &buf, &count ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Execute request
  count = send( sockno, ( const uint8* )buf, count, WIZS_SEND_TIMEOUT ); 

  // Send back response
  wiznet_write_write_response( wiz_buffer, count );
  return 1;
}

static int server_read()
{
  u8 sockno;
  u16 count, timeout;
  u8 execute_request = 1;

  // Validate request
  if( wiznet_read_read_request( wiz_buffer, &sockno, &count, &timeout ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Wait for data
  vtmr_set_timeout( VTMR_NET, timeout );
  vtmr_enable( VTMR_NET );
  while( !vtmr_is_expired( VTMR_NET ) )
    if( wiz_getSn_RX_RSR( sockno ) > 0 )
      break;
  vtmr_disable( VTMR_NET );
  if( wiz_getSn_RX_RSR( sockno ) == 0 )
    execute_request = 0;    

  // Execute request if needed
  if( execute_request )
    count = recv( sockno, wiz_buffer + ELUARPC_SMALL_READ_BUF_OFFSET, count > wiz_getSn_RX_RSR( sockno ) ? wiz_getSn_RX_RSR( sockno ) : count );
  else
    count = 0;

  // Send back response
  wiznet_read_write_response( wiz_buffer, count );
  return 1;
}

static int server_sendto()
{
  u8 sockno;
  u32 dest;
  const void *buf;
  u16 count, port, timeout;
  u8 ip[ 4 ];

  // Validate request
  if( wiznet_sendto_read_request( wiz_buffer, &sockno, &dest, &port, &buf, &count, &timeout ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Execute request
  server_u32_to_ip( dest, ip );
  count = sendto( sockno, ( const uint8* )buf, count, ip, port, timeout ); 

  // Send back response
  wiznet_sendto_write_response( wiz_buffer, count );
  return 1;
}

static int server_recvfrom()
{
  u8 sockno;
  u16 count, timeout, port;
  u8 ip[ 4 ];
  u32 from;
  u8 execute_request = 1;

  // Validate request
  if( wiznet_recvfrom_read_request( wiz_buffer, &sockno, &count, &timeout ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Wait for data
  vtmr_set_timeout( VTMR_NET, timeout );
  vtmr_enable( VTMR_NET );
  while( !vtmr_is_expired( VTMR_NET ) )
    if( wiz_getSn_RX_RSR( sockno ) > 0 )
      break;
  vtmr_disable( VTMR_NET );
  if( wiz_getSn_RX_RSR( sockno ) == 0 )
    execute_request = 0;      

  // Execute request if needed
  if( execute_request )
  {
    count = recvfrom( sockno, wiz_buffer + ELUARPC_SMALL_READ_BUF_OFFSET, count, ip, &port );
    from = server_ip_to_u32( ip );
  }
  else
  {
    count = 0;
    from = 0;
    port = 0;
  }

  // Send back response
  wiznet_recvfrom_write_response( wiz_buffer, from, port, count );
  return 1;
}

static int server_lookup()
{
  const char* pname;
  u8 ip[ 4 ];
  u32 hostip = 0;
  u16 timeout;

  // Validate arguments
  if( wiznet_lookup_read_request( wiz_buffer, &pname, &timeout ) == ELUARPC_ERR )
    return 0;

  // Execute request
  if( dns_query( WIZS_SERVICE_SOCKET, ip, pname, timeout ) )
    hostip = server_ip_to_u32( ip );

  // Send back response
  wiznet_lookup_write_response( wiz_buffer, hostip );
  return 1;
}

static int server_ping()
{
  u32 hostip;
  u8 ip[ 4 ];
  u16 timeout;
  u8 tries;

  // Validate arguments
  if( wiznet_ping_read_request( wiz_buffer, &hostip, &tries, &timeout ) == ELUARPC_ERR )
    return 0;

  // Execute request
  server_u32_to_ip( hostip, ip );
  tries = ping( WIZS_SERVICE_SOCKET, ip, tries, timeout );

  // Send back response
  wiznet_ping_write_response( wiz_buffer, tries );
  return 1;
}

static int server_close()
{
  u8 sockno;

  // Validate request
  if( wiznet_close_read_request( wiz_buffer, &sockno ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Execute request
  if( socket_state( sockno ) == SOCK_ESTABLISHED )
    disconnect( sockno );
  close( sockno );

  // Send back response
  wiznet_close_write_response( wiz_buffer, 1 );
  return 1;
}

static int server_connect()
{
  u8 sockno;
  u32 hostip;
  u8 ip[ 4 ];
  u16 port;

  // Validate request
  if( wiznet_connect_read_request( wiz_buffer, &sockno, &hostip, &port ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  // Execute request
  server_u32_to_ip( hostip, ip );
  sockno = connect( sockno, ip, port, WIZ_INF_TIMEOUT );
  
  // Send back response
  wiznet_connect_write_response( wiz_buffer, sockno );
  return 1;  
}

// Initiate HTTP request
// Expects an URL on input (http://host[:port][/path])
static int server_http_request()
{
  const char* purl;
  u8 sockno, res = WIZ_HTTP_REQ_ERR_URL;
  char *p, *req, *hostname;
  u8 ip[ 4 ];
  u16 port = 80;
  char local_url[ 320 ];
  u16 timeout;

  // Validate request
  if( wiznet_http_request_read_request( wiz_buffer, &sockno, &purl, &timeout ) == ELUARPC_ERR )
    return 0;
  sockno += WIZ_FIRST_APP_SOCKET;

  if( strlen( purl ) > 320 )
  {
    res = WIZ_HTTP_REQ_ERR_TOO_LONG;
    goto req_out;
  }
  strcpy( local_url, purl );

  // Analyze request
  if( strstr( local_url, "http://" ) != local_url )
    goto req_out;
  p = local_url + strlen( "http://" );
  if( strlen( p ) == 0 )
    goto req_out;
  hostname = p;

  // Look for trailing '/'
  if( p[ strlen( p ) - 1 ] == '/' )
    p[ strlen( p ) - 1 ] = '\0';

  // Separate hostname and path
  if( req = strstr( p, "/" ) )
    *req ++ = '\0';

  // Look for hostname:port and parse port if found
  if( p = strstr( p, ":" ) )
  {
    *p ++ = '\0';
    if( strlen( p ) == 0 || strlen( p ) > 5 )
      goto req_out;
    for( port = 0; *p; p ++ )
      if( !isdigit( *p ) )
        goto req_out;
      else
        port = port * 10 + *p - '0';
  }

  // Lookup host name
  if( dns_query( WIZS_SERVICE_SOCKET, ip, hostname, timeout ) == 0 )
  {
    res = WIZ_HTTP_REQ_ERR_LOOKUP;
    goto req_out;
  }

  // Connect socket
  if( connect( sockno, ip, port, WIZ_INF_TIMEOUT ) == 0 )
  {
    res = WIZ_HTTP_REQ_ERR_CONNECT;
    goto req_out;
  }
  
  evb_set_lcd_text( 0, "                " );
  evb_set_lcd_text( 0, hostname );
	sprintf(gstr,"%.3bu.%.3bu.%.3bu.%.3bu ", ip[ 0 ], ip[ 1 ], ip[ 2 ], ip[ 3 ] );
  evb_set_lcd_text(1,gstr);	    
  // Build request (use wiz_buffer for this, as we don't need it right now)
  sprintf( wiz_buffer, "GET /%s HTTP/1.1\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nUser-Agent: Mozilla/4.0\r\n\r\n", 
    req && *req ? req : "", hostname );

  // And send request
  if( send( sockno, wiz_buffer, strlen( wiz_buffer ), timeout ) != strlen( wiz_buffer ) )
  {
    disconnect( sockno );
    res = WIZ_HTTP_REQ_ERR_SEND;
  }
  else
    res = WIZ_HTTP_REQ_OK;
   
req_out:
  // Send back response
  wiznet_http_request_write_response( wiz_buffer, res );
  return 1; 
}

// ****************************************************************************
// Application web page handling

static void server_app_internal_init()
{
  unsigned i;

  wizs_app_page[ 0 ] = wizs_app_page[ WIZ_APP_MAX_PAGE ] = '\0';
  for( i = 0; i < WIZ_APP_MAX_NUM_ARGS; i ++ )
    wizs_app_args[ i ][ 0 ] = wizs_app_args[ i ][ WIZ_APP_MAX_ARG_LEN ] = '\0';
  wizs_app_name[ 0 ] = wizs_app_name[ WIZ_APP_MAX_NAME_LEN ] = '\0';
  wizs_app_cgi_data[ 0 ] = wizs_app_cgi_data[ WIZ_APP_MAX_CGI_LEN ] = '\0';
  wizs_app_crt_arg = 0;
  wizs_app_finished = 0;
}

static int server_app_init()
{
  const char *appname;

  // Validate request
  if( wiznet_app_init_read_request( wiz_buffer, &appname ) == ELUARPC_ERR )
    return 0;

  // Execute request
  server_app_internal_init();
  strncpy( wizs_app_name, appname, WIZ_APP_MAX_NAME_LEN );

  // Write back response
  wiznet_app_init_write_response( wiz_buffer, 1 );
  return 1;
}

static int server_app_set_page()
{
  const char *page;

  // Validate request
  if( wiznet_app_set_page_read_request( wiz_buffer, &page ) == ELUARPC_ERR )
    return 0;

  // Execute request
  strncat( wizs_app_page, page, WIZ_APP_MAX_PAGE );

  // Write back response
  wiznet_app_set_page_write_response( wiz_buffer, 1 );
  return 1;
}

static int server_app_set_arg()
{
  const char *arg;
  u8 res = 1;

  // Validate request
  if( wiznet_app_set_arg_read_request( wiz_buffer, &arg ) == ELUARPC_ERR )
    return 0;

  if( wizs_app_crt_arg == WIZ_APP_MAX_NUM_ARGS )
    res = 0;
  else
    strncpy( wizs_app_args[ wizs_app_crt_arg ++ ], arg, WIZ_APP_MAX_ARG_LEN );

  // Write back response
  wiznet_app_set_arg_write_response( wiz_buffer, res );
  return 1;
}

static int server_app_get_cgi()
{
  // Validate request
  if( wiznet_app_get_cgi_read_request( wiz_buffer ) == ELUARPC_ERR )
    return 0;
                             
  // Write back response     
  wiznet_app_get_cgi_write_response( wiz_buffer, 1, wizs_app_cgi_data );
  return 1;
}

static int server_app_end_page()
{
  // Validate request
  if( wiznet_app_end_page_read_request( wiz_buffer ) == ELUARPC_ERR )
    return 0;

  wizs_app_finished = 1;

  // Write back response
  wiznet_app_end_page_write_response( wiz_buffer, 1 );
  return 1;  
}

// ****************************************************************************
// Dispatcher

// List of server request handlers
static const p_server_handler server_handlers[] = 
{ 
  server_socket, server_write, server_read, server_sendto,
  server_recvfrom, server_lookup, server_ping, server_close,
  server_connect, server_http_request, server_app_init,
  server_app_set_page, server_app_set_arg, server_app_get_cgi,
  server_app_end_page
};

// Execute the given requerst
static int server_execute_request()
{
  u8 req;
  	
  if( eluarpc_get_request_id( wiz_buffer, &req ) == ELUARPC_ERR )
    return 0;
  if( req >= WIZ_OP_FIRST && req <= WIZ_OP_LAST ) 
    return server_handlers[ req - WIZ_OP_FIRST ]();
  else
    return 0;
}

// ****************************************************************************
// Public interface

void wizs_init()
{
  // Setup timers
  vtmr_setup( VTMR_NET, VTMR_TYPE_ONESHOT, 1000 );
  // And application data
  server_app_internal_init();
}

void wizs_execute_request()
{
  if( read_request_packet() )
    if( server_execute_request() )
      send_response_packet();
}

int wizs_app_exists()
{
  return wizs_app_finished && wizs_app_name[ 0 ] != '\0';
}

const char* wizs_app_get_name()
{
  return wizs_app_finished ? wizs_app_name : NULL;
}

const char* wizs_app_get_template()
{
  return wizs_app_finished && wizs_app_page[ 0 ] != '\0' ? wizs_app_page : NULL;
}

const char* wizs_app_get_arg( unsigned i )
{
  if( !wizs_app_finished )
    return NULL;
  if( i < wizs_app_crt_arg )
    return wizs_app_args[ i ];
  else
    return NULL;
}

void wizs_app_set_cgi_data( const char* cgidata )
{
  // Set the CGI data
  if( !cgidata || strlen( cgidata ) == 0 )
    return;
  strncpy( wizs_app_cgi_data, cgidata, WIZ_APP_MAX_CGI_LEN );
  // And signal to the STM32 board that we have a response ready
  ser_write_byte( WIZ_HAS_CGI_CHAR );
}
