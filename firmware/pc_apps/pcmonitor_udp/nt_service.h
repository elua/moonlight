#ifndef __NTSERVICE_INCLUDED__
#define __NTSERVICE_INCLUDED__

#include <map>
#include <ostream>
#include "stdh.h"
#include "bitmask.h"


using std::map;
using std_::bitmask;
using std::ostream;
namespace win
{
//-----------------------------------------------------------------------------

typedef void (*NTSERVICE_CALLBACK_FUNCTION)(void);

typedef DWORD NTSERVICE_CONTROL;
typedef DWORD NTSERVICE_ACCEPT;

//-----------------------------------------------------------------------------
class nt_service
{
public:
	
	static nt_service&  instance    ( const char_ * const name_ , std::ostream * const log_ = 0 );
		
	void register_service_main      ( LPSERVICE_MAIN_FUNCTION );
	void register_control_handler   ( NTSERVICE_CONTROL, NTSERVICE_CALLBACK_FUNCTION );
	void register_init_function     ( NTSERVICE_CALLBACK_FUNCTION );
	void accept_control				( NTSERVICE_ACCEPT );	
	
	void		start( void );
	static void	stop ( DWORD exit_code = 0 );	

private:
	
	static VOID WINAPI			service_control_handler( DWORD control_ );
	static VOID WINAPI			nt_service_main( DWORD argc_, char_* argv_[] );
	static NTSERVICE_ACCEPT		convert_control_to_accept( NTSERVICE_CONTROL );

private:

	static const char_*							m_name;
	static SERVICE_STATUS						m_status;
	static SERVICE_STATUS_HANDLE				m_handle;
	static LPSERVICE_MAIN_FUNCTION				m_user_service_main;
	static NTSERVICE_CALLBACK_FUNCTION			m_service_init_fcn;
	static map< NTSERVICE_CONTROL , 
				NTSERVICE_CALLBACK_FUNCTION >	m_callback_map;
	static bitmask< NTSERVICE_ACCEPT >			m_accepted_controls;
	static ostream *							m_log;

private:
	
	// This is kept private because this class is implemented as a 
	// meyer singleton.
	// To create an instance of this class declare a reference to it and call
	// instance, like in the example below:
	// nt_service & service = nt_service::instance("service_name" );

	nt_service(){};
	~nt_service(){};
	nt_service( const nt_service & );
	const nt_service & operator=( const nt_service & );

};
}
#endif
