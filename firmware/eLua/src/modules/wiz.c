// Module for interfacing with network functions

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "elua_net.h"
#include "eluarpc.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include "lrotable.h"
#include "platform_conf.h"
#include "wizc.h"
#include "utils.h"
#include "wiznet.h"
#include "eluarpc.h"
#include "buf.h"
#include "common.h"

// *****************************************************************************
// Internal data structures and variables

// Socket data
static u8 wiz_sock_type[ WIZ_NUM_MAX_SOCKETS ];
// RPC buffer
static u8 wiz_buffer[ WIZNET_BUFFER_SIZE ];

// *****************************************************************************
// Helpers

static u32 ip_to_u32( const u8* ip )
{
  u32 temp = 0;
  
  temp = ip[ 0 ] << 24;
  temp |= ip[ 1 ] << 16;
  temp |= ip[ 2 ] << 8;
  temp |= ip[ 3 ];
  return temp;
}

static void u32_to_ip( u32 ip, u8 *pdest )
{
  pdest[ 0 ] = ip >> 24;
  pdest[ 1 ] = ( ip >> 16 ) & 0xFF;
  pdest[ 2 ] = ( ip >> 8 ) & 0xFF;
  pdest[ 3 ] = ip & 0xFF;  
}

// *****************************************************************************

// Lua: sock = socket( type, [port] )
static int wiz_socket( lua_State *L )
{
  int type;
  u16 port = 0;
  u8 res;
  
  // Check arguments
  type = luaL_checkinteger( L, 1 );
  if( lua_isnumber( L, 2 ) )
    port = ( u16 )luaL_checkinteger( L, 2 );  
  if( type != WIZ_SOCK_TCP && type != WIZ_SOCK_UDP && type != WIZ_SOCK_IPRAW )
    return luaL_error( L, "invalid socket type" );
    
  // Create the actual socket
  if( ( res = wizc_socket( ( u8 )type, port ) ) < 0 )
    return luaL_error( L, "unable to create socket" );
  wiz_sock_type[ res ] = type;
  
  lua_pushinteger( L, res );
  return 1;
}

// Lua: res = close( socket )
static int wiz_close( lua_State* L )
{
  int s;
  int res;
    
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );  
  res = wizc_close( s );
  wiz_sock_type[ s ] = WIZ_SOCK_NONE;
  lua_pushinteger( L, res );
  return 1;
}

// Lua: data = ip( ip0, ip1, ip2, ip3 ), or
// Lua: data = ip( "ip" )
// Returns an internal representation for the given IP address
static int wiz_ip( lua_State *L )
{
  unsigned i, temp;
  u8 ip[ 4 ];
  
  if( lua_isnumber( L, 1 ) )
    for( i = 0; i < 4; i ++ )
    {
      temp = luaL_checkinteger( L, i + 1 );
      if( temp < 0 || temp > 255 )
        return luaL_error( L, "invalid IP adddress" );
      ip[ i ] = temp;
    }
  else
  {
    const char* pip = luaL_checkstring( L, 1 );
    unsigned tip[ 4 ];
    unsigned len;
    
    if( sscanf( pip, "%u.%u.%u.%u%n", tip, tip + 1, tip + 2, tip + 3, &len ) != 4 || len != strlen( pip ) )
      return luaL_error( L, "invalid IP adddress" );    
    for( i = 0; i < 4; i ++ )
    {
      if( tip[ i ] < 0 || tip[ i ] > 255 )
        return luaL_error( L, "invalid IP address" );
      ip[ i ] = ( u8 )tip[ i ];
    }
  }
  lua_pushinteger( L, ( lua_Integer )ip_to_u32( ip ) );
  return 1;
}

// Lua: ip0, ip1, ip2, ip3 = unpackip( iptype, "*n" ), or
//               string_ip = unpackip( iptype, "*s" )
static int wiz_unpackip( lua_State *L )
{
  unsigned i;  
  const char* fmt;
  u32 ip;
  u8 cip[ 4 ];
  
  ip = ( u32 )luaL_checkinteger( L, 1 );
  fmt = luaL_checkstring( L, 2 );
  u32_to_ip( ip, cip );
  if( !strcmp( fmt, "*n" ) )
  {
    for( i = 0; i < 4; i ++ ) 
      lua_pushinteger( L, cip[ i ] );
    return 4;
  }
  else if( !strcmp( fmt, "*s" ) )
  {
    lua_pushfstring( L, "%d.%d.%d.%d", ( int )cip[ 0 ], ( int )cip[ 1 ], 
                     ( int )cip[ 2 ], ( int )cip[ 3 ] );
    return 1;
  }
  else
    return luaL_error( L, "invalid format" );                                      
}

