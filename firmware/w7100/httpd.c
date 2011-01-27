// Wiz HTTP server

#include "httpd.h"
#include "type.h"
#include "socket.h"
#include "wiz.h"
#include "utils.h"
#include "lcd.h"
#include "settings.h"
#include "wizs.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define WEBSERVER
#include "index_html.h"

// ****************************************************************************
// Local variables

// What page to send after a HTTP request was intepreted
#define HTTPD_SEND_NOTHING              0
#define HTTPD_SEND_INDEX_PAGE           1
#define HTTPD_SEND_APP_PAGE             2
#define HTTPD_PROCESS_REQUEST_MAIN      3
#define HTTPD_PROCESS_REQUEST_APP       4

// Replacer argument
#define HTTPD_REPLACE_MAIN_TEMPLATE     0
#define HTTPD_REPLACE_APP_TEMPLATE      1

static char httpd_req_buf[ HTTPD_BUF_SIZE ];
static char httpd_resp_buf[ HTTPD_MAX_RESP_SIZE ];
static char gstr[ 41 ];
static char httpd_last_err[ HTTPD_MAX_PARAM_SIZE + 1 ];
static char *httpd_phost;
static char httpd_cgi_arg[ HTTPD_MAX_PARAM_SIZE + 1 ];
static char httpd_cgi_val[ HTTPD_MAX_PARAM_SIZE + 1 ];

// ****************************************************************************
// Hardcoded HTTP data (codes and responses)

static char http_auth_response[] = "HTTP/1.0 401 Unauthorized\r\n"
  "Server: moonlight\r\n"
  "WWW-Authenticate: Basic realm=\"moonlight\"\r\n"
  "Content-Type: text/html\r\n"
  "Connection: close\r\n"
  "\r\n"
  "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD><BODY>401 Unauthorized</BODY></HTML>";

static char http_invalid_request_response[] = "HTTP/1.0 400 Bad Request\r\n"
  "Server: moonlight\r\nContent-Type: text/html\r\n"
  "Connection: close\r\n"
  "\r\n"
  "<html><head><title>Invalid request</title></head><body>The server received a request that it couldn't understand</body></html>";

static char http_resp_header[] = "HTTP/1.0 200 OK\r\n"
  "Server: moonlight\r\n"
  "Cache-Control: no-cache\r\n"
  "Pragma: no-cache\r\n"
  "Expires: 0\r\n"
  "Content-Type: text/html\r\n"
  "Connection: close\r\n"
  "\r\n";

static char http_redirect_response[] = "HTTP/1.0 302 Found\r\n"
  "Server: moonlight\r\n"
  "Location: http://%s/\r\n"
  "Connection: close\r\n"
  "\r\n";

static char http_not_found_response[] = "HTTP/1.0 404 Not Found\r\n"
  "Server: moonlight\r\n"
  "Content-Type: text/html\r\n"
  "Connection: close\r\n"
  "\r\n"
  "<html><head><title>Not found</title></head><body>Page not found</body></html>";

// ****************************************************************************
// Local functions

// Send a "HTTP invalid request" response
static int httpd_invalid_request()
{
  send( HTTPD_SOCKET, http_invalid_request_response, strlen( http_invalid_request_response ), HTTPD_SEND_TIMEOUT ); 
  return 1;
}

// Send a "HTTP needs authorization" response
static int httpd_needs_authorization()
{
  send( HTTPD_SOCKET, http_auth_response, strlen( http_auth_response ), HTTPD_SEND_TIMEOUT ); 
  return 1;
}

// Send a "HTTP not found" response
static int httpd_not_found()
{
  send( HTTPD_SOCKET, http_not_found_response, strlen( http_not_found_response ), HTTPD_SEND_TIMEOUT );
  return 1;
}

// Send a response header
static void httpd_send_response_header()
{
  send( HTTPD_SOCKET, http_resp_header, strlen( http_resp_header ), HTTPD_SEND_TIMEOUT );
}

// Send a redirect response
static void httpd_send_redirect()
{
  SETTINGS *psettings = settings_get();
  char ipaddr[ 16 ];

  if( httpd_phost )
    sprintf( httpd_resp_buf, http_redirect_response, httpd_phost );
  else
  {
    sprintf( ipaddr, "%d.%d.%d.%d", ( int )psettings->ip[ 0 ], ( int )psettings->ip[ 1 ], ( int )psettings->ip[ 2 ], ( int )psettings->ip[ 3 ] ); 
    sprintf( httpd_resp_buf, http_redirect_response, ipaddr );
  }
  send( HTTPD_SOCKET, httpd_resp_buf, strlen( httpd_resp_buf ), HTTPD_SEND_TIMEOUT );
}

