// Persistent settings implementation

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "type.h"

// Settings data
#define SETTINGS_SIGN                   0xAFD5
#define SETTINGS_MAX_SIZE               256
#define SETTINGS_MAX_USERNAME_LEN       64
#define SETTINGS_MAX_PASSWORD_LEN       64
#define SETTINGS_MAX_NAME_LEN           32

// Edit below to change the default settings
#define SETTINGS_DEFAULT_MAC            { 0x00, 0x08, 0xdc, 0x00, 0x00,  0x00 }
#define SETTINGS_DEFAULT_IP             { 192, 168, 1, 5 }
#define SETTINGS_DEFAULT_MASK           { 255, 255, 255, 0 }
#define SETTINGS_DEFAULT_GATEWAY        { 192, 168, 1, 1 }
#define SETTINGS_DEFAULT_DNS            { 192, 168, 1, 1 }
#define SETTINGS_DEFAULT_USERNAME       "admin"
#define SETTINGS_DEFAULT_PASSWORD       "admin"
#define SETTINGS_DEFAULT_NAME           "moonlight"

// Settings structure
typedef struct 
{
  u16 sign;
  u8 static_mode;
  u8 mac[ 6 ];
  u8 ip[ 4 ], mask[ 4 ], gw[ 4 ], dns[ 4 ];
  char username[ SETTINGS_MAX_USERNAME_LEN + 1 ];
  char password[ SETTINGS_MAX_PASSWORD_LEN + 1 ];
  char name[ SETTINGS_MAX_NAME_LEN + 1 ];
} SETTINGS;

void settings_init();
void settings_write();
SETTINGS* settings_get();
void settings_set( SETTINGS* psettings );

#endif