// Lua: sent = sendto( s, ip, port, arg1, [arg2], ... [argn] )
/*static int wiz_sendto( lua_State *L )
{
  u32 ip;
  u16 port;
  const char* buf;
  size_t len;
  unsigned i;
  s16 sent = 0, towrite, wrote;
  int s; 
  u8 d;
      
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_UDP )
    return luaL_error( L, "invalid socket type for sendto (must be UDP)" );
  ip = ( u32 )luaL_checkinteger( L, 2 );
  port = ( u16 )luaL_checkinteger( L, 3 );
  for( i = 4; i <= lua_gettop( L ); i ++ )
  {
    if( lua_type( L, i ) == LUA_TNUMBER )
    {
      // Send a single byte
      len = lua_tointeger( L, i );
      if( ( len < 0 ) || ( len > 255 ) )
        return luaL_error( L, "invalid number" );
      d = ( u8 )len;
      sent += wizc_sendto( s, ip, port, &d, 1 ); 
    }
    else
    {
      // Send an arbitrary string, but split it in multiples of the RPC packet size
      luaL_checktype( L, i, LUA_TSTRING );
      buf = lua_tolstring( L, i, &len );      
      while( len )
      {
        towrite = UMIN( len, WIZNET_PACKET_SIZE );
        wrote = wizc_sendto( s, ip, port, buf, towrite );
        sent += wrote;
        if( wrote != towrite )
          break;        
        len -= towrite;
        buf += towrite;
      }     
    }
  }        
  lua_pushinteger( L, sent );
  return 1;
}       */

// Lua: sent = sendto( s, ip, port, arg, [timeout] )
static int wiz_sendto( lua_State *L )
{
  u32 ip;
  u16 port;
  const char* buf;
  size_t len;
  s16 sent = 0, towrite, wrote;
  int s; 
  u8 d;
  u16 timeout = WIZ_INF_TIMEOUT;
      
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_UDP )
    return luaL_error( L, "invalid socket type for sendto (must be UDP)" );
  ip = ( u32 )luaL_checkinteger( L, 2 );
  port = ( u16 )luaL_checkinteger( L, 3 );
  if( lua_isnumber( L, 5 ) )
    timeout = ( u16 )luaL_checkinteger( L, 5 );
  if( lua_type( L, 4 ) == LUA_TNUMBER )
  {
    // Send a single byte
    len = lua_tointeger( L, 4 );
    if( ( len < 0 ) || ( len > 255 ) )
      return luaL_error( L, "invalid number" );
    d = ( u8 )len;
    sent += wizc_sendto( s, ip, port, &d, 1, timeout ); 
  }
  else
  {
    // Send an arbitrary string, but split it in multiples of the RPC packet size
    luaL_checktype( L, 4, LUA_TSTRING );
    buf = lua_tolstring( L, 4, &len );      
    while( len )
    {
      towrite = UMIN( len, WIZNET_PACKET_SIZE );
      wrote = wizc_sendto( s, ip, port, buf, towrite, timeout );
      sent += wrote;
      if( wrote != towrite )
        break;        
      len -= towrite;
      buf += towrite;
    }     
  }        
  lua_pushinteger( L, sent );
  return 1;
} 

// Lua: recvd, host, port = recvfrom( s, size, [timeout] ) 
static int wiz_recvfrom( lua_State *L )
{
  u32 ip;
  int s;
  u16 timeout = WIZ_INF_TIMEOUT;
  u16 port, lastport = 0;
  s16 size, toread, bread;
  luaL_Buffer b;
  u8 *databuf;
  u32 lasthost = 0;
    
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_UDP )
    return luaL_error( L, "invalid socket type for recvfrom (must be UDP)" );
  size = ( s16 )luaL_checkinteger( L, 2 );
  if( size <= 0 )
    return luaL_error( L, "invalid size" );
  if( lua_isnumber( L, 3 ) )
    timeout = ( u16 )luaL_checkinteger( L, 3 );
    
  // Execute operation   
  luaL_buffinit( L, &b );
  lastport = 0;
  while( size )
  {
    toread = UMIN( size, WIZNET_PACKET_SIZE );  
    bread = wizc_recvfrom( s, &ip, &port, &databuf, toread, timeout );
    if( lastport != 0 && lastport != port )
      break;
    if( lasthost !=0 && lasthost != ip )
      break;
    if( bread > 0 )
      luaL_addlstring( &b, ( const char* )databuf, bread );
    if( bread != toread )
      break;
    lastport = port;  
    lasthost = ip;    
    size -= toread;
  }  
  luaL_pushresult( &b );
  lua_pushinteger( L, ip );
  lua_pushinteger( L, port );
  return 3;
}