// Convert an URL encoded CGI argument to ascii
static void httpd_cgi_arg_to_ascii( char *dest, const char* src, unsigned maxlen )
{
  unsigned i, j, totlen = strlen( src );

  i = j = 0;
  while( i < totlen && j < maxlen )
  {
    if( src[ i ] != '%' )
      dest[ j ++ ] = src[ i ++ ];
    else
    {
      dest[ j ++ ] = ( utils_from_hex_digit( src[ i + 1 ] ) << 4 ) + utils_from_hex_digit( src[ i + 2 ] );
      i += 3;
    }
  }
  dest[ j ] = '\0';
}

// Return the next arg/value pair in a list of CGI arguments
static char* httpd_get_cgi_data( char *args, char **parg, char **pvalue )
{ 
  char *p, *p1;

  httpd_cgi_arg[ 0 ] = httpd_cgi_val[ 0 ] = '\0';
  *parg = httpd_cgi_arg;
  *pvalue = httpd_cgi_val;
  if( args == NULL )
    return NULL;
  if( p = strchr( args, '&' ) )
    *p = '\0';
  if( ( p1 = strchr( args, '=' ) ) == NULL )
    return NULL;
  *p1 = '\0';
  httpd_cgi_arg_to_ascii( httpd_cgi_arg, args, HTTPD_MAX_PARAM_SIZE );
  httpd_cgi_arg_to_ascii( httpd_cgi_val, p1 + 1, HTTPD_MAX_PARAM_SIZE );
  return p ? p + 1 : NULL;         
}

// ****************************************************************************
// CGI handlers

// Main page CGI handler
static void httpd_main_cgi_handler( char *pars )
{
  char *p, *arg, *val;
  SETTINGS new_settings;

  new_settings = *settings_get();
  new_settings.static_mode = 1;
  p = pars;
  httpd_resp_buf[ 0 ] = '\0';
  while( 1 )  
  {
    // Get a single argument/value pair
    p = httpd_get_cgi_data( p, &arg, &val );

    if( arg[ 0 ] && val[ 0 ] )
    {
      // Process this pair
      if( strcmp( arg, "sip" ) == 0 ) // source ip
      {
        if( utils_string_to_ip( val, &new_settings.ip ) == 0 )
        {
          strcpy( httpd_last_err, "Invalid source IP" );
          return;
        }
      }

      else if( strcmp( arg, "gwip" ) == 0 ) // gateway
      {
        if( utils_string_to_ip( val, &new_settings.gw ) == 0 )
        {
          strcpy( httpd_last_err, "Invalid gateway IP" );
          return;
        }
      }

      else if( strcmp( arg, "sn" ) == 0 ) // netmask
      {
        if( utils_string_to_ip( val, &new_settings.mask ) == 0 )
        {
          strcpy( httpd_last_err, "Invalid network mask" );
          return;
        }
      }

      else if( strcmp( arg, "dns" ) == 0 ) // dns
      {
        if( utils_string_to_ip( val, &new_settings.dns ) == 0 )
        {
          strcpy( httpd_last_err, "Invalid DNS IP" );
          return;
        }
      }

      else if( strcmp( arg, "hwaddr" ) == 0 ) // MAC
      {
        if( utils_string_to_mac( val, &new_settings.mac ) == 0 )
        {
          strcpy( httpd_last_err, "Invalid MAC address" );
          return;
        }
      } 
      
      else if( strcmp( arg, "isdhcp" ) == 0 && strcmp( val, "on" ) == 0 )
        new_settings.static_mode = 0;
        
      else if( strcmp( arg, "bname" ) == 0 )
      {
        new_settings.name[ SETTINGS_MAX_NAME_LEN ] = '\0';
        strncpy( new_settings.name, val, SETTINGS_MAX_NAME_LEN );
      }

      else if( strcmp( arg, "username" ) == 0 )
      {
        new_settings.username[ SETTINGS_MAX_USERNAME_LEN ] = '\0';
        strncpy( new_settings.username, val, SETTINGS_MAX_USERNAME_LEN );
      }

      else if( strcmp( arg, "password" ) == 0 )
      {
        new_settings.password[ SETTINGS_MAX_PASSWORD_LEN ] = '\0';
        strncpy( new_settings.password, val, SETTINGS_MAX_PASSWORD_LEN );
      }
        
      else 
      {
        strcpy( httpd_last_err, "Invalid argument found in CGI list" );
        return;
      }           
    }

    if( p == NULL )
      break;
  }
  settings_set( &new_settings );
  settings_write();
  strcpy( httpd_last_err, "Configuration updated" );
}

