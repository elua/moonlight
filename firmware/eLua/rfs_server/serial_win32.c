// Serial inteface implementation for POSIX-compliant systems

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "serial.h"

#define WIN_ERROR     ( HANDLE )-1
#define WIN_MAX_PORT_NAME   1024

// Helper: set timeout
static int ser_win32_set_timeouts( HANDLE hComm, DWORD ri, DWORD rtm, DWORD rtc, DWORD wtm, DWORD wtc )
{   
  COMMTIMEOUTS timeouts;
  
  if( GetCommTimeouts( hComm, &timeouts ) == FALSE )
  {
    CloseHandle( hComm );
    return SER_ERR;
  }
  timeouts.ReadIntervalTimeout = ri;
  timeouts.ReadTotalTimeoutConstant = rtm;
  timeouts.ReadTotalTimeoutMultiplier = rtc;
  timeouts.WriteTotalTimeoutConstant = wtm;
  timeouts.WriteTotalTimeoutMultiplier = wtc;
	if( SetCommTimeouts( hComm, &timeouts ) == FALSE )
	{
	  CloseHandle( hComm );
	  return SER_ERR;
  }               
  
  return SER_OK;
}

// Helper: set communication timeout
static int ser_set_timeout_ms( HANDLE hComm, u32 timeout )
{ 
  if( timeout == SER_NO_TIMEOUT )
    return ser_win32_set_timeouts( hComm, MAXDWORD, 0, 0, 0, 0 );
  else if( timeout == SER_INF_TIMEOUT )
    return ser_win32_set_timeouts( hComm, 0, 0, 0, 0, 0 );
  else
    return ser_win32_set_timeouts( hComm, 0, 0, timeout, 0, 0 );
}

// Open the serial port
ser_handler ser_open( const char* sername )
{
  char portname[ WIN_MAX_PORT_NAME + 1 ];
  HANDLE hComm;
  
  portname[ 0 ] = portname[ WIN_MAX_PORT_NAME ] = '\0';
  _snprintf( portname, WIN_MAX_PORT_NAME, "\\\\.\\%s", sername );
  hComm = CreateFile( portname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0 );
  if( hComm == INVALID_HANDLE_VALUE )
    return WIN_ERROR;
  if( !SetupComm( hComm, 2048, 2048 ) )
    return WIN_ERROR;
  if( ser_set_timeout_ms( hComm, SER_INF_TIMEOUT ) != SER_OK )
    return WIN_ERROR;
  return ( ser_handler )hComm;
}

// Close the serial port
void ser_close( ser_handler id )
{
  CloseHandle( ( HANDLE )id );
}

int ser_setup( ser_handler id, u32 baud, int databits, int parity, int stopbits )
{
  HANDLE hComm = ( HANDLE )id;
  DCB dcb;
  
	if( GetCommState( hComm, &dcb ) == FALSE )
	{
		CloseHandle( hComm );
		return SER_ERR;
	}
  dcb.BaudRate = baud;
  dcb.ByteSize = databits;
  dcb.Parity = parity == SER_PARITY_NONE ? NOPARITY : ( parity == SER_PARITY_EVEN ? EVENPARITY : ODDPARITY );
  dcb.StopBits = stopbits == SER_STOPBITS_1 ? ONESTOPBIT : ( stopbits == SER_STOPBITS_1_5 ? ONE5STOPBITS : TWOSTOPBITS );
  dcb.fBinary = TRUE;
  dcb.fDsrSensitivity = FALSE;
  dcb.fParity = parity != SER_PARITY_NONE ? TRUE : FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fNull = FALSE;
  /**/ dcb.fAbortOnError = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fDsrSensitivity = FALSE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fOutxCtsFlow = FALSE;
  if( SetCommState( hComm, &dcb ) == 0 )
  {
    CloseHandle( hComm );
    return SER_ERR;
  }
  
  if( ser_win32_set_timeouts( hComm, 0, 0, 0, 0, 0 ) == SER_ERR )
  {
    CloseHandle( hComm );
    return SER_ERR;
  }
  
  FlushFileBuffers( hComm );

  return SER_OK;
}

// Read up to the specified number of bytes, return bytes actually read
u32 ser_read( ser_handler id, u8* dest, u32 maxsize, u32 timeout )
{
  HANDLE hComm = ( HANDLE )id;
  OVERLAPPED o = { 0 };
  DWORD readbytes = 0;
  BOOL fWaitingOnRead = FALSE;
  
  o.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( ReadFile( hComm, dest, maxsize, &readbytes, &o ) == FALSE )
  {
    if( GetLastError() != ERROR_IO_PENDING )
    {
      CloseHandle( o.hEvent );      
      return 0;
    }
    else
      fWaitingOnRead = TRUE;
  }
  else
  {
    CloseHandle( o.hEvent );
    return readbytes;
  }
    
  if( fWaitingOnRead )
  {
    BOOL dwRes = WaitForSingleObject( o.hEvent, timeout == SER_INF_TIMEOUT ? INFINITE : timeout );
    if( dwRes == WAIT_OBJECT_0 ) 
    {
      if( !GetOverlappedResult( hComm, &o, &readbytes, TRUE ) )
        readbytes = 0;
    }
    else if( dwRes == WAIT_TIMEOUT )
    {
      CancelIo( hComm );
      GetOverlappedResult( hComm, &o, &readbytes, TRUE );
      readbytes = 0;
    }
  }  
  CloseHandle( o.hEvent );
  return readbytes;
}

// Read a single byte and return it (or -1 for error)
int ser_read_byte( ser_handler id, u32 timeout )
{
  u8 data;
  int res = ser_read( id, &data, 1, timeout );

  return res == 1 ? data : -1;
}

// Write up to the specified number of bytes, return bytes actually written
u32 ser_write( ser_handler id, const u8 *src, u32 size )
{	
  HANDLE hComm = ( HANDLE )id;
  OVERLAPPED o = { 0 };
  DWORD written = 0;
  BOOL fWaitingOnWrite = FALSE;
  
  o.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( WriteFile( hComm, src, size, &written, &o ) == FALSE )
  {
    if( GetLastError() != ERROR_IO_PENDING )
    {
      CloseHandle( o.hEvent );      
      return 0;
    }
    else
      fWaitingOnWrite = TRUE;
  }
  else
  {
    CloseHandle( o.hEvent );
    return written;
  }
    
  if( fWaitingOnWrite )
  {
    BOOL dwRes = WaitForSingleObject( o.hEvent, INFINITE );
    if( dwRes == WAIT_OBJECT_0 )
      if( !GetOverlappedResult( hComm, &o, &written, FALSE ) )
        written = 0;
  }
  
  CloseHandle( o.hEvent );
  return written;
}

// Write a byte to the serial port
u32 ser_write_byte( ser_handler id, u8 data )
{
  return ser_write( id, &data, 1 );
}
