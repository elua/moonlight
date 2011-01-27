// Various generic utilities

#ifndef __UTILS_H__
#define __UTILS_H__

#include "type.h"

void utils_base64_decode( char* dest, const char* src, u16 maxlen );
char utils_to_hex_digit( u8 n );
int utils_from_hex_digit( char c );
int utils_string_to_ip( const char *ipstr, u8 *pip );
int utils_string_to_mac( const char *macstr, u8 *pmac );

#endif