// APP CGI handler (store CGI data in a buffer for later use)
void httpd_app_cgi_handler( char *pars )
{
  wizs_app_set_cgi_data( pars );
}

// ****************************************************************************
// Parameter replacement functions

// Main replace parameter function
static void httpd_main_replace_param( u8 id, char* buf )
{
  SETTINGS *psettings = settings_get();
  u8 *p = NULL;
  u8 i, j;
  char ipaddr[ 16 ];

  buf[ 0 ] = buf[ HTTPD_MAX_PARAM_SIZE ] = '\0';
  switch( id )
  {
    case HTTPD_PAR_SOURCE_IP:
      p = psettings->ip;
      break;

    case HTTPD_PAR_GATEWAY_IP:
      p = psettings->gw;
      break;

    case HTTPD_PAR_SUBNET_MASK:
      p = psettings->mask;
      break;

    case HTTPD_PAR_DNS_SERVER_IP:
      p = psettings->dns;
      break;

    case HTTPD_PAR_MAC_ADDRESS:
      p = psettings->mac;
      for( i = j = 0; i < 6; i ++ )
      {
        buf[ j ++ ] = utils_to_hex_digit( p[ i ] >> 4 );
        buf[ j ++ ] = utils_to_hex_digit( p[ i ] & 0x0F );
        if( i != 5 )
          buf[ j ++ ] = ':';
      }
      buf[ j ] = '\0';
      p = NULL;
      break;
            
    case HTTPD_PAR_BOARD_NAME:
      strcpy( buf, psettings->name );
      break;

    case HTTPD_PAR_USE_DHCP:
      if( !psettings->static_mode )
        strcpy( buf, "checked" );
      break;

    case HTTPD_PAR_USERNAME:
      strcpy( buf, psettings->username );
      break;

    case HTTPD_PAR_PASSWORD:
      strcpy( buf, psettings->password );
      break;

    case HTTPD_PAR_APPNAME:
      if( wizs_app_exists() )
        strcpy( buf, wizs_app_get_name() );
      else
        strcpy( buf, "not available" );
      break;

    case HTTPD_PAR_APPURL:
      if( wizs_app_exists() )
      {
        p = psettings->ip;
        if( !httpd_phost )
        {
          sprintf( ipaddr, "%d.%d.%d.%d", ( int )p[ 0 ], ( int )p[ 1 ], ( int )p[ 2 ], ( int )p[ 3 ] ); 
          p = ipaddr;
        }
        else
          p = httpd_phost;
        sprintf( buf, "<a href=\"http://%s/app\">http://%s/app</a>", p, p );
        p = NULL;
      }
      else
        strcpy( buf, "not available" );
      break;

    case HTTPD_PAR_MESSAGE:
      strcpy( buf, httpd_last_err );
      httpd_last_err[ 0 ] = '\0';
      break;
  }
  if( p )
    sprintf( buf, "%d.%d.%d.%d", ( int )p[ 0 ], ( int )p[ 1 ], ( int )p[ 2 ], ( int )p[ 3 ] );
}

// Application replace parameter function
static void httpd_app_replace_param( u8 id, char* buf )
{
  const char* parg = wizs_app_get_arg( id );
  
  buf[ 0 ] = buf[ HTTPD_MAX_PARAM_SIZE ] = '\0';
  if( parg )
    strncpy( buf, parg, HTTPD_MAX_PARAM_SIZE );
}

