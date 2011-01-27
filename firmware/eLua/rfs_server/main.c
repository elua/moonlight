// Remote FS server

#include "remotefs.h"
#include "eluarpc.h"
#include "serial.h"
#include "server.h"
#include "type.h"
#include "log.h"
#include "os_io.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "pnet.h"

// ****************************************************************************
// Local variables

#define   MAX_PACKET_SIZE     4096

static u8 rfs_buffer[ MAX_PACKET_SIZE + ELUARPC_WRITE_REQUEST_EXTRA ]; 

// ****************************************************************************
// Helpers

#if defined( RFS_SERIAL_TRANSPORT )

static ser_handler ser;

static void flush_serial()
{
  // Flush all data in serial port
  while( ser_read_byte( ser, SER_NO_TIMEOUT ) != -1 );
}

// Read a packet from the serial port
static void read_request_packet()
{
  u16 temp16;
  u32 readbytes;

  while( 1 )
  {
    // First read the length
    if( ( readbytes = ser_read( ser, rfs_buffer, ELUARPC_START_OFFSET, SER_INF_TIMEOUT ) ) != ELUARPC_START_OFFSET )
    {
      log_msg( "read_request_packet: ERROR reading packet length. Requested %d bytes, got %d bytes\n", ELUARPC_START_OFFSET, readbytes );
      flush_serial();
      continue;
    }

    if( eluarpc_get_packet_size( rfs_buffer, &temp16 ) == ELUARPC_ERR )
    {
      log_msg( "read_request_packet: ERROR getting packet size.\n" );
      flush_serial();
      continue;
    }

    // Then the rest of the data
    if( ( readbytes = ser_read( ser, rfs_buffer + ELUARPC_START_OFFSET, temp16 - ELUARPC_START_OFFSET, SER_INF_TIMEOUT ) ) != temp16 - ELUARPC_START_OFFSET )
    {
      log_msg( "read_request_packet: ERROR reading full packet, got %u bytes, expected %u bytes\n", ( unsigned )readbytes, ( unsigned )temp16 - ELUARPC_START_OFFSET );
      flush_serial();
      continue;
    }
    else
      break;
  }
}

// Send a packet to the serial port
static void send_response_packet()
{
  u16 temp16;

  // Send request
  if( eluarpc_get_packet_size( rfs_buffer, &temp16 ) != ELUARPC_ERR )
  {
    log_msg( "send_response_packet: sending response packet of %u bytes\n", ( unsigned )temp16 );
    ser_write( ser, rfs_buffer, temp16 );
  }
}

// Secure atoi
static int secure_atoi( const char *str, long *pres )
{
  char *end_ptr;
  long s1;
  
  errno = 0;
  s1 = strtol( str, &end_ptr, 10 );
  if( ( s1 == LONG_MIN || s1 == LONG_MAX ) && errno != 0 )
    return 0;
  else if( end_ptr == str )
    return 0;
  else if( s1 > INT_MAX || s1 < INT_MIN )
    return 0;
  else if( '\0' != *end_ptr )
    return 0;
  *pres = s1;
  return 1;  
}

#elif defined( RFS_UDP_TRANSPORT ) // #if defined( USE_SERIAL_TRANSPORT )

#include <pthread.h>

static SOCKET trans_socket = INVALID_SOCKET_VALUE;
static struct sockaddr_in trans_from;
volatile int rfs_thread_should_die = 0;

// Helper: read (blocking) the specified number of bytes

static void read_helper( u8 *dest, u32 size )
{
  socklen_t fromlen;
  int readbytes;
#ifdef MUX_THREAD_MODE
  fd_set fds;
  struct timeval tv;
#endif

  while( size )
  {
#ifdef MUX_THREAD_MODE
    FD_ZERO( &fds );
    FD_SET( trans_socket, &fds );
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if( select( trans_socket + 1, &fds, NULL, NULL, &tv ) <= 0 )
    {
      if( rfs_thread_should_die )
        pthread_exit( NULL );
      else
        continue;
    }
#endif
    fromlen = sizeof( trans_from );
    readbytes = recvfrom( trans_socket, dest, size, 0, ( struct sockaddr* )&trans_from, &fromlen );
    size -= readbytes;
    if( size == 0 )
      break;
    dest += readbytes;
  }
}

