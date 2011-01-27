// Persistent settings implementation

#include "settings.h"
#include "datamem.h"
#include <string.h>

// ****************************************************************************
// Local variables

static u8 settings_buf[ SETTINGS_MAX_SIZE ];
static SETTINGS settings_data;

// ****************************************************************************
// Local functions

// Build a default set of settings
static void settings_make_default()
{
  u8 defmac[ 6 ] = SETTINGS_DEFAULT_MAC;
  u8 defip[ 4 ] = SETTINGS_DEFAULT_IP;
  u8 defmask[ 4 ] = SETTINGS_DEFAULT_MASK;
  u8 defgw[ 4 ] = SETTINGS_DEFAULT_GATEWAY;
  u8 defdns[ 4 ] = SETTINGS_DEFAULT_DNS;

  settings_data.sign = SETTINGS_SIGN;
  settings_data.static_mode = 1;
  memcpy( settings_data.mac, defmac, 6 );
  memcpy( settings_data.ip, defip, 4 );
  memcpy( settings_data.mask, defmask, 4 );
  memcpy( settings_data.gw, defgw, 4 );
  memcpy( settings_data.dns, defdns, 4 );
  strcpy( settings_data.username, SETTINGS_DEFAULT_USERNAME );
  strcpy( settings_data.password, SETTINGS_DEFAULT_PASSWORD );
  strcpy( settings_data.name, SETTINGS_DEFAULT_NAME );
}

// ****************************************************************************
// Public interface

void settings_init()
{
	ISP_data_sector_read( ( uint16 )settings_buf );
  memcpy( &settings_data, settings_buf, sizeof( settings_data ) );
  if( settings_data.sign != SETTINGS_SIGN )
    settings_make_default();
}

SETTINGS* settings_get()
{
  return &settings_data;
}

void settings_set( SETTINGS* psettings )
{
  settings_data = *psettings;  
}

void settings_write()
{
  memset( settings_buf, 0xFF, SETTINGS_MAX_SIZE );
  memcpy( settings_buf, &settings_data, sizeof( settings_data ) );
	ISP_data_erase();
  ISP_data_sector_prog( ( uint16 )settings_buf );
}
