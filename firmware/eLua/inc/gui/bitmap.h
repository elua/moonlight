// GUI bitmap functions

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "type.h"

// Bitmap
typedef struct 
{
  u8 width, height;
  const u8 *pdata;
} BITMAP;

BITMAP* bitmap_load_from_file( const char* pname ); // loads a bitmap from a file
void bitmap_free( BITMAP* pbitmap ); // frees the memory associated with a bitmap

#endif
