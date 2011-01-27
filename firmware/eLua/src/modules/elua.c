// Interface with core services

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include "legc.h"
#include "platform_conf.h"
#include "devman.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// Lua: elua.egc_setup( mode, [ memlimit ] )
static int elua_egc_setup( lua_State *L )
{
  int mode = luaL_checkinteger( L, 1 );
  unsigned memlimit = 0;

  if( lua_gettop( L ) >= 2 )
    memlimit = ( unsigned )luaL_checkinteger( L, 2 );
  legc_set_mode( L, mode, memlimit );
  return 0;
}

// Lua: elua.con_uart_id()
static int elua_con_uart_id( lua_State *L )
{
#ifdef CON_UART_ID
  lua_pushinteger( L, CON_UART_ID );
  return 1;
#else
  return luaL_error( L, "CON_UART_ID is not defined" );
#endif
}

// Lua: ftable = elua.ls( fsroot, [extfilter] )
static int elua_ls( lua_State *L )
{
  const char *proot = luaL_checkstring( L, 1 );
  const char *pextf = NULL;
  DM_DIR *d;
  struct dm_dirent *ent;
  int totfiles = 0;
  
  if( lua_isstring( L, 2 ) )
    pextf = luaL_checkstring( L, 2 );
  d = dm_opendir( proot );
  if( d )
  {
    while( ( ent = dm_readdir( d ) ) != NULL )
    {
      if( pextf && strcasestr( ent->fname, pextf ) == NULL )
        continue;
      if( totfiles == 0 )
        lua_newtable( L );
      lua_pushstring( L, ent->fname );
      lua_rawseti( L, -2, totfiles + 1 );
      totfiles ++;
    }  
    dm_closedir( d );
    if( totfiles == 0 )
      lua_pushnil( L );
  } 
  else
    lua_pushnil( L );
  return 1;
}

// Lua: s = strftime( fmt, time_t )
#define MAX_TIME_BUF_SIZE     80
static int elua_strftime( lua_State *L )
{
  const char *fmt = luaL_checkstring( L, 1 );
  time_t t = ( time_t )luaL_checkinteger( L, 2 );
  struct tm *tm = NULL;
  char tbuf[ MAX_TIME_BUF_SIZE + 1 ];
  luaL_Buffer b;
    
  tm = localtime( &t );
  strftime( tbuf, MAX_TIME_BUF_SIZE, fmt, tm );
  luaL_buffinit( L, &b );
  luaL_addstring( &b, tbuf );
  luaL_pushresult( &b );
  return 1;    
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE elua_map[] = 
{
  { LSTRKEY( "egc_setup" ), LFUNCVAL( elua_egc_setup ) },
  { LSTRKEY( "con_uart_id" ), LFUNCVAL ( elua_con_uart_id ) },
  { LSTRKEY( "ls" ), LFUNCVAL( elua_ls ) },
  { LSTRKEY( "strftime" ), LFUNCVAL( elua_strftime ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "EGC_NOT_ACTIVE" ), LNUMVAL( EGC_NOT_ACTIVE ) },
  { LSTRKEY( "EGC_ON_ALLOC_FAILURE" ), LNUMVAL( EGC_ON_ALLOC_FAILURE ) },
  { LSTRKEY( "EGC_ON_MEM_LIMIT" ), LNUMVAL( EGC_ON_MEM_LIMIT ) },
  { LSTRKEY( "EGC_ALWAYS" ), LNUMVAL( EGC_ALWAYS ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_elua( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else
  luaL_register( L, AUXLIB_ELUA, elua_map );
  MOD_REG_NUMBER( L, "EGC_NOT_ACTIVE", EGC_NOT_ACTIVE );
  MOD_REG_NUMBER( L, "EGC_ON_ALLOC_FAILURE", EGC_ON_ALLOC_FAILURE );
  MOD_REG_NUMBER( L, "EGC_ON_MEM_LIMIT", EGC_ON_MEM_LIMIT );
  MOD_REG_NUMBER( L, "EGC_ALWAYS", EGC_ALWAYS );
  return 1;
#endif
}
