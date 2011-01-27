// Remote PC monitor server
// Credits : http://www.codeproject.com/KB/system/system_information.aspx
//           http://www.codeproject.com/KB/system/sysinfo.aspx
//           http://www.codeproject.com/KB/system/MultiCPUUsage.aspx
//           http://www.codeproject.com/KB/system/nt_service.aspx

#define _WIN32_WINNT 0x0601

#include "type.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <comdef.h>
#include <psapi.h>
#include "cpu_info.h"
#include "usage.h"
#include "nt_service.h"
#include <strsafe.h>
#include "pnet.h"

#define STRING_BUFFER_SIZE    64
#define CRT_IDX               0
#define NEW_IDX               1
#define MAX_HOSTNAME_SIZE     128
#define MAX_NREQ_COUNT        8

#ifndef VER_SUITE_WH_SERVER    
#define VER_SUITE_WH_SERVER   0x00008000
#endif   

#ifndef SM_SERVERR2
#define SM_SERVERR2           89
#endif

#define PROPID_FIRST          0xA0

// List of properties
enum
{
  // Screen 1
  PROP_MEMORY_DATA,
  PROP_FIRST = PROP_MEMORY_DATA,
  PROP_CPU_LOAD,
  PROP_HDD_DATA,
  PROP_NUM_PROCESS,
  
  // Screen 2
  PROP_MACHINE_NAME,
  PROP_USERNAME,
  PROP_OSNAME,
  PROP_IP,
  
  // Screen 3
  PROP_CPU_NAME,
  PROP_CPU_CORES,  
  PROP_L1CACHE_SIZE,
  PROP_L2CACHE_SIZE,
  PROP_LAST = PROP_L2CACHE_SIZE
};

