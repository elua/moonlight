// Driver for the multi-32x8 display

#include "type.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x.h"
#include "platform.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "modcommon.h"
#include "lrotable.h"
#include "font.h"
#include "bitmap.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "platform_conf.h"

#ifdef ELUA_BOARD_WIZ

// ****************************************************************************
// Display data

// Display definitions
#define SIZEX_PER_DISP          32
#define SIZEY_PER_DISP          8
#define NUM_X_DISPLAYS          4
#define NUM_Y_DISPLAYS          4

#define SIZEX                   ( NUM_X_DISPLAYS * SIZEX_PER_DISP )
#define SIZEY                   ( NUM_Y_DISPLAYS * SIZEY_PER_DISP )
#define VMEM_SIZE_PER_DISP      ( ( SIZEX_PER_DISP >> 5 ) * SIZEY_PER_DISP )
#define VMEM_SIZE               ( VMEM_SIZE_PER_DISP * NUM_X_DISPLAYS * NUM_Y_DISPLAYS )
#define DISP_VMEM_PTR( p, no )  ( p + ( no ) * VMEM_SIZE_PER_DISP ) 
#define NUM_TOTAL_DISPLAYS      ( NUM_X_DISPLAYS * NUM_Y_DISPLAYS )

#define SCR_X_TO_DISP_X( x )    ( ( x ) & 0x1F )
#define SCR_Y_TO_DISP_Y( y )    ( ( y ) & 0x07 )
#define SCR_XY_TO_DISP_VMEM_IDX( x, y ) ( ( ( y ) >> 3 ) * NUM_X_DISPLAYS + ( ( x ) >> 5 ) )
#define DISP_X_TO_ARR_IDX( x )  ( 7 - ( ( x ) >> 2 ) )
#define DISP_XY_TO_MASK( x, y ) ( ( ( 3 - ( ( x ) & 3 ) ) << 3 ) + ( 7 - y ) )

// Physical connection data
//    PC0 - A0
//    PC1 - A1
//    PC2 - A2
//    PC3 - A3
//    PC4 - CLOCK (WR)
//    PC5 - DATA 
//    PC6 - EN (74154 enable pin)

// Pin definitions
#define CLOCK_PIN               4
#define DATA_PIN                5
#define EN_PIN                  6

// Bit banding data
#define BB_PERIPH_ALIAS_BASE    0x42000000
#define BB_PERIPH_BASE          0x40000000
#define GPIOC_ODR_ADDR          0x4001100C
#define BB_EN_PIN_ADDR          ( BB_PERIPH_ALIAS_BASE + ( GPIOC_ODR_ADDR - BB_PERIPH_BASE ) * 32 + EN_PIN * 4 )
#define BB_DATA_PIN_ADDR        ( BB_PERIPH_ALIAS_BASE + ( GPIOC_ODR_ADDR - BB_PERIPH_BASE ) * 32 + DATA_PIN * 4 )
#define BB_CLOCK_PIN_ADDR       ( BB_PERIPH_ALIAS_BASE + ( GPIOC_ODR_ADDR - BB_PERIPH_BASE ) * 32 + CLOCK_PIN * 4 )

#define BB_EN                   *( volatile u32* )BB_EN_PIN_ADDR
#define BB_DATA                 *( volatile u32* )BB_DATA_PIN_ADDR
#define BB_CLOCK                *( volatile u32* )BB_CLOCK_PIN_ADDR

// 'action' argument to drv_disp_select
#define DISP_SELECT             1
#define DISP_UNSELECT           0

// 'type' argument to drv_putpixel
#define PIXEL_ON                1
#define PIXEL_OFF               0

// Video memory
static u32 vmem[ VMEM_SIZE ];

// Display mapping (takes into account the 74154 selection lines)
static const u8 disp_map[] = 
{ 
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11, 
  15, 12, 13, 14
};

// ****************************************************************************
// GPIO display driver

