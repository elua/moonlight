// Validate eLua configuration options
// Should be included in main.c, as this is the first file that's compiled, so
// any configuration errors are caught right at the beginning of the build
// process

#ifndef __VALIDATE_H__
#define __VALIDATE_H__

#include "platform_conf.h"

// The maximum 32 GPIO ports limitation is given by the port_mask variable in src/modules/pio.c
#if NUM_PIO > 32
#error "Can't have more than 32 GPIO ports"
#endif

// Can't define more than one console devices
#if defined( BUILD_CON_TCP ) && defined( BUILD_CON_GENERIC )
#error "Can't have two console devices (don't enable BUILD_CON_TCP and BUILD_CON_GENERIC in platform_conf.h at the same time)"
#endif // #if defined( BUILD_CON_TCP ) && defined( BUILD_CON_GENERIC )

// For TCP console we need to enable TCP support
#ifdef BUILD_CON_TCP
  #ifndef BUILD_UIP
  #error "BUILD_CON_TCP requires BUILD_UIP to be defined in platform_conf.h" 
  #endif // #ifndef BUILD_UIP
#endif // #ifdef BUILD_CON_TCP

// If TCP console is enabled, we don't need xmodem or term
// (they can still function separately over UART, but this doesn't make sense)
#ifdef BUILD_CON_TCP
  #ifdef BUILD_XMODEM
  #error "XMODEM doesn't work with TCP console. Disable BUILD_XMODEM in platform_conf.h"
  #endif // #ifdef BUILD_XMODME
  #ifdef BUILD_TERM
  #error "ANSI terminal support doesn't work (yet) with TCP console. Disable BUILD_TERM in platform_conf.h"
  #endif // #ifdef BUILD_TERM
#endif // #ifdef BUILD_CON_TCP

// For DHCP we need to have TCP/IP support
#ifdef BUILD_DHCPC
  #ifndef BUILD_UIP
  #error "DHCP client requires TCP/IP support (enable BUILD_UIP in platform_conf.h)"
  #endif // #ifndef BUILD_UIP
#endif // #ifdef BUILD_DHCPC

// For DNS we need to have TCP/IP support
#ifdef BUILD_DNS
  #ifndef BUILD_UIP
  #error "DNS resolver requires TCP/IP support (enable BUILD_UIP in platform_conf.h)"
  #endif // #ifndef BUILD_UIP
#endif // #ifdef BUILD_DNS

#endif // #ifndef __VALIDATE_H__