// Lua: ip = lookup( "hostname", [timeout] )
static int wiz_lookup( lua_State *L )
{
  const char *hostname;
  u16 timeout = WIZ_INF_TIMEOUT;
  
  hostname = luaL_checkstring( L, 1 );
  if( lua_isnumber( L, 2 ) )
    timeout = ( u16 )luaL_checkinteger( L, 2 );
  lua_pushinteger( L, ( lua_Integer )wizc_lookup( hostname, timeout ) );
  return 1;
}

// Lua: bool_res = connect( socket, host, port )
static int wiz_connect( lua_State *L )
{
  u32 ip;
  int s, res;
  u16 port;
    
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_TCP )
    return luaL_error( L, "invalid socket type for connect (must be TCP)" );
  ip = ( u32 )luaL_checkinteger( L, 2 );
  port = ( u16 )luaL_checkinteger( L, 3 );
  res = wizc_connect( s, ip, port );
  if( res != 1 )
  {
    // An error occured, close the socket and marked it as closed on our side
    wizc_close( s );
    wiz_sock_type[ s ] = WIZ_SOCK_NONE;
  }
  lua_pushboolean( L, res );
  return 1;
}

// Lua: sentbytes = send( socket, arg1, [arg2], ... [argn] )
static int wiz_send( lua_State *L )
{
  const char* buf;
  size_t len;
  unsigned i;
  s16 sent = 0, towrite, wrote;
  int s; 
  u8 d;
      
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_TCP )
    return luaL_error( L, "invalid socket type for send (must be TCP)" );
  for( i = 2; i <= lua_gettop( L ); i ++ )
  {
    if( lua_type( L, i ) == LUA_TNUMBER )
    {
      // Send a single byte
      len = lua_tointeger( L, i );
      if( ( len < 0 ) || ( len > 255 ) )
        return luaL_error( L, "invalid number" );
      d = ( u8 )len;
      sent += wizc_write( s, &d, 1 ); 
    }
    else
    {
      // Send an arbitrary string, but split it in multiples of the RPC packet size
      luaL_checktype( L, i, LUA_TSTRING );
      buf = lua_tolstring( L, i, &len );      
      while( len )
      {
        towrite = UMIN( len, WIZNET_PACKET_SIZE );
        wrote = wizc_write( s, buf, towrite );
        sent += wrote;
        if( wrote != towrite )
          break;        
        len -= towrite;
        buf += towrite;
      }     
    }
  }        
  lua_pushinteger( L, sent );
  return 1;
}

// Lua: recvd = recv( s, size, [timeout])
static int wiz_recv( lua_State *L )
{
  int s;
  u16 timeout = WIZ_INF_TIMEOUT;
  s16 size, bread, toread;
  luaL_Buffer b;
  u8 *databuf;
    
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_TCP )
    return luaL_error( L, "invalid socket type for recv (must be TCP)" );
  size = ( s16 )luaL_checkinteger( L, 2 );
  if( size <= 0 )
    return luaL_error( L, "invalid size" );  
  if( lua_isnumber( L, 3 ) )
    timeout = ( u16 )luaL_checkinteger( L, 3 );
    
  // Execute operation   
  luaL_buffinit( L, &b );
  while( size )
  {
    toread = UMIN( size, WIZNET_PACKET_SIZE );  
    bread = wizc_read( s, &databuf, toread, timeout );
    if( bread > 0 )
      luaL_addlstring( &b, ( const char* )databuf, bread );
    if( bread != toread )
      break;
    size -= toread;
  }  
  luaL_pushresult( &b );
  return 1;
}