void drv_to_disp( u32 n, int bits )
{
  for( ; bits; bits --, n >>= 1 )
  {
    BB_CLOCK = 0;
    BB_DATA = n & 1;
    BB_CLOCK = 1;
  }
}

// Select or unselect the given display
static void drv_disp_select( int dispno, int action )
{
  if( action == DISP_UNSELECT )
    BB_EN = 1;
  else
  {
    GPIOC->ODR = ( GPIOC->ODR & ~0x0F ) | disp_map[ dispno ];
    BB_EN = 0;
  }
}

// Send command to display
static void drv_disp_send_command( int dispno, u16 command )
{
  drv_disp_select( dispno, DISP_SELECT );
  drv_to_disp( command, 12 );
  drv_disp_select( dispno, DISP_UNSELECT );
}

// Send data to display
static void drv_disp_send_data( int dispno )
{
  unsigned i;
  const u32* pdata = DISP_VMEM_PTR( vmem, dispno );

  drv_disp_select( dispno, DISP_SELECT );
  // Send "101b" (write command)
  drv_to_disp( 5, 3 );
  // Send address 0 (7 bits)
  drv_to_disp( 0, 7 );
  // Send all data
  for( i = 0; i < VMEM_SIZE_PER_DISP; i ++ )
    drv_to_disp( *pdata ++, 32 );
  drv_disp_select( dispno, DISP_UNSELECT );
}

// Setup the given display
static void drv_disp_setup( int dispno )
{
  // All commands are sent in LE mode
  // SYS EN - turn on system oscillator - 100 0000 0001 0 - 0100 0000 0001
  drv_disp_send_command( dispno, 0b010000000001 );
  // LED ON - turn on LED duty cycle generator - 100 0000 0011 0 - 0110 0000 0001
  drv_disp_send_command( dispno, 0b011000000001 );
  // BLINK OFF - turn off blinking - 100 0000 1000 0 - 0000 1000 0001
  drv_disp_send_command( dispno, 0b000010000001 );
  // MASTER MODE - ser master mode - 100 0001 0111 0 - 0111 0100 0001
  drv_disp_send_command( dispno, 0b011101000001 );
  // RC - system clock from RC oscillator - 100 0001 1011 0 - 0110 1100 0001
  drv_disp_send_command( dispno, 0b011011000001 );
  // COMMON OPTIONS - P-MOS drain output, 8 common - 100 0010 1011 0 - 0110 1010 0001
  drv_disp_send_command( dispno, 0b011010100001 );
  // PWM duty - 16/16 - 100 1011 1111 0 - 0111 1110 1001
  drv_disp_send_command( dispno, 0b011111101001 );
}

// Setup the intensity (from 0 to 15) of the given display
// Array mapping between a 4 bit integer and its reversed endian counterpart
static u8 drv_intensity_table[] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
static void drv_disp_set_intensity( int dispno, int intensity )
{
  u16 cmd = 0b000001101001 | ( drv_intensity_table[ intensity ] << 7 );

  drv_disp_send_command( dispno, cmd );
}

void drv_disp_init()
{
  GPIO_InitTypeDef gpio;
  unsigned i;

  // GPIO init
  BB_EN = 1;
  gpio.GPIO_Pin = 0x7F; // PC0 to PC6
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( GPIOC, &gpio );
  
  // Init all displays
  for( i = 0; i < NUM_TOTAL_DISPLAYS; i ++ )
    drv_disp_setup( i );

  // Then clear them
  for( i = 0; i < NUM_TOTAL_DISPLAYS; i ++ )
    drv_disp_send_data( i );
}

// ****************************************************************************
// Logical display driver

// Write modes
#define DISP_WRITE_OVERWRITE    0
#define DISP_WRITE_OR           1
#define DISP_WRITE_AND          2
#define DISP_WRITE_XOR          3

// Viewport types
#define DISP_VP_NONE            0
#define DISP_VP_NORMAL          1
#define DISP_VP_INVERTED        2

