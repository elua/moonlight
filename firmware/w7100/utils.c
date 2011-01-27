// Various generic utilities

#include "utils.h"
#include "types.h"
#include <string.h>
#include <ctype.h>

// Decode the given Base64-encoded string
// Taken from http://base64.sourceforge.net/b64.c
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/
static void decodeblock( unsigned char* in, unsigned char* out )
{   
  out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
  out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
  out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

void utils_base64_decode( char* dest, const char* src, u16 maxlen )
{
  unsigned char in[4], out[3], v;
  u16 i,  stridx = 0, outidx = 0;

  while( stridx < strlen( src ) ) 
  {
    for( i = 0; i < 4; i++ ) 
    {
      v = (unsigned char) src[ stridx++ ];
      v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
      v = (unsigned char) ((v == '$') ? 0 : v - 61);
      in[ i ] = v ? (unsigned char) (v - 1) : 0;
    }
    decodeblock( in, out );
    for( i = 0; i < 3; i++ ) 
    {
      dest[ outidx ++ ] = out[ i ];
      if( outidx == maxlen  )
      {
        dest[ outidx ] = 0;
        return;
      }
    }
  }
  dest[ outidx ] = 0;
}

char utils_to_hex_digit( u8 n )
{
  return n < 10 ? n + '0' : n - 10 + 'A';
}

int utils_from_hex_digit( char c )
{
  return c <= '9' ? c - '0' : toupper( c ) - 'A' + 10;
}

// Convert a string representation of an IP to its numeric format,
// returns 1 for success and 0 for error
int utils_string_to_ip( const char* ipstr, u8 *pip )
{
  uint8 i, is_ip, ipidx, numcnt;
  uint16 crtnum;
  
  if( strlen( ipstr ) < 7 )
    return 0;   
  is_ip = 1;
  crtnum = ipidx = numcnt = 0;
  if( isdigit( ipstr[ 0 ] ) && isdigit( ipstr[ strlen( ipstr ) - 1 ] ) )
    for( i = 0; i <= strlen( ipstr ) && is_ip; i ++ )
    {
      if( i == strlen( ipstr ) )
      {
        if( crtnum > 255 )
          is_ip = 0;
        else
          pip[ 3 ] = crtnum;
      }
      else if( ipstr[ i ] != '.' && !isdigit( ipstr[ i ] ) )
        is_ip = 0;
      else
      {
        if( isdigit( ipstr[ i ] ) )
        {
          if( ++ numcnt == 4 )
            is_ip = 0;
          else
            crtnum = crtnum * 10 + ipstr[ i ] - '0';
        }
        else
        {     
          if( ipstr[ i - 1 ] == '.' )
            is_ip = 0;
          else
          {      
            if( crtnum > 255 )
              is_ip = 0;
            else
            {
              pip[ ipidx ] = ( uint8 )crtnum;
              numcnt = crtnum = 0;
              if( ++ ipidx == 4 )
                is_ip = 0;
            }
          }
        }
      }
    }
  else
    is_ip = 0;
  return is_ip;
}

// Convert a string representation of a MAC address to its numeric format,
// returns 1 for success and 0 for error
int utils_string_to_mac( const char *macstr, u8 *pmac )
{
  u8 j;
  char *p;

  if( strlen( macstr ) != 17 )
    return 0;
  p = macstr;
  j = 0;
  while( 1 )
  { 
    if( !isxdigit( *p ) || !isxdigit( p[ 1 ] ) )
      return 0;
    pmac[ j ++ ] = ( utils_from_hex_digit( *p ) << 4 ) + utils_from_hex_digit( p[ 1 ] );
    if( p[ 2 ] == '\0' )
      break;
    if( p[ 2 ] != ':' )
      return 0;
    p += 3;
  }
  return 1;
}
