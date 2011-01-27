// GUI font functions

#ifndef __FONT_H__
#define __FONT_H__

#include "type.h"

// Font char
typedef struct 
{
  u8 code, w, h;
  const u8* data;
} FONT_CHAR;

// Font
typedef struct 
{
  u8 num_chars;
  const FONT_CHAR *pchars;
} FONT;

FONT* font_load_from_file( const char* pname ); // loads a font from a file
const FONT_CHAR* font_get_char( const FONT* pfont, u8 code ); // get a char from the font
void font_free( FONT *pfont ); // free a font structure  

#endif