// Lua: replies = ping( ip, count, [timeout])
static int wiz_ping( lua_State *L )
{
  u32 ip;
  u8 count;
  u16 timeout = 100;
  
  ip = ( u32 )luaL_checkinteger( L, 1 );
  count = ( u8 )luaL_checkinteger( L, 2 );
  if( lua_isnumber( L, 3 ) )
    timeout = ( u16 )luaL_checkinteger( L, 3 );
  if( timeout == WIZ_INF_TIMEOUT || timeout == WIZ_NO_TIMEOUT )
    return luaL_error( L, "invalid timeout value" );  
  lua_pushinteger( L, wizc_ping( ip, count, timeout ) );
  return 1;
}

// Lua: errcode = http_request( s, url, [timeout] )
static int wiz_http_request( lua_State *L )
{
  int s;
  const char* url;
  u16 timeout = WIZ_INF_TIMEOUT;
      
  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );
  if( wiz_sock_type[ s ] != WIZ_SOCK_TCP )
    return luaL_error( L, "invalid socket type for http_request (must be TCP)" );
  url = luaL_checkstring( L, 2 );
  if( lua_isnumber( L, 3 ) )
    timeout = ( u16 )luaL_checkinteger( L, 3 );
    
  // Execute operation and push back result
  lua_pushinteger( L, wizc_http_request( s, url, timeout ) );
  return 1;
}

// *****************************************************************************
// expect will read a TCP/UDP data stream until the specified tag is found. It
// can discard the data found while looking for the tag, or return it 

// Search states
#define EXPECT_MAX_GUARD_LEN        64

typedef struct
{
  s16 total;
  u8 *pdata;
  u16 lastport;
  u32 lastip;
} EXPECT_BUF;

static EXPECT_BUF expect_buf;
static char expect_guard_data[ EXPECT_MAX_GUARD_LEN + 1 ];

// Helper: init internal data structures
static void wizh_expect_init()
{
  memset( &expect_buf, 0, sizeof( EXPECT_BUF ) );
  memset( expect_guard_data, 0, EXPECT_MAX_GUARD_LEN + 1 );
}

// Helper: return a single byte from the buffer
static int wizh_get_next_byte( u8 sockno, u8 socktype, u16 timeout )
{  
  u16 port;
  u32 ip;
  s16 res;
  int c = -1;
  
  if( expect_buf.total == 0 )
  {
    if( socktype == WIZ_SOCK_TCP )
      res = wizc_read( sockno, &expect_buf.pdata, WIZNET_PACKET_SIZE, timeout );
    else
    {
      res = wizc_recvfrom( sockno, &ip, &port, &expect_buf.pdata, WIZNET_PACKET_SIZE, timeout );
      if( expect_buf.lastport != 0 && expect_buf.lastport != port )
        res = 0;
      else if( expect_buf.lastip != 0 && expect_buf.lastip != ip )
        res = 0;
      else
      {
        expect_buf.lastip = ip;
        expect_buf.lastport = port;      
      }  
    }      
    if( res > 0 )
      expect_buf.total = res;    
  }
  if( expect_buf.total > 0 )
  {
    c = *expect_buf.pdata ++;
    expect_buf.total --;
  }  
  return c;
}

// Helper for both 'expect' and 'expect_and_read'
static int wiz_expect_helper( lua_State *L, int accumulate )
{
  int s;
  const char *tag;
  u16 timeout = WIZ_INF_TIMEOUT;
  u8 finished, compidx;
  luaL_Buffer b;
  int c;

  s = luaL_checkinteger( L, 1 );
  if( s < 0 || s >= WIZ_NUM_MAX_SOCKETS )
    return luaL_error( L, "invalid socket" );  
  tag = luaL_checkstring( L, 2 );
  if( strlen( tag ) == 0 )
    return luaL_error( L, "non-empty tag expected" );
  if( strlen( tag ) > EXPECT_MAX_GUARD_LEN )
    return luaL_error( L, "the maximum length of the end guard is %d", EXPECT_MAX_GUARD_LEN );  
  if( lua_isnumber( L, 3 ) )
    timeout = ( u16 )luaL_checkinteger( L, 3 );
  
  // Start looking for the string
  if( accumulate )   
    luaL_buffinit( L, &b );
  finished = compidx = 0;
  memset( expect_guard_data, 0, EXPECT_MAX_GUARD_LEN + 1 );
  while( !finished )
  {
    if( ( c = wizh_get_next_byte( s, wiz_sock_type[ s ], timeout ) ) == -1 )
      break;
    if( c == tag[ compidx ] )
    {
      expect_guard_data[ compidx ] = c;
      compidx ++;
      if( compidx == strlen( tag ) )
        finished = 1;
    }
    else
    {
      if( strlen( expect_guard_data ) > 0 )
      {
        if( accumulate )
          luaL_addstring( &b, expect_guard_data );
        memset( expect_guard_data, 0, EXPECT_MAX_GUARD_LEN + 1 );
      }
      if( accumulate )
        luaL_addchar( &b, ( char )c );
      compidx = 0;      
    }        
  }   
  if( accumulate )
  {
    if( finished )
      luaL_pushresult( &b );
    else
      lua_pushnil( L );
  }
  else
    lua_pushboolean( L, finished );
  return 1;
}

