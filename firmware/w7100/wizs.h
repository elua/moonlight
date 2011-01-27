// WizNET RPC server

#ifndef __WIZS_H__
#define __WIZS_H__

#include "type.h"
#include "wiznet.h"

// Application socket range
#define WIZ_FIRST_APP_SOCKET    3
#define WIZ_LAST_APP_SOCKET     6
#define WIZ_TOTAL_APP_SOCKET    ( WIZ_LAST_APP_SOCKET - WIZ_FIRST_APP_SOCKET + 1 )

// Application web page limits
#define WIZ_APP_MAX_NAME_LEN    32
#define WIZ_APP_MAX_PAGE        8192
#define WIZ_APP_MAX_NUM_ARGS    32
#define WIZ_APP_MAX_ARG_LEN     256
#define WIZ_APP_MAX_CGI_LEN     1024

// Nonblocking data
typedef struct
{
  u8 type;
  u16 count;
} wizs_nb_data_t;

void wizs_init();
void wizs_execute_request();
void wizs_check_nb_response();
int wizs_app_exists();
const char* wizs_app_get_name();
const char* wizs_app_get_template();
const char* wizs_app_get_arg( unsigned i );
void wizs_app_set_cgi_data( const char* cgidata );

#endif