static u16 disp_mod_mask;
static int disp_write_mode = DISP_WRITE_OVERWRITE;
// Viewport data
static int disp_vp_mode = DISP_VP_NONE;
static int disp_vp_top, disp_vp_left, disp_vp_right, disp_vp_bottom;
// Fonts
#define DISP_FONT_5x7           0
#define DISP_FONT_5x5_ROUND     1
#define DISP_FONT_5x5_SQUARE    2
#define DISP_FONT_3x5           3
#define DISP_FONT_4x6           4
#define FONT_META_NAME          "eLua.dp105fnt"
#define font_check( L )         ( font_t* )luaL_checkudata( L, 1, FONT_META_NAME )
typedef struct 
{
  FONT *pfont;
} font_t;
extern const FONT type_writer_font;
extern const FONT fnt5x5_rounded_font;
extern const FONT font5x5_font;
extern const FONT pzim3x5_font;
extern const FONT f6_font;
static const FONT* disp_font = &type_writer_font;
static const FONT* const disp_fixed_fonts[] = { &type_writer_font, &fnt5x5_rounded_font, &font5x5_font, &pzim3x5_font, &f6_font };

// Bitmaps
typedef struct 
{
  BITMAP *pbitmap;
} bitmap_t;
#define BITMAP_META_NAME      "eLua.dp105bmp"
#define bitmap_check( L, n )  ( bitmap_t* )luaL_checkudata( L, ( n ), BITMAP_META_NAME )

// Put a pixel at the given location with the specified write mode
// (taking into account the viewport if one is specified)
static void disp_putpixel( int x, int y, int col, int mode )
{
  if( x < 0 || x >= SIZEX || y < 0 || y >= SIZEY )
    return;
  if( disp_vp_mode != DISP_VP_NONE ) 
  {
     if( disp_vp_mode == DISP_VP_NORMAL && ( x < disp_vp_left || x > disp_vp_right || y < disp_vp_top || y > disp_vp_bottom ) )
       return;
     if( disp_vp_mode == DISP_VP_INVERTED && ( x >= disp_vp_left && x <= disp_vp_right && y >= disp_vp_top && y <= disp_vp_bottom ) )
       return;
  }
  u32 idx, mask, prevv;
  int disp_idx = SCR_XY_TO_DISP_VMEM_IDX( x, y );
  u32 *localmem = DISP_VMEM_PTR( vmem, disp_idx );
  int pcol;

  x = SCR_X_TO_DISP_X( x );
  y = SCR_Y_TO_DISP_Y( y );
  idx = DISP_X_TO_ARR_IDX( x );
  mask = 1 << DISP_XY_TO_MASK( x, y );
  prevv = localmem[ idx ];
  pcol = ( prevv & mask ) ? PIXEL_ON : PIXEL_OFF;
  if( mode != DISP_WRITE_OVERWRITE )
  {
    switch( mode )
    {
      case DISP_WRITE_OR:
        col = pcol | col;
        break;
      case DISP_WRITE_AND:
        col = pcol & col;
        break;
      case DISP_WRITE_XOR:
        col = pcol ^ col;
        break;
    }
  }
  if( pcol != col )
  {
    if( col == PIXEL_ON )
      localmem[ idx ] = prevv | mask;
    else
      localmem[ idx ] = prevv & ~mask;
    disp_mod_mask |= 1 << disp_idx;
  }
}

// Return the value of the pixel at the given coordinates
static int disp_getpixel( int x, int y )
{
  if( x < 0 || x >= SIZEX || y < 0 || y >= SIZEY )
    return PIXEL_OFF;
  u32 idx, mask;;
  int disp_idx = SCR_XY_TO_DISP_VMEM_IDX( x, y );
  u32 *localmem = DISP_VMEM_PTR( vmem, disp_idx );

  x = SCR_X_TO_DISP_X( x );
  y = SCR_Y_TO_DISP_Y( y );
  idx = DISP_X_TO_ARR_IDX( x );
  mask = 1 << DISP_XY_TO_MASK( x, y );
  return localmem[ idx ] & mask ? PIXEL_ON : PIXEL_OFF;
}