// Strings for all properties
static char prop_memory_data[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_cpu_load[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_hdd_data[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_num_process[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];

static char prop_machine_name[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_username[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_osname[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_ip[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];

static char prop_cpu_name[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_cpu_cores[ 2 * ( STRING_BUFFER_SIZE + 1 ) ];
static char prop_l1cache_size[ 2 * ( STRING_BUFFER_SIZE + 1  )];
static char prop_l2cache_size[2 * ( STRING_BUFFER_SIZE + 1 ) ];

static SOCKET srv_socket;
static DWORD process_ids[ 4096 ];
static CPUInfo cpu_info;
static CProcessorUsage cpu_usage;

static int prop_needs_refresh[] = { 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0 };
static int force_all_data = 1;
static int initialized = 0;

static char hostname[ MAX_HOSTNAME_SIZE + 1 ];

typedef void ( *p_prop_get )( char * );
static char* const propstr_array[] = 
{
  prop_memory_data, prop_cpu_load, prop_hdd_data, prop_num_process,
  prop_machine_name, prop_username, prop_osname, prop_ip,
  prop_cpu_name, prop_cpu_cores, prop_l1cache_size, prop_l2cache_size
};                         

static char sendbuf[ STRING_BUFFER_SIZE + 2 ];      

static socklen_t srv_fromlen;
static struct sockaddr_in srv_from;                                  

// *****************************************************************************
// System information functions

static void get_cpu_load( char* dest )
{
	sprintf( dest, "%d%%\n", cpu_usage.GetUsage() );
}

static void get_memory_data( char* dest )
{
  u32 total, free;
  
	MEMORYSTATUS memoryStatus;
	ZeroMemory(&memoryStatus,sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	free = memoryStatus.dwAvailPhys/1024/1024;
  total = memoryStatus.dwTotalPhys/1024/1024;
	sprintf( dest, "%d/%dMB\n", free, total );
}

static void get_hdd_data( char* dest )
{
	ULARGE_INTEGER AvailableToCaller, Disk, Free;
	u32 free_size = 0;
  u32 total_size = 0;
  
	if (GetDriveType("c:\\")==DRIVE_FIXED)
	{
		if (GetDiskFreeSpaceEx("c:\\",&AvailableToCaller,&Disk, &Free))
		{
			free_size = Free.QuadPart/1024/1024/1024;
			total_size = Disk.QuadPart/1024/1024/1024;
		}
	}
	sprintf( dest, "%d/%dGB\n", free_size, total_size );
}

static void get_num_processes( char* dest )
{
  DWORD nbytes;
  
  if( EnumProcesses( process_ids, sizeof( process_ids ), &nbytes ) == 0 )
    strcpy( dest, "0\n" );
  else
    sprintf( dest, "%d\n", nbytes / sizeof( DWORD ) );
}

static void get_computer_name( char* dest )
{
	DWORD Size=STRING_BUFFER_SIZE;
	
	strcpy( dest, "unknown\n" );	
  if(GetComputerName(dest,&Size))
    strcat( dest, "\n" );
}

static void get_username( char *dest )
{
	DWORD Size=STRING_BUFFER_SIZE;
	
	strcpy( dest, "unknown\n" );
	if(GetUserName(dest,&Size))
	 strcat( dest, "\n" );
}

static void get_os( char *dest )
{
  OSVERSIONINFOEX versioninfo;
  SYSTEM_INFO sysinfo;
  DWORD major, minor;
  
  strcpy( dest, "unknown\n" );
  versioninfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
  GetSystemInfo( &sysinfo );
  if( GetVersionEx( ( LPOSVERSIONINFO )&versioninfo ) )
  {
    major = versioninfo.dwMajorVersion;
    minor = versioninfo.dwMinorVersion;
    switch( major )
    {
      case 6:
        switch( minor )
        {
          case 1:
            if(versioninfo.wProductType == VER_NT_WORKSTATION )
              strcpy( dest, "Windows 7" );
            else
              strcpy( dest, "Windows Server 2008 R2" );
            break;
            
          case 0:
            if(versioninfo.wProductType == VER_NT_WORKSTATION)
              strcpy( dest, "Windows Vista" );
            else
              strcpy( dest, "Windows Server 2008" );
          break;
        }
        break;
        
      case 5:
        switch( minor )
        {
          case 2:
            if(GetSystemMetrics(SM_SERVERR2) != 0)
              strcpy( dest, "Windows Server 2003 R2" );
            else if( versioninfo.wSuiteMask & VER_SUITE_WH_SERVER )
              strcpy( dest, "Windows Home Server" );
            else if (GetSystemMetrics(SM_SERVERR2) == 0)
              strcpy( dest, "Windows Server 2003" );
            else if( (versioninfo.wProductType == VER_NT_WORKSTATION) && (sysinfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64))
              strcpy( dest, "Windows XP Professional x64 Edition" );
            break;
            
          case 1:
            strcpy( dest, "Windows XP" );
            break;
            
          case 0:
            strcpy( dest, "Windows 2000" );
            break;
            
        }
        break;
    }
    strcat( dest, "\n" ); 
  }
}

static void get_ip( char* dest )
{
  strcpy( dest, "unknown\n" );
  
	WORD wVersionRequested;
  WSADATA wsaData;
  char name[255];
	PHOSTENT hostinfo;
	wVersionRequested = MAKEWORD(2,0);
	
	if (WSAStartup(wVersionRequested, &wsaData)==0)
	{
		if(gethostname(name, sizeof(name))==0)
		{
			if((hostinfo=gethostbyname(name)) != NULL)
			{
				strcpy( dest, inet_ntoa(*(struct in_addr*)* hostinfo->h_addr_list) );
				strcat( dest, "\n" );
			}
		}
		
		WSACleanup();
	} 
}

static void get_cpu_name( char* dest )
{
  sprintf( dest, "%s (stepping %s)\n", cpu_info.GetVendorString(), cpu_info.GetSteppingCode() );
}

static void get_cpu_freq( char* dest )
{
  sprintf( dest, "%dMHz\n", cpu_info.GetProcessorClockFrequency() );
}

static void get_cpu_cores( char* dest )
{
  sprintf( dest, "%d\n", cpu_info.GetLogicalProcessorsPerPhysical() );
}

static void null_getter( char* dest )
{
  strcpy( dest, "\n" );
}

static void get_uptime( char* dest )
{
  FILETIME IdleTime;
  FILETIME KernelTime;
  FILETIME UserTime;
  ULARGE_INTEGER idle;
  ULARGE_INTEGER kernel;
  ULARGE_INTEGER user;
  ULARGE_INTEGER total;
  FILETIME TotalTime;
  SYSTEMTIME actual;
  
  GetSystemTimes( &IdleTime, &KernelTime, &UserTime );
  idle.LowPart = IdleTime.dwLowDateTime;
  idle.HighPart = IdleTime.dwHighDateTime;
  kernel.LowPart = KernelTime.dwLowDateTime;
  kernel.HighPart = KernelTime.dwHighDateTime;
  user.LowPart = UserTime.dwLowDateTime;
  user.HighPart = UserTime.dwHighDateTime;
  total.QuadPart = idle.QuadPart + kernel.QuadPart + user.QuadPart;
  TotalTime.dwLowDateTime = total.LowPart;
  TotalTime.dwHighDateTime = total.HighPart;
  FileTimeToSystemTime( &TotalTime, &actual );
  sprintf( dest, "%dd:%dh:%dm\n", actual.wDay, actual.wHour, actual.wMinute );
}

static const p_prop_get prop_getters[] =
{
  get_memory_data, get_cpu_load, get_hdd_data, get_num_processes,
  get_computer_name, get_username, get_os, get_ip,
  get_cpu_name, get_cpu_freq, get_cpu_cores, get_uptime
};
     
// *****************************************************************************
// Windows service implmentation
    
using win::nt_service;

static int service_init()
{
  LONG res;
  int length;
  struct sockaddr_in server;
  HKEY hKey;
  DWORD data_len;
  long srv_port;
  fd_set fds;
  struct timeval tv;
  char g[ 128 ];
  
  // Setup networking in Windows
#ifdef WIN32_BUILD
  // The socket subsystem must be initialized if working in Windows
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
 
  wVersionRequested = MAKEWORD( 2, 0 );
  err = WSAStartup( wVersionRequested, &wsaData );  
  if( err != 0 )
  {
    OutputDebugString( "Unable to initialize the socket subsystem\n" ); 
    return 0;
  }
#endif // #ifdef WIN32_BUILD

  srv_port = 0;
  hostname[ 0 ] = '\0';
  if( ( res = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT( "Software\\elua.org\\pcmonitor" ), 0, KEY_QUERY_VALUE | KEY_READ | KEY_ENUMERATE_SUB_KEYS, &hKey ) ) == ERROR_SUCCESS )
  {
    data_len = MAX_HOSTNAME_SIZE; 
    RegQueryValueEx( hKey, TEXT( "hostname" ), NULL, NULL, ( LPBYTE )hostname, &data_len );
    data_len = sizeof( long );
    RegQueryValueEx( hKey, TEXT( "port" ), NULL, NULL, ( LPBYTE )&srv_port, &data_len );
    RegCloseKey( hKey );
  }
  else
  {
    OutputDebugString( "Cannot find configuration" );
    return 0;
  }
  sprintf(g,"Hostname: %s Port: %d", hostname, srv_port );
  OutputDebugString(g);
  
  // Setup socket    
  if( ( srv_socket = socket( AF_INET, SOCK_DGRAM, 0 ) ) == INVALID_SOCKET_VALUE )
  {
    OutputDebugString( "Unable to create socket\n" );
    return 0;
  } 
  length = sizeof( server );
  memset( &server, 0, sizeof( server ) );
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( srv_port );
  if( bind( srv_socket, ( struct sockaddr * )&server, length ) < 0 )
  {
   OutputDebugString( "Unable to bind socket" );
   return 0; 
  }
  
  // Flush data
  FD_ZERO( &fds );
  FD_SET( srv_socket, &fds );
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  if( select( srv_socket + 1, &fds, NULL, NULL, &tv ) > 0 )  
    recvfrom( srv_socket, ( char* )process_ids, 4096, 0, ( struct sockaddr* )&server, &length );

  force_all_data = 1;
  memset( &srv_from, 0, sizeof( srv_from ) );
  return 1;
}

void WINAPI my_service_main(DWORD argc, char_* argv[])
{
  u8 sdata;
  fd_set fds;
  struct timeval tv;
  socklen_t fromlen;
  struct sockaddr_in from;   
        
  if( !initialized )
  {  
    if( !service_init() )
    {
      nt_service::stop( -1 );
      return;
    }
    else
      initialized = 1;
  }
  
  FD_ZERO( &fds );
  FD_SET( srv_socket, &fds );
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  if( select( srv_socket + 1, &fds, NULL, NULL, &tv ) > 0 )
  {   
    fromlen = sizeof( from );
    if( recvfrom( srv_socket, ( char* )&sdata, 1, 0, ( struct sockaddr* )&from, &fromlen ) == 1 && ( sdata == PROPID_FIRST ) )
    {
      OutputDebugString( "sending all data" );
      force_all_data = 1;
      srv_fromlen = fromlen;
      srv_from = from;
    }
  }
      
  if( srv_from.sin_addr.s_addr != 0 )
  {
    // Read the data
    for( unsigned i = PROP_FIRST; i <= PROP_LAST; i ++ )
      if( prop_needs_refresh[ i ] || force_all_data )
        if( prop_getters[ i ] )
          prop_getters[ i ]( propstr_array[ i ] );
              
    // Compare with old data, send only the changes
    for( unsigned i = PROP_FIRST; i <= PROP_LAST; i ++ )
      if( ( prop_needs_refresh[ i ] && strcmp( propstr_array[ i ] + STRING_BUFFER_SIZE + 1, propstr_array[ i ] ) ) || force_all_data )
      {
        strcpy( propstr_array[ i ] + STRING_BUFFER_SIZE + 1, propstr_array[ i ] );   
        sendbuf[ 0 ] = PROPID_FIRST + i;
        strcpy( sendbuf + 1, propstr_array[ i ] );
        sendto( srv_socket, sendbuf, strlen( propstr_array[ i ] ), 0, ( const struct sockaddr* )&srv_from, srv_fromlen );
      }
    force_all_data = 0;
  }            
  
  Sleep( 200 );  
}

void my_init_fcn()
{  
}

void my_shutdown_fcn()
{
  closesocket( srv_socket );
  initialized = 0;
  force_all_data = 1;
	Beep( 1000, 1000 );
}

void main(DWORD argc, LPWSTR* argv)
{
	// creates an access point to the instance of the service framework
	nt_service&  service = nt_service::instance("Moonlight PC monitor service");

	// register "my_service_main" to be executed as the service main method 
	service.register_service_main( my_service_main );

	// register "my_init_fcn" as initialization fcn
	service.register_init_function( my_init_fcn );
	
	// config the service to accept stop controls. Do nothing when it happens
	service.accept_control( SERVICE_ACCEPT_STOP );

	// config the service to accept shutdown controls and do something when receive it 
	service.register_control_handler( SERVICE_CONTROL_SHUTDOWN, my_shutdown_fcn );
		
	service.start();
}
