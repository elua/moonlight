// TFTP server implementation

#include "tftp.h"
#include "type.h"
#include "serial.h"
#include "socket.h"
#include "vtmr.h"
#include "w7100.h"
#include "stm32ld.h"
#include "lcd.h"
#include "wiz.h"
#include "stm32conf.h"
#include <string.h>
#include <stdio.h>

static char gstr[ 41 ];

// ****************************************************************************
// Protocol data

// TFTP buffer
static u8 xdata tftp_buffer[ TFTP_MAX_PACKET_SIZE ];

// Bootloader data
#define BL_VERSION_MAJOR      2
#define BL_VERSION_MINOR      1
#define BL_CHIP_ID            0x0414

// ****************************************************************************
// Helpers

// Add a u16 to the packet
static u8 xdata* tftp_add_u16( u8 xdata *p, u16 adata )
{
  *p ++ = adata >> 8;
  *p ++ = adata & 0xFF;
  return p;
}

// Add a string to the packet
static u8 xdata* tftp_add_string( u8 xdata *p, const char* adata )
{
  strcpy( p, adata );
  return p + strlen( adata ) + 1;
}

// Build an error packet
static u16 tftp_build_error_packet( u16 pcode, const char* msg )
{
  u8 xdata *p = tftp_buffer;

  p = tftp_add_u16( p, TFTP_PACKET_ERROR );
  p = tftp_add_u16( p, pcode );
  p = tftp_add_string( p, msg );
  return p - tftp_buffer;
}

// Build an ACK packet
static u16 tftp_build_ack_packet( u16 seqno )
{
  u8 xdata *p = tftp_buffer;

  p = tftp_add_u16( p, TFTP_PACKET_ACK );
  p = tftp_add_u16( p, seqno );
  return p - tftp_buffer;
}

// Check bootloader 
static u8 tftp_fwupd_init()
{
  u8 major, minor;
  u16 chip_id;

  // Init the bootloader
  if( stm32_init() != STM32_OK )          
    return 0;

  // Get and check bootloader version
  if( stm32_get_version( &major, &minor ) != STM32_OK )
    return 0;                                          
  if( major != BL_VERSION_MAJOR || minor != BL_VERSION_MINOR )
    return 0;

  // Get and check chip ID
  if( stm32_get_chip_id( &chip_id ) != STM32_OK )
    return 0;
  if( chip_id != BL_CHIP_ID )
    return 0;

  // Erase flash
  if( stm32_erase_flash() != STM32_OK )
    return 0;

  // All OK
  return 1;
}

// Write data packet
static u8 tftp_write_packet( u16 len, u32 *paddr )
{
  u8 xdata *p = tftp_buffer + TFTP_DATA_HEADER_SIZE;
  u16 towrite;

  // Packet size is maximum 512 bytes, so write twice if needed
  while( len )
  {
    towrite = len > STM32_WRITE_BUFSIZE ? STM32_WRITE_BUFSIZE : len;
    if( stm32_write_flash( p, towrite, *paddr ) != STM32_OK )
      return 0;
    len -= towrite;
    *paddr += towrite;    
    p += towrite;
  }
  return 1;
}

// ****************************************************************************
// Main protocol implementation

// Run the main loop of the TFTP server
void tftp_server_run()
{
  u16 port, len, pcode;
  u8 host[ 4 ]; 
  u32 addr = 0xFFFFFFFFUL;
  u8 finished = 0;
  u16 seq;

  while( 1 )
  {
    // Wait for data with timeout
    vtmr_set_timeout( VTMR_NET, TFTP_TIMEOUT );
    vtmr_enable( VTMR_NET );
    while( !vtmr_is_expired( VTMR_NET ) )
      if( wiz_getSn_RX_RSR( WIZ_SOCK_TFTP ) > 0 )
        break;
    vtmr_disable( VTMR_NET );
    if(  wiz_getSn_RX_RSR( WIZ_SOCK_TFTP ) == 0 )
      break;

    len = recvfrom( WIZ_SOCK_TFTP, tftp_buffer, TFTP_MAX_PACKET_SIZE, host, &port );

    // Get packet code
    pcode = ( ( uint16 )tftp_buffer[ 0 ] << 8 ) + tftp_buffer[ 1 ];

    // Interpret packet
    switch( pcode )
    {
      case TFTP_PACKET_RRQ:
        // We don't support reads, return error packet
        len = tftp_build_error_packet( TFTP_ERR_ILLEGAL_OP, "not implemented" );
        sendto( WIZ_SOCK_TFTP, tftp_buffer, len, host, port, VTMR_INF_TIMEOUT );
        break;  

      case TFTP_PACKET_WRQ:
        // Write request: put target in firmware update mode, then try to communicate
        // with the bootloader. If it works, return ACK, otherwise return error
        STM32_BOOT0_PIN = 1;
        vtmr_delay( 16 );
        STM32_RESET_PIN = 0;
        vtmr_delay( 64 );
        STM32_RESET_PIN = 1;
        vtmr_delay( WIZ_FWUPD_WAIT );
        // Init the bootloader
        if( tftp_fwupd_init() )          
        {
          len = tftp_build_ack_packet( 0 );
          addr = STM32_FLASH_START_ADDRESS;
        }
        else
         len = tftp_build_error_packet( TFTP_ERR_ILLEGAL_OP, "bootloader error" );
        sendto( WIZ_SOCK_TFTP, tftp_buffer, len, host, port, VTMR_INF_TIMEOUT );
        break;

      case TFTP_PACKET_ERROR:
        // Simply ignore this
        break;

      case TFTP_PACKET_DATA:
        // Read and program data
        // Get sequence number
        seq = ( ( uint16 )tftp_buffer[ 2 ] << 8 ) + tftp_buffer[ 3 ];
        len -= TFTP_DATA_HEADER_SIZE;
        if( len < TFTP_BLOCK_SIZE ) // this was the last data packet
          finished = 1;
        // Write packet
        if( tftp_write_packet( len, &addr ) )
          len = tftp_build_ack_packet( seq );
        else
        {
          len = tftp_build_error_packet( TFTP_ERR_ILLEGAL_OP, "write error" );
          finished = 0;
        }
        sendto( WIZ_SOCK_TFTP, tftp_buffer, len, host, port, VTMR_INF_TIMEOUT );
        break;
    }
  }    
  if( finished )
  {
    // Automatically reset the CPU here
    EA = 0;			
  	TA = 0xAA;
  	TA = 0x55;
  	WDCON = 0x02;  	
    while( 1 );
  }
  ser_set_mode( SER_MODE_RPC ); 
}
