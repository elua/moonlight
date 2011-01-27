// WinAmp plugin that sends information about the currently playing song to the
// Moonlight display 
// WinAmp specific code adapted from ircex (http://ircex.sourceforge.net)

#include <windows.h> 
#include <process.h>
#include <stdio.h>
#include <limits.h>
#include "gen.h"
#include "wa_ipc.h" 
#include "type.h"
#include "pnet.h"

// *****************************************************************************
// Local data

// Local constants and macros
#define TIMER_PERIOD_MS           25                   
#define MAX_WA_BUF_LEN            40
#define MAX_LINE_SIZE             21
#define UMAX(a, b)                ( (a) > (b) ? (a) : (b) )
#define UMIN(a, b)                ( (a) < (b) ? (a) : (b) )
#define MAXSVAL                   24
#define CMD_SPECTRUM              255
#define CMD_SONGNAME              254
#define CMD_SONGTIME              253
#define CMD_POSITION              252

// Set true to signal the thread should die
int thread_should_die;

// Thread info
HANDLE hThread;
DWORD idThread;

// WinAmp exported functions
char* ( *export_sa_get )( void );
void ( *export_sa_setreq )( int want );

// Serial port data
static char ini_file_name[ MAX_PATH ];
static SOCKET s, cmds;
static struct sockaddr_in moonlight;
  
// Persistent thread data 
static char safalloff[ 150 ];
static char song_title[ MAX_PATH ];
static int wa_volume = -1;

static char sendbuf[ 512 ];

// *****************************************************************************
// Plugin data

// Forward declarations
int init(); 
void config(); 
void quit();

// Plugin data
winampGeneralPurposePlugin plugin = {
  GPPHDR_VER,
  "Moonlight plugin", // Plug-in description 
  init,
  config, 
  quit,
}; 

// *****************************************************************************
// Module functions
 
int config_init()
{                 
	char *p;
	char ip[ 21 ], port[ 21 ];
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
  struct hostent *hp;
  struct sockaddr_in server;
  int length; 
    
  wVersionRequested = MAKEWORD( 2, 0 );
  err = WSAStartup( wVersionRequested, &wsaData );  
  if( err != 0 )
  {
    MessageBox( NULL, "Unable to initialize WinSock", "Moonlight plugin error", MB_OK );  	
    return 1;
  }
	GetModuleFileName( NULL, ini_file_name, sizeof( ini_file_name ) );
	p = ini_file_name;
	while( *p ) 
    p++;
	while( p >= ini_file_name && *p != '.' ) 
    p--;
	strcpy( p + 1 , "ini" );
	GetPrivateProfileString( "moonlight", "ip", "192.168.1.5", ip, 20, ini_file_name );
  GetPrivateProfileString( "moonlight", "port", "31000", port, 20, ini_file_name );
  
  if( ( s = socket( AF_INET, SOCK_DGRAM, 0 ) ) == INVALID_SOCKET_VALUE )
  {
    MessageBox( NULL, "Error creating socket", "Moonlight plugin error", MB_OK );  
    return 1;
  }
  moonlight.sin_family = AF_INET;
  hp = gethostbyname( ip );
  memcpy( &moonlight.sin_addr, hp->h_addr, hp->h_length );
  moonlight.sin_port = htons( atoi( port ) );  
  // Setup socket    
  if( ( cmds = socket( AF_INET, SOCK_DGRAM, 0 ) ) == INVALID_SOCKET_VALUE )
  {
    MessageBox( NULL, "Error creating command socket", "Moonlight plugin error", MB_OK );  
    return 1;
  } 
  length = sizeof( server );
  memset( &server, 0, sizeof( server ) );
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 32000 );
  if( bind( cmds, ( struct sockaddr * )&server, length ) < 0 )
  {
    MessageBox( NULL, "Unable to bind socket", "Moonlight plugin error", MB_OK );  
   return 1; 
  }  
	return 0;
}
  