// Clear the screen with the given color
static void disp_clrscr( int col )
{
  memset( vmem, col == PIXEL_OFF ? 0 : 0xFF, VMEM_SIZE * sizeof( u32 ) );
  disp_mod_mask = ( 1 << NUM_TOTAL_DISPLAYS ) - 1;
} 

// End draw
static void disp_draw()
{
  unsigned i;
  
  // Check for changes, update only the changed displays
  for( i = 0; i < NUM_TOTAL_DISPLAYS; i ++ )
    if( disp_mod_mask & ( 1 << i ) )
      drv_disp_send_data( i );
  disp_mod_mask = 0;
}

// Initialize the driver
static void disp_init()
{
  disp_write_mode = DISP_WRITE_OVERWRITE;
  disp_vp_mode = DISP_VP_NONE;
  disp_vp_top = disp_vp_left = disp_vp_right = disp_vp_bottom = 0;
  disp_font = &type_writer_font;
  disp_clrscr( 0 );
  disp_draw();
}

// Set write mode
static void disp_set_write_mode( int mode )
{
  disp_write_mode = mode;
}

// Set viewport
static void disp_set_viewport( int left, int top, int right, int bottom, int mode )
{
  disp_vp_left = left;
  disp_vp_top = top;
  disp_vp_right = right;
  disp_vp_bottom = bottom;
  disp_vp_mode = mode;  
}

// Draw a line
static void disp_line( int x0, int y0, int x1, int y1, int col, int mode )
{
  int temp;
  
  if( x0 == x1 ) // Vertical line
  {
    if( y0 > y1 )
    {
      temp = y0;
      y0 = y1;
      y1 = temp;
    }
    for( temp = y0; temp <= y1; temp ++ )
      disp_putpixel( x0, temp, col, mode );
  }
  else if( y0 == y1 ) // Horizontal line
  {
    if( x0 > x1 )
    {
      temp = x0;
      x0 = x1;
      x1 = temp;
    }
    for( temp = x0; temp <= x1; temp ++ )
      disp_putpixel( temp, y0, col, mode );
  }
  else
  {
    int steep = abs( y1 - y0 ) > abs( x1 - x0 );
    if( steep )
    {
      unsigned long temp = x0;
      x0 = y0;
      y0 = temp;
      temp = x1;
      x1 = y1;
      y1 = temp;
    }
    if( x0 > x1 )
    {
      unsigned long temp = x0;
      x0 = x1;
      x1 = temp;
      temp = y0;
      y0 = y1;
      y1 = temp;
    }
    long deltax = x1 - x0;
    long deltay = abs( y1 - y0 );
    long error = deltax >> 1; 
    long ystep;
    unsigned long x;
    long y = y0;
    if( y0 < y1 ) 
      ystep = 1;
    else 
      ystep = -1;
    if(steep)
      for( x = x0; x < x1; x++ )
      {
        disp_putpixel( y, x, col, mode );
        error -= deltay;
        if( error < 0 )
        {
          y += ystep;
          error += deltax;
        }
      }
    else
      for( x = x0; x < x1; x++ )
      {
        disp_putpixel( x, y, col, mode );
        error -= deltay;
        if(error < 0)
        {
          y += ystep;
          error += deltax;
        }
      }
  }
}

// Draw a rectangle
static void disp_rectangle( int left, int top, int right, int bottom, int col, int mode )
{
  disp_line( left, top, right, top, col, mode );
  disp_line( right, top, right, bottom, col, mode );
  disp_line( right, bottom, left, bottom, col, mode );
  disp_line( left, bottom, left, top, col, mode );
}

// Draw a filled rectangle
static void disp_filled_rectangle( int left, int top, int right, int bottom, int col, int mode )
{
  int i;
  
  for( i = left; i <= right; i ++ )
    disp_line( i, top, i, bottom, col, mode );
}

