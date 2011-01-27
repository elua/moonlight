// GUI bitmap functions

#include "bitmap.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "utils.h"

EXC_DECLARE;

// Free an image structure
void bitmap_free( BITMAP *pbitmap )
{
  if( pbitmap && pbitmap->pdata )
    free( ( u8* )pbitmap->pdata );
  if( pbitmap )
    free( pbitmap );
}

// Loads a bitmap from a file
// File format : 
//   number of chars - one byte
//   (char descriptor)*
BITMAP *bitmap_load_from_file( const char* pname )
{
  FILE *fp = NULL;
  BITMAP *pbitmap = NULL;
  u32 total;
  
  EXC_TRY
  {
    if( ( fp = fopen( pname, "rb" ) ) == NULL )
      EXC_THROW();
    if( ( pbitmap = ( BITMAP* )malloc( sizeof( BITMAP ) ) ) == NULL )
      EXC_THROW();
    memset( pbitmap, 0, sizeof( BITMAP ) );
            
    // Read the width and the height
    fread( &pbitmap->width, 1, 1, fp );
    fread( &pbitmap->height, 1, 1, fp );
    total = pbitmap->width * pbitmap->height;
    total = ( total >> 3 ) + ( total & 7 ? 1 : 0 );
    if( ( pbitmap->pdata = ( const u8* )malloc( total ) ) == NULL )
      EXC_THROW();
    fread( ( u8* )pbitmap->pdata, total, 1, fp );
    fclose( fp );
  }
  EXC_CATCH
  {
    if( fp )
      fclose( fp );
    bitmap_free( pbitmap );
    pbitmap = NULL;
  }
  return pbitmap;
}