// Parse HTTP template parameters
static void httpd_parse_and_replace( char *src, u8 type )
{
  char parbuf[ HTTPD_MAX_PARAM_SIZE + 1 ], *p, *p1;
  u16 initlen;
  u8 id;
  
  memset( httpd_resp_buf, 0, HTTPD_MAX_RESP_SIZE );
  // Iterate through the template, replacing parameters as needed
  p = src;
  initlen = strlen( src );
  while( p - src < initlen && *p  )
  {
    if( ( p1 = strchr( p, '$' ) ) == NULL )
    {
      strcat( httpd_resp_buf, p );
      break;
    }
    // We found a '$', check if it of the form $xx$ (a parameter)
    if( strlen( p1 ) < 4 )
    {
      strcat( httpd_resp_buf, p1 );
      break;
    }
    if( isdigit( p1[ 1 ] ) && isdigit( p1[ 2 ] ) && p1[ 3 ] == '$' )
    {
      // We found a parameter
      id = ( p1[ 1 ] - '0' ) * 10 + p1[ 2 ] - '0';
      if( type == HTTPD_REPLACE_MAIN_TEMPLATE )
        httpd_main_replace_param( id, parbuf );
      else
        httpd_app_replace_param( id, parbuf );
      strncat( httpd_resp_buf, p, p1 - p ); 
      strcat( httpd_resp_buf, parbuf );
      p = p1 + 4; 
    } 
    else
    {
      strncat( httpd_resp_buf, p, p1 - p + 1 ); 
      p = p1 + 1;
    }
  }
}

// ****************************************************************************
// Other HTTPD functions

// Send the index page
static void http_send_index_page()
{
  // Send header
  httpd_send_response_header();
  // Parse index page, looking for parameters to replace
  httpd_parse_and_replace( http_index_template, HTTPD_REPLACE_MAIN_TEMPLATE );
  // Send response
  send( HTTPD_SOCKET, httpd_resp_buf, strlen( httpd_resp_buf ), HTTPD_SEND_TIMEOUT );
}

// Send the app page
static void http_send_app_page()
{      
  if( !wizs_app_exists() || !wizs_app_get_template() )
    httpd_not_found();
  else
  {
    // Send header
    httpd_send_response_header();
    // Parse index page, looking for parameters to replace
    httpd_parse_and_replace( wizs_app_get_template(), HTTPD_REPLACE_APP_TEMPLATE );
    // Send response
    send( HTTPD_SOCKET, httpd_resp_buf, strlen( httpd_resp_buf ), HTTPD_SEND_TIMEOUT );
  }
}