static void disp_putstr( int x, int y, const char* str, int mode )
{
  unsigned xx, yy, i;
  const FONT_CHAR *pc;
  u32 bcnt;
  u8 bmask;
  int col;
  
  for( i = 0; i < strlen( str ); i ++ )
  {
    if( ( pc = font_get_char( disp_font, str[ i ] ) ) == NULL )
      continue;
    bcnt = 0;
    bmask = 1;
    if( ( x + ( int )pc->w ) >= 0 && x < SIZEX && ( y + ( int )pc->h ) >= 0 && y < SIZEY )
      for( yy = 0; yy < pc->h; yy ++ )
        for( xx = 0; xx < pc->w; xx ++ )
        { 
          col = pc->data[ bcnt ] & bmask ? 1 : 0;
          disp_putpixel( x + xx, y + yy, col, mode );
          bmask <<= 1;
          if( bmask == 0 )
          {
            bmask = 1;
            bcnt ++;
          }
        }
    x += pc->w + 1;
    if( str[ i ] == ' ' && pc->w < 2 )
      x += 2 - pc->w;
    if( x >= SIZEX )
      break;
  }
}

static void disp_get_text_extent( const char* str, unsigned *w, unsigned *h )
{
  unsigned i;
  const FONT_CHAR *pc;
  
  *w = *h = 0;  
  for( i = 0; i < strlen( str ); i ++ )
  {
    if( ( pc = font_get_char( disp_font, str[ i ] ) ) == NULL )
      continue;
    *w += pc->w + 1;
    if( str[ i ] == ' ' && pc->w < 2 )
      *w += 2 - pc->w;
    if( pc->h > *h )
      *h = pc->h;      
  }
}

static void disp_set_font( const FONT* pfont )
{
  disp_font = pfont;
}

static void disp_draw_bitmap( int x, int y, BITMAP *pb, int mode )
{
  unsigned xx, yy;
  u8 bmask = 1;
  int col;
  const u8 *pdata = pb->pdata;
  
  for( yy = 0; yy < pb->height; yy ++ )
  {
    if( y + yy >= SIZEY )
      break;
    for( xx = 0; xx < pb->width; xx ++ )
    {
      col = ( *pdata & bmask ) ? 1 : 0;
      disp_putpixel( x + xx, y + yy, col, mode );
      bmask <<= 1;
      if( bmask == 0 )
      {
        bmask = 1;
        pdata ++;
      }       
    }
  }
}

static void disp_circle( int xc, int yc, int r, int col, int mode )
{
  int x = 0; 
  int y = r; 
  int p = 3 - 2 * r;
  
  if( r == 0 )
    return;
    
  while( y >= x )
  {
    disp_putpixel( xc - x, yc - y, col, mode );
    disp_putpixel( xc - y, yc - x, col, mode );
    disp_putpixel( xc + y, yc - x, col, mode );
    disp_putpixel( xc + x, yc - y, col, mode );
    disp_putpixel( xc - x, yc + y, col, mode );
    disp_putpixel( xc - y, yc + x, col, mode );
    disp_putpixel( xc + y, yc + x, col, mode );
    disp_putpixel( xc + x, yc + y, col, mode );
    if( p < 0 ) 
      p += 4 * x++ + 6; 
    else 
      p += 4 * ( x++ - y-- ) + 10; 
  } 
}

// ****************************************************************************
// Lua interface

// Lua: disp.init()
static int dp105_init( lua_State *L )
{
  disp_init();
  return 0;
}

// Lua: disp.draw()
static int dp105_draw( lua_State *L )
{
  disp_draw();
  return 0;
}

// Lua: disp.putpixel( x, y, col, [mode] )
static int dp105_putpixel( lua_State *L )
{
  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );
  int col = luaL_checkinteger( L, 3 );
  int mode;

  if( lua_gettop( L ) >= 4 )
    mode = luaL_checkinteger( L, 4 );
  else
    mode = disp_write_mode;
  disp_putpixel( x, y, col, mode );
  return 0;
}

// Lua: pixel = disp.getpixel( x, y )
static int dp105_getpixel( lua_State *L )
{
  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );

  lua_pushinteger( L, disp_getpixel( x, y ) );
  return 1;
}

