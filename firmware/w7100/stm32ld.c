// STM32 bootloader client
 
#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "type.h"
#include "stm32ld.h"
 
static ser_handler stm32_ser_id = ( ser_handler )-1;
 
#define STM32_RETRY_COUNT 10
#define STM32_SER_TIMEOUT 2000
 
// ****************************************************************************
// Helper functions and macros
 
// Check initialization
#define STM32_CHECK_INIT
 
// Check received byte
#define STM32_EXPECT( expected )\
  if( stm32h_read_byte() != expected )\
    return STM32_COMM_ERROR;
 
#define STM32_READ_AND_CHECK( x )\
  if( ( x = stm32h_read_byte() ) == -1 )\
    return STM32_COMM_ERROR;
 
// Helper: send a command to the STM32 chip
static void stm32h_send_command( u8 cmd )
{
  ser_write_byte( cmd );
  ser_write_byte( ~cmd );
}
 
// Helper: read a byte from STM32 with timeout
static int stm32h_read_byte()
{
  return ser_read_byte( STM32_SER_TIMEOUT );
}
 
// Helper: append a checksum to a packet and send it
static int stm32h_send_packet_with_checksum( u8 *packet, u16 len )
{
  u8 chksum = 0;
  u16 i;
 
  for( i = 0; i < len; i ++ )
    chksum ^= packet[ i ];
  ser_write( packet, len );
  ser_write_byte( chksum );
  return STM32_OK;
}
 
// Helper: send an address to STM32
static int stm32h_send_address( u32 address )
{
  u8 addr_buf[ 4 ];
 
  addr_buf[ 0 ] = address >> 24;
  addr_buf[ 1 ] = ( address >> 16 ) & 0xFF;
  addr_buf[ 2 ] = ( address >> 8 ) & 0xFF;
  addr_buf[ 3 ] = address & 0xFF;
  return stm32h_send_packet_with_checksum( addr_buf, 4 );
}
 
// Helper: intiate BL communication
static int stm32h_connect_to_bl()
{
  int res;
 
  // Flush all incoming data
  while( ser_read_byte( SER_NO_TIMEOUT ) != -1 );
 
  // Initiate communication
  ser_write_byte( STM32_CMD_INIT );
  res = stm32h_read_byte();
  return res == STM32_COMM_ACK || res == STM32_COMM_NACK ? STM32_OK : STM32_INIT_ERROR;
}
 
// ****************************************************************************
// Implementation of the protocol
 
int stm32_init()
{ 
  // Setup port
  ser_set_mode( SER_MODE_FWUPD );
 
  // Connect to bootloader
  return stm32h_connect_to_bl();
}
 
// Get bootloader version
// Expected response: ACK version OPTION1 OPTION2 ACK
int stm32_get_version( u8 *major, u8 *minor )
{
  u8 temp, total, i, version;
  int tries = STM32_RETRY_COUNT;
 
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_COMMAND );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_READ_AND_CHECK( total );
  for( i = 0; i < total + 1; i ++ )
  {
    STM32_READ_AND_CHECK( temp );
    if( i == 0 )
      version = temp;
  }
  *major = version >> 4;
  *minor = version & 0x0F;
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}
 
// Get chip ID
int stm32_get_chip_id( u16 *version )
{
  u8 vh, vl;
 
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_ID );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( 1 );
  STM32_READ_AND_CHECK( vh );
  STM32_READ_AND_CHECK( vl );
  STM32_EXPECT( STM32_COMM_ACK );
  *version = ( ( u16 )vh << 8 ) | vl;
  return STM32_OK;
}
 
// Write unprotect
int stm32_write_unprotect()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_WRITE_UNPROTECT );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( STM32_COMM_ACK );
  // At this point the system got a reset, so we need to re-enter BL mode
  return stm32h_connect_to_bl();
}
 
// Erase flash
int stm32_erase_flash()
{ 
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_ERASE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK );
  ser_write_byte( 0xFF );
  ser_write_byte( 0x00 );
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}
 
// Program flash
// Requires pointers to two functions: get data and progress report
int stm32_write_flash( const u8* progdata, u16 datalen, u32 addr )
{
  u8 fdata[ STM32_WRITE_BUFSIZE + 1 ];
 
  STM32_CHECK_INIT;
  fdata[ 0 ] = ( u8 )( datalen - 1 );
  memcpy( fdata + 1, progdata, datalen );

  // Send write request
  stm32h_send_command( STM32_CMD_WRITE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK );
  
  // Send address
  stm32h_send_address( addr );
  STM32_EXPECT( STM32_COMM_ACK );

  // Send data
  stm32h_send_packet_with_checksum( fdata, datalen + 1 );
  STM32_EXPECT( STM32_COMM_ACK );

  // All done
  return STM32_OK;
}