// Parse a HTTP request, send HTTP response
static int httpd_read_parse()
{
  u8 *p, *nl, *p1, *p2, *username, *password, *params, post_request, post_content_type_found;
  u16 initlen, lineno, send_id;
  char authdata[ HTTPD_AUTH_SIZE + 1 ];
  SETTINGS *psettings = settings_get();
  int post_len;

  // Read at max HTTPD_BUF_SIZE bytes from the request, discard the rest
  if( ( initlen = wiz_getSn_RX_RSR( HTTPD_SOCKET ) ) > HTTPD_BUF_SIZE )
    initlen = HTTPD_BUF_SIZE;
  initlen = recv( HTTPD_SOCKET, httpd_req_buf, initlen );
  httpd_req_buf[ initlen ] = 0;
  if( wiz_getSn_RX_RSR( HTTPD_SOCKET ) > 0 )
    recv_flush( HTTPD_SOCKET, wiz_getSn_RX_RSR( HTTPD_SOCKET ) );

  p = httpd_req_buf;
  lineno = 0;
  send_id = HTTPD_SEND_NOTHING;
  username = password = params = NULL;
  post_request = 0;
  post_content_type_found = 0;
  post_len = 0;
  httpd_phost = NULL;
  while( p - httpd_req_buf < initlen && *p ) 
  {
    // Get the current line
    if( ( nl = strchr( p, '\r' ) ) == NULL )
        return httpd_invalid_request();

    if( *( nl + 1 ) != '\n' )
      return httpd_invalid_request();
    if( nl == p ) // this was the last line in the request
    {
      if( post_request )
        params = p + 2;
      break;
    }
    *nl = 0;
    lineno ++;

    // Interpret current line
    if( strstr( p, "GET " ) == p || strstr( p, "POST " ) == p )
    {
      if( lineno != 1 )
        return httpd_invalid_request();
      // GET or POST line, read request
      if( strstr( p, "POST " ) == p )
        post_request = 1;
      p1 = p + ( post_request == 0 ? strlen( "GET " ) : strlen( "POST " ) );
      if( ( p2 = strchr( p1, ' ' ) ) == NULL )
        return httpd_invalid_request();
      if( strstr( p2 + 1, "HTTP/" ) != p2 + 1 )
        return httpd_invalid_request(); 
      *p2 = 0;
      if( ( p2 = strchr( p1, '?' ) ) != NULL )
      {
        if( post_request )
          return httpd_invalid_request();
        *p2 = 0;
        params = p2 + 1;
      }
      if( post_request == 0 && ( strcmp( p1, "/" ) == 0 || strcmp( p1, "/index.html" ) == 0 || strcmp( p1, "/index.htm" ) == 0 ) )
        send_id = HTTPD_SEND_INDEX_PAGE;
      else if( post_request == 0 && strcmp( p1, "/app" ) == 0 ) 
        send_id = HTTPD_SEND_APP_PAGE;
      else if( strcmp( p1, "/moonlight.cgi" ) == 0 )
        send_id = HTTPD_PROCESS_REQUEST_MAIN;
      else if( strcmp( p1, "/app.cgi" ) == 0 )
        send_id = HTTPD_PROCESS_REQUEST_APP;
      else
        return httpd_not_found();
    }
    else if( strstr( p, "Authorization: Basic " ) == p )
    {
      // Interpret authorization line
      p1 = p + strlen( "Authorization: Basic " );
      utils_base64_decode( authdata, p1, HTTPD_AUTH_SIZE );
      if( ( p2 = strchr( authdata, ':' ) ) == NULL )
        return httpd_invalid_request();
      *p2 = 0;
      username = authdata;
      password = p2 + 1;
    }
    else if( post_request && strcmp( p, "Content-Type: application/x-www-form-urlencoded" ) == 0 )
      post_content_type_found = 1;
    else if( post_request && strstr( p, "Content-Length: " ) == p )
    {
      p1 = p + strlen( "Content-Length: " );
      if( sscanf( p1, "%d", &post_len ) != 1 )
        return httpd_invalid_request();
    }
    else if( strstr( p, "Host: " ) == p )
      httpd_phost = p + strlen( "Host: " );

    // Advance to next line
    p = nl + 2;       
  }

  // Check the POST request
  if( post_request )
  {
    if( post_content_type_found == 0 )
      return httpd_invalid_request();
    if( !params || strlen( params ) != post_len )
      return httpd_invalid_request();
  }

  // Now send back response
  if( send_id == HTTPD_SEND_NOTHING )
    httpd_not_found();
  else
  {
    // Check authentication
    if( strcmp( username, psettings->username ) || strcmp( password, psettings->password ) )
      return httpd_needs_authorization(); 
    if( send_id == HTTPD_SEND_INDEX_PAGE )
      http_send_index_page();
    else if( send_id == HTTPD_SEND_APP_PAGE )
      http_send_app_page();
    else if( send_id == HTTPD_PROCESS_REQUEST_MAIN )
    {
      httpd_main_cgi_handler( params );
      httpd_send_redirect();
    }
    else if( send_id == HTTPD_PROCESS_REQUEST_APP )
    {
      httpd_app_cgi_handler( params );
      strcpy( httpd_last_err, "Configuration data sent to application" );
      httpd_send_redirect();
    }
    else
      httpd_not_found();
  }

  return 1;
}

// ****************************************************************************
// Public interface

void httpd_init()
{
  httpd_last_err[ 0 ] = '\0';
}

void httpd_run()
{
  switch( wiz_getSn_SR( HTTPD_SOCKET ) )
  {
    case SOCK_ESTABLISHED: //if connection is established
      if( wiz_getSn_RX_RSR( HTTPD_SOCKET ) > 0 ) //check Rx data
      {
        httpd_read_parse();
        disconnect( HTTPD_SOCKET );
      }
      break;

    case SOCK_CLOSE_WAIT: //If the client request to close
      if( wiz_getSn_RX_RSR( HTTPD_SOCKET ) > 0 ) //check Rx data
        httpd_read_parse();
      disconnect( HTTPD_SOCKET );
      break;

    case SOCK_CLOSED: //if a socket is closed
      close( HTTPD_SOCKET );
      socket( HTTPD_SOCKET, PROTOCOL_TCP, HTTPD_PORT, 0 );
      break;

    case SOCK_INIT: //if a socket is initiated
      listen( HTTPD_SOCKET );
      break;
  }
}