// Lua: disp.set_write_mode( mode )
static int dp105_set_write_mode( lua_State *L )
{
  disp_set_write_mode( luaL_checkinteger( L, 1 ) );
  return 0;
}

// Lua: disp.set_viewport( int left, int top, int right, int bottom, int type )
// or set_viewport() to clear
static int dp105_set_viewport( lua_State *L )
{
  int top, left, right, bottom, type;

  if( lua_gettop( L ) == 0 )
    disp_set_viewport( 0, 0, 0, 0, DISP_VP_NONE );
  else
  {
    left = luaL_checkinteger( L, 1 );
    top = luaL_checkinteger( L, 2 );
    right = luaL_checkinteger( L, 3 );
    bottom = luaL_checkinteger( L, 4 );
    type = luaL_checkinteger( L, 5 );
    disp_set_viewport( left, top, right, bottom, type );
  }
  return 0;
}

// Lua: disp.clrscr( col )
static int dp105_clrscr( lua_State *L )
{
  int col = luaL_checkinteger( L, 1 );

  disp_clrscr( col );
  return 0;
}

// Lua: disp.putstr( x, y, string, [mode] )
static int dp105_putstr( lua_State *L )
{
  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );
  const char *str = luaL_checklstring( L, 3, NULL );
  int mode;

  if( lua_gettop( L ) >= 4 )
    mode = luaL_checkinteger( L, 4 );
  else
    mode = disp_write_mode;
  disp_putstr( x, y, str, mode );
  return 0;
}

// Lua: disp.set_row_intensity( row, intensity )
static int dp105_set_row_intensity( lua_State *L )
{
  int row = luaL_checkinteger( L, 1 );
  int intensity = luaL_checkinteger( L, 2 );
  int i;

  row *= NUM_X_DISPLAYS;
  for( i = 0; i < NUM_X_DISPLAYS; i ++ )
    drv_disp_set_intensity( disp_map[ row + i ], intensity );
  return 0;
}

// Lua: disp.line( x1, y1, x2, y2, col, [mode] )
static int dp105_line( lua_State *L )
{
  int x1 = luaL_checkinteger( L, 1 );
  int y1 = luaL_checkinteger( L, 2 );
  int x2 = luaL_checkinteger( L, 3 );
  int y2 = luaL_checkinteger( L, 4 );
  int col = luaL_checkinteger( L, 5 );
  int mode;

  if( lua_gettop( L ) >= 6 )
    mode = luaL_checkinteger( L, 6 );
  else
    mode = disp_write_mode;
  disp_line( x1, y1, x2, y2, col, mode );
  return 0;
}

// Lua: disp.rectangle( left, top, right, bottom, col, [mode] )
static int dp105_rectangle( lua_State *L )
{
  int left = luaL_checkinteger( L, 1 );
  int top = luaL_checkinteger( L, 2 );
  int right = luaL_checkinteger( L, 3 );
  int bottom = luaL_checkinteger( L, 4 );
  int col = luaL_checkinteger( L, 5 );
  int mode;

  if( lua_gettop( L ) >= 6 )
    mode = luaL_checkinteger( L, 6 );
  else
    mode = disp_write_mode;
  disp_rectangle( left, top, right, bottom, col, mode );
  return 0;
}

// Lua: disp.filled_rectangle( left, top, right, bottom, col, [mode] )
static int dp105_filled_rectangle( lua_State *L )
{
  int left = luaL_checkinteger( L, 1 );
  int top = luaL_checkinteger( L, 2 );
  int right = luaL_checkinteger( L, 3 );
  int bottom = luaL_checkinteger( L, 4 );
  int col = luaL_checkinteger( L, 5 );
  int mode;

  if( lua_gettop( L ) >= 6 )
    mode = luaL_checkinteger( L, 6 );
  else
    mode = disp_write_mode;
  disp_filled_rectangle( left, top, right, bottom, col, mode );
  return 0;
}