static void read_request_packet()
{
  u16 temp16;
 
  while( 1 )
  {
    // First read the length
    read_helper( rfs_buffer, ELUARPC_START_OFFSET );

    if( eluarpc_get_packet_size( rfs_buffer, &temp16 ) == ELUARPC_ERR )
    {
      log_msg( "read_request_packet: ERROR getting packet size.\n" );
      continue;
    }

    // Then the rest of the data
    read_helper( rfs_buffer + ELUARPC_START_OFFSET, temp16 - ELUARPC_START_OFFSET );
    break;
  }
}

static void send_response_packet()
{
  u16 temp16;
  
  // Send request
  if( eluarpc_get_packet_size( rfs_buffer, &temp16 ) != ELUARPC_ERR )
  {
    log_msg( "send_response_packet: sending response packet of %u bytes\n", ( unsigned )temp16 );
    sendto( trans_socket, rfs_buffer, temp16, 0, ( struct sockaddr* )&trans_from, sizeof( trans_from ) );
  }  
}

int rfs_server_init( unsigned server_port, const char* dirname )
{
  int length;
  struct sockaddr_in server;
     
  if( ( trans_socket = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
  {
    log_err( "Unable to create socket\n" );
    return 1;
  }
  length = sizeof( server );
  memset( &server, 0, sizeof( server ) );
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( server_port );
  if( bind( trans_socket, ( struct sockaddr * )&server, length ) < 0 )
  {
   log_err( "Unable to bind socket\n" );
   return 0; 
  }
  // Setup RFS server
  server_setup( dirname );   
  return 1;    
}

void rfs_server_cleanup()
{
  server_cleanup();
  if( trans_socket != INVALID_SOCKET_VALUE )
  {
    socket_close( trans_socket );
    trans_socket = INVALID_SOCKET_VALUE;
  }    
  rfs_thread_should_die = 0;
}

#else                               // #if defined( USE_SERIAL_TRANSPORT )
#error "No transport defined"
#endif                              // #if defined( USE_SERIAL_TRANSPORT )

// ****************************************************************************
// Entry point

#if defined( RFS_THREAD_MODE )

void* rfs_thread( void* data )
{  
  log_msg( "Starting RFS service thread on directory %s\n", ( char* )data );      
  // Enter the server endless loop
  while( 1 )
  {
    read_request_packet();
    server_execute_request( rfs_buffer );
    send_response_packet();
  }  
  return NULL; 
}

#elif defined( RFS_STANDALONE_MODE ) // if defined( RFS_THREAD_MODE )

#define PORT_ARG_IDX          1
#define SPEED_ARG_IDX         2
#define DIRNAME_ARG_IDX       3
#define VERBOSE_ARG_IDX       4

int main( int argc, const char **argv )
{
  long serspeed;
  
  if( argc < 4 )
  {
    log_err( "Usage: %s <port> <speed> <dirname> [-v]\n", argv[ 0 ] );
    log_err( "(use -v for verbose output).\n");
    return 1;
  }
  if( secure_atoi( argv[ SPEED_ARG_IDX ], &serspeed ) == 0 )
  {
    log_err( "Invalid speed\n" );
    return 1;
  } 
  if( !os_isdir( argv[ DIRNAME_ARG_IDX ] ) )
  {
    log_err( "Invalid directory %s\n", argv[ DIRNAME_ARG_IDX ] );
    return 1;
  }
  if( ( argc >= 5 ) && !strcmp( argv[ VERBOSE_ARG_IDX ], "-v" ) )
    log_init( LOG_ALL );
  else
    log_init( LOG_NONE );

  // Setup RFS server
  server_setup( argv[ DIRNAME_ARG_IDX ] );

  // Setup serial port
  if( ( ser = ser_open( argv[ PORT_ARG_IDX ] ) ) == ( ser_handler )-1 )
  {
    log_err( "Cannot open port %s\n", argv[ PORT_ARG_IDX ] );
    return 1;
  }
  if( ser_setup( ser, ( u32 )serspeed, SER_DATABITS_8, SER_PARITY_NONE, SER_STOPBITS_1 ) != SER_OK )
  {
    log_err( "Unable to initialize serial port\n" );
    return 1;
  }
  flush_serial();
  
  // User report
  log_msg( "Running RFS server on port %s (%u baud) in directory %s\n", argv[ PORT_ARG_IDX ], ( unsigned )serspeed, argv[ DIRNAME_ARG_IDX ] );  

  // Enter the server endless loop
  while( 1 )
  {
    read_request_packet();
    server_execute_request( rfs_buffer );
    send_response_packet();
  }

  ser_close( ser );
  return 0;
}

#else // if defined( RFS_THREAD_MODE )
#error "Undefined run mode (standalone or thread)"
#endif