// The main monitor thread that watches winamp 
static DWORD winamp_thread( LPDWORD lpdwParam )
{
  HWND hWndWinamp = plugin.hwndParent;
  int isPlaying = -1, isPlayingPrev = -1, temp;
  int index;
  double a;
  unsigned i;
  const char *p;
  char *sadata;
  int crtsec = -1;
  int crttrack = -1, totaltracks = -1;
  fd_set fds;
  struct timeval tv;
  socklen_t fromlen;
  struct sockaddr_in from;   
  unsigned char sdata;
        
  while( !thread_should_die ) 
  {
    FD_ZERO( &fds );
    FD_SET( cmds, &fds );
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if( select( cmds + 1, &fds, NULL, NULL, &tv ) > 0 )
    {   
      fromlen = sizeof( from );     
      if( recvfrom( cmds, ( char* )&sdata, 1, 0, ( struct sockaddr* )&from, &fromlen ) == 1 )
      {      
        switch( sdata )
        {
          case 18: // volume up
            SendMessage( hWndWinamp, WM_COMMAND, WINAMP_VOLUMEUP, 0 );
            break;
            
          case 19: // volume down
            SendMessage( hWndWinamp, WM_COMMAND, WINAMP_VOLUMEDOWN, 0 );        
            break;
            
          case 48: // prev track
            SendMessage( hWndWinamp, WM_COMMAND, WINAMP_BUTTON1, 0 );        
            break;
            
          case 49: // next track
            SendMessage( hWndWinamp, WM_COMMAND, WINAMP_BUTTON5, 0 );        
            break;
            
          case 56: // stop
            SendMessage( hWndWinamp, WM_COMMAND, WINAMP_BUTTON4, 0 );
            break;
            
          case 107: // play/pause
            SendMessage( hWndWinamp, WM_COMMAND, SendMessage( hWndWinamp, WM_WA_IPC, 0, IPC_ISPLAYING ) == 1 ? WINAMP_BUTTON3 : WINAMP_BUTTON2, 0 );
            break;
        }
      }
    }
    Sleep( TIMER_PERIOD_MS );
    isPlaying = SendMessage( hWndWinamp, WM_WA_IPC, 0, IPC_ISPLAYING ) == 1;
    if( isPlaying != isPlayingPrev )
    {
      isPlayingPrev = isPlaying;
      if( isPlaying == 0 )
      {
        // TODO: send pause notification
        continue;
      }
    }
    if( isPlaying == 0 )
      continue;
    
    // Get spectrum data
		export_sa_setreq( 1 );
		sadata = export_sa_get();  
		for( i = 0, temp = INT_MIN; i < 75; i ++ )
		{
			if( sadata[ i ] > safalloff[ i ] ) 
        safalloff[ i ] = sadata[ i ];
			else 
        safalloff[ i ] = safalloff[ i ] - 2;
      if( safalloff[ i ] > temp )
        temp = safalloff[ i ];
		}
		if( temp == INT_MIN )
		  continue;
		a = ( double )MAX_LINE_SIZE / MAXSVAL;
		// Scale everything to MAX_LINE_SIZE
		for( i = 0; i < 75; i ++ )
		  safalloff[ i ] = a * UMAX( UMIN( safalloff[ i ], MAXSVAL ), 0 ) + 0.5;
		// And send it 
		sendbuf[ 0 ] = CMD_SPECTRUM;
		for( i = 0; i < 75; i ++ )
		  sendbuf[ i + 1 ] = ( u8 )UMIN ( safalloff[ i ], MAX_LINE_SIZE );
		sendto( s, sendbuf, 76, 0, ( const struct sockaddr* )&moonlight, sizeof( moonlight ) );
        
    // Get index of currently playing song
    index = SendMessage( hWndWinamp, WM_WA_IPC, 0, IPC_GETLISTPOS );
            
    // Get the current playlist file
    p = ( const char* )SendMessage( hWndWinamp, WM_WA_IPC, index, IPC_GETPLAYLISTTITLE );
    if( p && strcmp( p, song_title ) )
    { 
      // Save new file, send message to display
      Sleep( TIMER_PERIOD_MS );
      strcpy( song_title, p );
      sendbuf[ 0 ] = CMD_SONGNAME;
      strcpy( sendbuf + 1, song_title );
		  sendto( s, sendbuf, strlen( sendbuf ), 0, ( const struct sockaddr* )&moonlight, sizeof( moonlight ) );
    }
    
    // Get the current time
    temp = SendMessage( hWndWinamp, WM_WA_IPC, 0, IPC_GETOUTPUTTIME ) / 1000;
    if( temp != crtsec )
    {
      crtsec = temp;
      Sleep( TIMER_PERIOD_MS );
      sendbuf[ 0 ] = CMD_SONGTIME;
      sprintf( sendbuf + 1, "%0d:%02d", crtsec / 60, crtsec % 60 );
 		  sendto( s, sendbuf, strlen( sendbuf ), 0, ( const struct sockaddr* )&moonlight, sizeof( moonlight ) );         
    }
    
    // Get the current list position
    temp = SendMessage( hWndWinamp, WM_WA_IPC, 0, IPC_GETLISTLENGTH );
    if( index != crttrack || temp != totaltracks )
    {
      crttrack = index;
      totaltracks = temp;
      Sleep( TIMER_PERIOD_MS );
      sendbuf[ 0 ] = CMD_POSITION;      
      sprintf( sendbuf + 1, "%d/%d", crttrack + 1, totaltracks );    
 		  sendto( s, sendbuf, strlen( sendbuf ), 0, ( const struct sockaddr* )&moonlight, sizeof( moonlight ) );                      
    }
  }
  return 0;
}

int init() 
{
  // Create the monitor thread
  thread_should_die = 0;
  hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)winamp_thread, 0, 0, &idThread);
  SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
  
  // Get functions
	export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
	export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
	if( export_sa_get == NULL || export_sa_setreq == NULL )
	{
    MessageBox( NULL, "Unable to find function pointers", "Moonlight plugin error", MB_OK );  
    return 1;
  }
        
  // Read config
  return config_init();
} 

void config() 
{  
  MessageBox( NULL, "Config does nothing right now", "Moonlight plugin info", MB_OK );
} 

void quit() 
{
  // Signal our thread to die, if it doesn't after 3 seconds then kill it
  thread_should_die = 1;
  if (hThread) {
    ResumeThread(hThread);
    WaitForSingleObject(hThread, 3000);
    CloseHandle(hThread);
  }
  socket_close( s );
} 

__declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
  return &plugin; 
}