// Lua: w, h = disp.get_text_extent( text )
static int dp105_get_text_extent( lua_State *L )
{
  unsigned w, h;
  const char* str = luaL_checklstring( L, 1, NULL );
  
  disp_get_text_extent( str, &w, &h );
  lua_pushinteger( L, w );
  lua_pushinteger( L, h );
  return 2;
}

// Lua: disp.set_font( font_id ), or
//      disp.set_font( font )
static int dp105_set_font( lua_State *L )
{ 
  int font_id;
  font_t *f;
  
  if( lua_type( L, 1 ) == LUA_TNUMBER )
  {
    font_id = luaL_checkinteger( L, 1 );
    if( font_id >= sizeof( disp_fixed_fonts ) / sizeof( FONT* ) )
      return luaL_error( L, "invalid font ID" );
    disp_set_font( disp_fixed_fonts[ font_id ] );  
  }
  else
  {
    f = font_check( L );
    disp_set_font( f->pfont );
  }
  return 0;
}

// Lua: font = disp.load_font( name )
static int dp105_load_font( lua_State *L )
{
  const FONT *pf;
  font_t *f;
  const char *fname = luaL_checklstring( L, 1, NULL );
  
  if( ( pf = font_load_from_file( fname ) ) == NULL )
    return luaL_error( L, "cannot load font" );
  f = ( font_t* )lua_newuserdata( L, sizeof( font_t ) );
  f->pfont = ( FONT* )pf;
  luaL_getmetatable( L, FONT_META_NAME );
  lua_setmetatable( L, -2 );  
  return 1;   
}

static int font_gc( lua_State *L )
{
  font_t *f = font_check( L );
  
  if( f && f->pfont )
    font_free( f->pfont );
  return 0; 
}
               
// Lua: bitmap = disp.load_bitmap( name )
static int dp105_load_bitmap( lua_State *L )
{
  BITMAP *pb;
  bitmap_t *b;
  const char *fname = luaL_checklstring( L, 1, NULL );
  
  if( ( pb = bitmap_load_from_file( fname ) ) == NULL )
    return luaL_error( L, "cannot load bitmap" );
  b = ( bitmap_t* )lua_newuserdata( L, sizeof( bitmap_t ) );
  b->pbitmap = ( BITMAP* )pb;
  luaL_getmetatable( L, BITMAP_META_NAME );
  lua_setmetatable( L, -2 );  
  return 1;   
}

static int bitmap_gc( lua_State *L )
{
  bitmap_t *b = bitmap_check( L, 1 );
  
  if( b && b->pbitmap )
    bitmap_free( b->pbitmap );
  return 0;
}

// Lua: disp.draw_bitmap( x, y, bitmap, [mode])
static int dp105_draw_bitmap( lua_State *L )
{
  int x, y;
  bitmap_t *b;
  int mode;
  
  x = luaL_checkinteger( L, 1 );
  y = luaL_checkinteger( L, 2 );
  b = bitmap_check( L, 3 );
  if( lua_gettop( L ) >= 4 )
    mode = luaL_checkinteger( L, 4 );
  else
    mode = disp_write_mode;
  disp_draw_bitmap( x, y, b->pbitmap, mode );
  return 0;
}

// Lua: width, height = disp.get_bitmap_size( bitmap )
static int dp105_get_bitmap_size( lua_State *L )
{
  bitmap_t *b;
  
  b = bitmap_check( L, 1 );
  lua_pushinteger( L, b->pbitmap->width );
  lua_pushinteger( L, b->pbitmap->height );
  return 2;    
}

