// GUI font functions

#include "font.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "utils.h"

EXC_DECLARE;

// Free a font structure
void font_free( FONT *pfont )
{
  int i;
  FONT_CHAR *pchar;
  
  if( pfont && pfont->pchars )
  {
    for( i = 0; i < pfont->num_chars; i ++ )
    {
      pchar = ( FONT_CHAR* )pfont->pchars + i;
      if( pchar->data )
        free( ( u8* )pchar->data );
    }
  }
  if( pfont && pfont->pchars )
    free( ( FONT_CHAR* )pfont->pchars );      
  if( pfont )
    free( pfont );
}  

// Loads a font from a file
// File format : 
//   number of chars - one byte
//   (char descriptor)*
FONT *font_load_from_file( const char* pname )
{
  FILE *fp = NULL;
  FONT *pfont = NULL;
  FONT_CHAR *pchar = NULL;
  int i;
  u32 total;
  
  EXC_TRY
  {
    if( ( fp = fopen( pname, "rb" ) ) == NULL )
      EXC_THROW();
    if( ( pfont = ( FONT* )malloc( sizeof( FONT ) ) ) == NULL )
      EXC_THROW();
    memset( pfont, 0, sizeof( FONT ) );
            
    // Read the total number of chars
    fread( &pfont->num_chars, 1, 1, fp );
    //printf("NUM CHARS: %d\n", pfont->num_chars );
    if( ( pfont->pchars = ( const FONT_CHAR* )malloc( pfont->num_chars * sizeof( FONT_CHAR ) ) ) == NULL )
      EXC_THROW();
    memset( ( FONT_CHAR* )pfont->pchars, 0, pfont->num_chars * sizeof( FONT_CHAR ) );
    
    // Read all the chars
    for( i = 0; i < pfont->num_chars; i ++ )
    {
      pchar = ( FONT_CHAR* )pfont->pchars + i;
      fread( &pchar->code, 1, 1, fp );
      fread( &pchar->w, 1, 1, fp );
      fread( &pchar->h, 1, 1, fp );
      //printf("DEBUG: code=%d, w=%d, h=%d\n", pchar->code, pchar->w, pchar->h );
      // Compute the total number of bytes used for encoding the char
      total = pchar->w * pchar->h;
      total = ( total >> 3 ) + ( total & 7 ? 1 : 0 );
      if( ( pchar->data = ( const u8* )malloc( total ) ) == NULL )
        EXC_THROW();
      fread( ( u8* )pchar->data, 1, total, fp );
    }
    fclose( fp );
  }
  EXC_CATCH
  {
    if( fp )
      fclose( fp );
    font_free( pfont );
    pfont = NULL;
  }
  return pfont;
}

// Get a char from the font
const FONT_CHAR* font_get_char( const FONT* pfont, u8 code )
{
  unsigned i;
  const FONT_CHAR *pchar;
  
  for( i = 0; i < pfont->num_chars; i ++ )
  {
    pchar = pfont->pchars + i;
    if( pchar->code == code )
      return pchar;
  }
  return NULL;
}
