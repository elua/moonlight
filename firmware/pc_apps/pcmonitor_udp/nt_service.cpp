#include "nt_service.h"

using namespace win;

//-----------------------------------------------------------------------------
// meyer's singletom implementation
//-----------------------------------------------------------------------------
nt_service& nt_service::instance(	const char_ * const name_ , std::ostream * const log_ )									
{
	m_name = name_;
	m_log  = log_;

	static  nt_service obj; 
	return  obj;
}
//-----------------------------------------------------------------------------
void nt_service::register_service_main( LPSERVICE_MAIN_FUNCTION service_main_ )
{
	m_user_service_main = service_main_;
}
//-----------------------------------------------------------------------------
void nt_service::register_control_handler( NTSERVICE_CONTROL			ctrl_,
										   NTSERVICE_CALLBACK_FUNCTION	ctrl_handler_ )
{
	if ( ctrl_handler_ )
	{
		m_callback_map[ ctrl_ ] = ctrl_handler_;
		m_accepted_controls << convert_control_to_accept( ctrl_ );
	}
}
//-----------------------------------------------------------------------------
void nt_service::register_init_function( NTSERVICE_CALLBACK_FUNCTION  init_fcn_ )
{
	m_service_init_fcn = init_fcn_;
}
//-----------------------------------------------------------------------------
void nt_service::accept_control( NTSERVICE_ACCEPT control_ )
{
	m_accepted_controls << control_;	
}
//-----------------------------------------------------------------------------
void nt_service::start()
{
	SERVICE_TABLE_ENTRY service_table[2];
    
	service_table[0].lpServiceName = const_cast<char_*>( m_name );
	service_table[0].lpServiceProc = & nt_service_main;
	service_table[1].lpServiceName = 0;
    service_table[1].lpServiceProc = 0;

	StartServiceCtrlDispatcher(service_table);
}
//-----------------------------------------------------------------------------
void nt_service::stop( DWORD exit_code_ )
{
	if ( m_handle )
	{
		m_status.dwCurrentState  = SERVICE_STOPPED;
		m_status.dwWin32ExitCode = exit_code_; 
		SetServiceStatus ( m_handle, & m_status );
	}
}
//-----------------------------------------------------------------------------
NTSERVICE_ACCEPT nt_service::convert_control_to_accept( NTSERVICE_CONTROL control_ )
{
	switch( control_ )
	{
		case SERVICE_CONTROL_STOP:
			return SERVICE_ACCEPT_STOP;

		case SERVICE_CONTROL_PAUSE:
		case SERVICE_CONTROL_CONTINUE:
			return SERVICE_ACCEPT_PAUSE_CONTINUE;

		case SERVICE_CONTROL_SHUTDOWN:
			return SERVICE_ACCEPT_SHUTDOWN;

		case SERVICE_CONTROL_PARAMCHANGE:
			return SERVICE_ACCEPT_PARAMCHANGE;

		case SERVICE_CONTROL_NETBINDADD:
		case SERVICE_CONTROL_NETBINDREMOVE:
		case SERVICE_CONTROL_NETBINDENABLE:
		case SERVICE_CONTROL_NETBINDDISABLE:
			return SERVICE_ACCEPT_NETBINDCHANGE;

		case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
			return SERVICE_ACCEPT_HARDWAREPROFILECHANGE;

		case SERVICE_CONTROL_POWEREVENT:
			return SERVICE_ACCEPT_POWEREVENT;

		case SERVICE_CONTROL_SESSIONCHANGE:
			return SERVICE_ACCEPT_SESSIONCHANGE;	

		case SERVICE_CONTROL_INTERROGATE:
		case SERVICE_CONTROL_DEVICEEVENT:
		default:
			return NTSERVICE_ACCEPT( 0 );

	}
}
//-----------------------------------------------------------------------------
VOID WINAPI nt_service::service_control_handler( NTSERVICE_CONTROL control_ )
{
	if ( m_callback_map.find( control_ ) != m_callback_map.end() )
	{
		m_callback_map[ control_ ]();
	}

	switch( control_ ) 
    { 
        case SERVICE_CONTROL_STOP: 
            m_status.dwCurrentState  = SERVICE_STOPPED; 
            break;
        case SERVICE_CONTROL_SHUTDOWN: 
            m_status.dwCurrentState  = SERVICE_STOPPED; 
            break;
		case SERVICE_CONTROL_PAUSE:
			m_status.dwCurrentState  = SERVICE_PAUSED; 
        default:
			m_status.dwCurrentState  = SERVICE_RUNNING; 
            break;
	}
	m_status.dwWin32ExitCode = 0; 
    SetServiceStatus ( m_handle, & m_status );
}
//-----------------------------------------------------------------------------
VOID WINAPI nt_service::nt_service_main( DWORD argc_, char_* argv_[] )
{
	m_status.dwServiceType        = SERVICE_WIN32; 
    m_status.dwCurrentState       = SERVICE_START_PENDING; 
	m_status.dwControlsAccepted   = m_accepted_controls.to_dword();
     
    m_handle = RegisterServiceCtrlHandler( m_name, service_control_handler );

    if ( m_handle == 0 ) 
    { 
		if ( m_log )*m_log<<"[E] - nt_service: RegisterServiceCtrlHandler failed\n";
		return; 
    }  
    
	if ( m_service_init_fcn )
	{
		try
		{
			m_service_init_fcn();
		}
		catch(...)
		{
			if ( m_log )*m_log<<"[E] - nt_service: m_service_init_fcn failed\n";
			nt_service::stop(-1);
			return;
		}
	}
	    
    // We report the running status to SCM. 
    m_status.dwCurrentState = SERVICE_RUNNING; 
    SetServiceStatus( m_handle, & m_status);
     
    // The worker loop of a service
    while ( m_status.dwCurrentState == SERVICE_RUNNING )
	{
		try
		{
			m_user_service_main( argc_, argv_ );
		}
		catch(...)
		{
			if ( m_log )*m_log<<"[E] - nt_service: in service main\n";
			nt_service::stop( -1 );
		}
	}
    return; 
}
//-----------------------------------------------------------------------------
const char_*						nt_service::m_name					= 0;
SERVICE_STATUS						nt_service::m_status;
SERVICE_STATUS_HANDLE				nt_service::m_handle				= 0;
LPSERVICE_MAIN_FUNCTION				nt_service::m_user_service_main		= 0;
NTSERVICE_CALLBACK_FUNCTION			nt_service::m_service_init_fcn		= 0;
map< NTSERVICE_CONTROL , 
	 NTSERVICE_CALLBACK_FUNCTION >	nt_service::m_callback_map;
bitmask< NTSERVICE_ACCEPT >			nt_service::m_accepted_controls		= 0;
ostream *							nt_service::m_log					= 0;