// Lua: disp.circle( xc, yc, r, col, [mode])
static int dp105_circle( lua_State *L )
{
  int xc, yc, r, col, mode;
  
  xc = luaL_checkinteger( L, 1 );
  yc = luaL_checkinteger( L, 2 );
  r = luaL_checkinteger( L, 3 );
  col = luaL_checkinteger( L, 4 );
  if( lua_gettop( L ) >= 5 )
    mode = luaL_checkinteger( L, 5 );
  else
    mode = disp_write_mode;
  disp_circle( xc, yc, r, col, mode );
  return 0;
}

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE dp105_map[] =
{
  { LSTRKEY( "init" ), LFUNCVAL( dp105_init ) },
  { LSTRKEY( "draw" ), LFUNCVAL( dp105_draw ) },
  { LSTRKEY( "putpixel" ), LFUNCVAL( dp105_putpixel ) },
  { LSTRKEY( "getpixel" ), LFUNCVAL( dp105_getpixel ) },
  { LSTRKEY( "set_write_mode" ), LFUNCVAL( dp105_set_write_mode ) },
  { LSTRKEY( "set_viewport" ), LFUNCVAL( dp105_set_viewport ) },
  { LSTRKEY( "clrscr" ), LFUNCVAL( dp105_clrscr ) },
  { LSTRKEY( "putstr" ), LFUNCVAL( dp105_putstr ) },
  { LSTRKEY( "set_row_intensity" ), LFUNCVAL( dp105_set_row_intensity ) },
  { LSTRKEY( "line" ), LFUNCVAL( dp105_line ) },
  { LSTRKEY( "rectangle" ), LFUNCVAL( dp105_rectangle ) },
  { LSTRKEY( "filled_rectangle" ), LFUNCVAL( dp105_filled_rectangle ) },
  { LSTRKEY( "get_text_extent" ), LFUNCVAL( dp105_get_text_extent ) },
  { LSTRKEY( "set_font" ), LFUNCVAL( dp105_set_font ) },
  { LSTRKEY( "load_font" ), LFUNCVAL( dp105_load_font ) },
  { LSTRKEY( "load_bitmap" ), LFUNCVAL( dp105_load_bitmap ) },
  { LSTRKEY( "draw_bitmap" ), LFUNCVAL( dp105_draw_bitmap ) },
  { LSTRKEY( "get_bitmap_size" ), LFUNCVAL( dp105_get_bitmap_size ) },
  { LSTRKEY( "circle" ), LFUNCVAL( dp105_circle ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "OVERWRITE" ), LNUMVAL( DISP_WRITE_OVERWRITE ) },
  { LSTRKEY( "AND" ), LNUMVAL( DISP_WRITE_AND ) },
  { LSTRKEY( "OR" ), LNUMVAL( DISP_WRITE_OR ) },
  { LSTRKEY( "XOR" ), LNUMVAL( DISP_WRITE_XOR ) },
  { LSTRKEY( "NORMAL" ), LNUMVAL( DISP_VP_NORMAL ) },
  { LSTRKEY( "INVERTED" ), LNUMVAL( DISP_VP_INVERTED ) }, 
  { LSTRKEY( "FONT_5x7" ), LNUMVAL( DISP_FONT_5x7 ) },
  { LSTRKEY( "FONT_5x5_ROUND" ), LNUMVAL( DISP_FONT_5x5_ROUND ) },
  { LSTRKEY( "FONT_5x5_SQUARE" ), LNUMVAL( DISP_FONT_5x5_SQUARE ) },  
  { LSTRKEY( "FONT_3x5" ), LNUMVAL( DISP_FONT_3x5 ) },
  { LSTRKEY( "FONT_4x6" ), LNUMVAL( DISP_FONT_4x6 ) },
#endif
  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE font_mt_map[] = 
{
  { LSTRKEY( "__gc" ), LFUNCVAL( font_gc ) },
  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE bitmap_mt_map[] = 
{
  { LSTRKEY( "__gc" ), LFUNCVAL( bitmap_gc ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_dp105( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  // Create default font metatable
  luaL_rometatable( L, FONT_META_NAME, ( void* )font_mt_map );
  // Create default bitmap metatable
  luaL_rometatable( L, BITMAP_META_NAME, ( void* )bitmap_mt_map );
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  #error "dp105 module doesn't run in LUA_OPTIMIZE_MEMORY != 0 "
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}

#endif // #ifdef ELUA_BOARD_WIZ