// Lua: expect_start()
static int wiz_expect_start( lua_State *L )
{
  memset( &expect_buf, 0, sizeof( expect_buf ) );
  memset( expect_guard_data, 0, EXPECT_MAX_GUARD_LEN + 1 );
  return 0; 
}

// Lua: found = expect( s, string, [timeout] )
static int wiz_expect( lua_State *L )
{
  return wiz_expect_helper( L, 0 );
}

// Lua: str = expect_and_read( s, string, [timeout] )
static int wiz_expect_and_read( lua_State *L )
{
  return wiz_expect_helper( L, 1 );
}

// *****************************************************************************
// Application web interface

// Lua: boolres = app_web_set( appname, page_str|FILE_userdata, [arg_table] )
static int wiz_app_web_set( lua_State *L )
{
  const char* app_name;
  const char* app_page = NULL;
  FILE *fp = NULL;
  u16 total_size, towrite;
  u32 fpos;
  unsigned i;
  
  app_name = luaL_checkstring( L, 1 );
  if( lua_isstring( L, 2 ) )
    app_page = luaL_checkstring( L, 2 );
  else
    fp = *( ( FILE ** )luaL_checkudata( L, 2, LUA_FILEHANDLE ) );
  
  // Initialize app web
  if( app_page )
    total_size = strlen( app_page );
  else
  {
    fpos = ftell( fp );
    fseek( fp, 0, SEEK_END );
    total_size = ftell( fp );
    fseek( fp, 0, fpos );      
  }
  if( total_size == 0 )
    return luaL_error( L, "the web page template cannot be empty" );
  wizc_app_init( app_name );
    
  // Send the page in chunks of maximum WIZNET_PACKET_SIZE
  while( total_size )
  {
    towrite = UMIN( total_size, WIZNET_PACKET_SIZE - 1 );
    if( app_page )
    {
      memcpy( wiz_buffer + WIZNET_APP_PAGE_BUF_OFFSET, app_page, towrite );
      app_page += towrite;
    }
    else
    {
      if( fread( wiz_buffer + WIZNET_APP_PAGE_BUF_OFFSET, 1, towrite, fp ) != towrite )
        return luaL_error( L, "file I/O error" );  
    }
    wiz_buffer[ WIZNET_APP_PAGE_BUF_OFFSET + towrite ] = '\0';
    wizc_app_set_page( NULL, towrite + 1 );
    total_size -= towrite;
  }
  
  // Send the arguments if needed
  if( lua_istable( L, 3 ) )
  {
    lua_settop( L, 3 );  
    total_size = lua_objlen( L, -1 );
    for( i = 1; i <= total_size; i ++ )
    {
      lua_rawgeti( L, -1, i );
      wizc_app_set_arg( luaL_checkstring( L, -1 ) );
      lua_pop( L, 1 );
    }
  }
  
  // Signal end of request
  lua_pushboolean( L, wizc_app_end_page() );
  return 1;
}

// Lua: boolres = app_web_unload()
static int wiz_app_web_unload( lua_State *L )
{
  lua_pushboolean( L, wizc_app_init( "" ) );
  return 1;  
}

// Lua: boolres = app_has_cgi()
static int wiz_app_has_cgi( lua_State *L )
{
  lua_pushboolean( L, wizc_get_cgi_flag() );
  return 1;
}

// *****************************************************************************
// CGI data handling

static int wiz_from_hex_digit( char c )
{
  return c <= '9' ? c - '0' : toupper( c ) - 'A' + 10;
}

static void wiz_cgi_arg_to_ascii( char *dest, const char* src, unsigned maxlen )
{
  unsigned i, j, totlen = strlen( src );

  i = j = 0;
  while( i < totlen && j < maxlen )
  {
    if( src[ i ] != '%' )
      dest[ j ++ ] = src[ i ++ ];
    else
    {
      dest[ j ++ ] = ( wiz_from_hex_digit( src[ i + 1 ] ) << 4 ) + wiz_from_hex_digit( src[ i + 2 ] );
      i += 3;
    }
  }
  dest[ j ] = '\0';
}

// Lua: partable = app_get_cgi() (returns nil for error)
static int wiz_app_get_cgi( lua_State *L )
{
  char *cgidata;
  char *p, *p1, *p2;
  char tempbuf[ 65 ];
  
  if( wizc_get_cgi_flag() == 0 )
  {
    lua_pushnil( L );
    return 1;
  }
  printf( "[wiz_app_get_cgi] Before app_get_cgi()\n" );
  if( ( cgidata = wizc_app_get_cgi() ) == NULL )
  {
    lua_pushnil( L );
    return 1;
  }
  printf( "[wiz_app_get_cgi] After app_get_cgi()\n" );
  // Parse the CGI data, save results in a new table
  lua_newtable( L );
  p = cgidata;
  while( p )
  {
    if( ( p1 = strchr( p, '&' ) ) != NULL )
      *p1 = '\0';
    if( ( p2 = strchr( p, '=' ) ) == NULL )
    {
      lua_pushnil( L );
      return 1;
    }      
    *p2 = '\0';
    wiz_cgi_arg_to_ascii( tempbuf, p, 64 );
    lua_pushstring( L, tempbuf );
    wiz_cgi_arg_to_ascii( tempbuf, p2 + 1, 64 );
    lua_pushstring( L, tempbuf );
    lua_rawset( L, -3 );          
    p = p1 == NULL ? NULL : p1 + 1;    
  }
  return 1;
}

// *****************************************************************************
// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE wiz_map[] = 
{
  { LSTRKEY( "ip" ), LFUNCVAL( wiz_ip ) },
  { LSTRKEY( "unpackip" ), LFUNCVAL( wiz_unpackip ) },
  { LSTRKEY( "socket" ), LFUNCVAL( wiz_socket ) },
  { LSTRKEY( "close" ), LFUNCVAL( wiz_close ) },
  { LSTRKEY( "sendto" ), LFUNCVAL( wiz_sendto ) },
  { LSTRKEY( "recvfrom" ), LFUNCVAL( wiz_recvfrom ) },  
  { LSTRKEY( "lookup" ), LFUNCVAL( wiz_lookup ) },
  { LSTRKEY( "connect" ), LFUNCVAL( wiz_connect ) },
  { LSTRKEY( "send" ), LFUNCVAL( wiz_send ) },
  { LSTRKEY( "recv" ), LFUNCVAL( wiz_recv ) },
  { LSTRKEY( "ping" ), LFUNCVAL( wiz_ping ) },
  { LSTRKEY( "http_request" ), LFUNCVAL( wiz_http_request ) },
  { LSTRKEY( "expect_start" ), LFUNCVAL( wiz_expect_start ) },
  { LSTRKEY( "expect" ), LFUNCVAL( wiz_expect ) },
  { LSTRKEY( "expect_and_read" ), LFUNCVAL( wiz_expect_and_read ) },
  { LSTRKEY( "app_web_set" ), LFUNCVAL( wiz_app_web_set ) },
  { LSTRKEY( "app_web_unload" ), LFUNCVAL( wiz_app_web_unload ) },
  { LSTRKEY( "app_has_cgi" ), LFUNCVAL( wiz_app_has_cgi ) },
  { LSTRKEY( "app_get_cgi" ), LFUNCVAL( wiz_app_get_cgi ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "SOCK_TCP" ), LNUMVAL( WIZ_SOCK_TCP ) },
  { LSTRKEY( "SOCK_UDP" ), LNUMVAL( WIZ_SOCK_UDP ) },
  { LSTRKEY( "SOCK_IPRAW" ), LNUMVAL( WIZ_SOCK_IPRAW ) },
  { LSTRKEY( "NO_TIMEOUT" ), LNUMVAL( WIZ_NO_TIMEOUT ) },
  { LSTRKEY( "INF_TIMEOUT" ), LNUMVAL( WIZ_INF_TIMEOUT ) },
  { LSTRKEY( "INVALID_IP" ), LNUMVAL( 0 ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_wiz( lua_State *L )
{
  wizc_setup( wiz_buffer );
  wizh_expect_init();
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
#error "the wiz module doesn't support LUA_OPTIMIZE_MEMORY == 0"
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}


