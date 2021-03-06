
/*
 * Copyright (c) 2007
 *      Shrew Soft Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Redistributions in any form must be accompanied by information on
 *    how to obtain complete source code for the software and any
 *    accompanying software that uses the software.  The source code
 *    must either be included in the distribution or be available for no
 *    more than the cost of distribution plus a nominal fee, and must be
 *    freely redistributable under reasonable conditions.  For an
 *    executable file, complete source code means the source code for all
 *    modules it contains.  It does not include source code for modules or
 *    files that typically accompany the major components of the operating
 *    system on which the executable file runs.
 *
 * THIS SOFTWARE IS PROVIDED BY SHREW SOFT INC ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 * NON-INFRINGEMENT, ARE DISCLAIMED.  IN NO EVENT SHALL SHREW SOFT INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * AUTHOR : Matthew Grooms
 *          mgrooms@shrew.net
 *
 */

#include "iked.h"

IKED iked;

#ifdef WIN32

//
// win32 service specific section
//

#define SERVICE_NAME	"iked"
#define SERVICE_DESC	"ShrewSoft IKE Daemon"
#define INSTALL_HKEY	"SOFTWARE\\ShrewSoft\\vpn"

SERVICE_STATUS_HANDLE	service_handle;
SERVICE_STATUS			service_status;

void service_add( char * path )
{
	char prog[ MAX_PATH ];
	sprintf( prog, "%s -service", path );

	SC_HANDLE hmngr = NULL;
	SC_HANDLE hsrvc = NULL;

	hmngr = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS );

	if( hmngr == NULL )
	{
		printf( "!! : unable to open service manager\n" );
		exit( 1 );
	}
	
	hsrvc = CreateService(
				hmngr,
				SERVICE_NAME,
				SERVICE_DESC,
				SERVICE_ALL_ACCESS,
				SERVICE_WIN32_OWN_PROCESS,
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				prog,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL );

	if( hsrvc != NULL )
		printf( "ii : service has been registered\n" );
	else
		printf( "!! : unable to register service\n" );
}

void service_del( char * path )
{
	SC_HANDLE hmngr = NULL;
	SC_HANDLE hsrvc = NULL;

	hmngr = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS );

	if( hmngr == NULL )
	{
		printf( "!! : unable to open service manager\n" );
		exit( 1 );
	}

	hsrvc = OpenService(
				hmngr,
				SERVICE_NAME,
				SERVICE_ALL_ACCESS );

	if( hmngr == NULL )
	{
		printf( "!! : unable to open service\n" );
		exit( 1 );
	}

	if( DeleteService( hsrvc ) )
		printf( "ii : service has been deregistered\n" );
	else
		printf( "!! : unable to deregister service\n" );
}

void __stdcall service_ctrl( DWORD opcode ) 
{
	switch( opcode )
	{ 
		case SERVICE_CONTROL_PAUSE:

			//
			// pause daemon operation
			//

			service_status.dwCurrentState = SERVICE_PAUSED; 

			break; 

		case SERVICE_CONTROL_CONTINUE: 

			//
			// continue daemon operation
			//

			service_status.dwCurrentState = SERVICE_RUNNING; 

			break;

		case SERVICE_CONTROL_STOP: 

			//
			// stop daemon operation
			//

			iked.halt();

			break;

		case SERVICE_CONTROL_INTERROGATE:

			//
			// send service status update
			//

			break; 

		default:

			//
			// unknown service opcode
			//

			printf( "ii : unknown service opcode\n" );
	} 

	//
	// send service status update
	//

	if( !SetServiceStatus( service_handle, &service_status ) )
		printf( "ii : unable to update service status" );

    return; 
}

void __stdcall service_main( DWORD argc, LPTSTR * argv ) 
{
	//
	// Register service control handler
	//

	service_status.dwServiceType        = SERVICE_WIN32;
	service_status.dwCurrentState       = SERVICE_START_PENDING;
	service_status.dwControlsAccepted   = SERVICE_ACCEPT_STOP;
	service_status.dwWin32ExitCode      = 0;
	service_status.dwServiceSpecificExitCode = 0;
	service_status.dwCheckPoint         = 0;
	service_status.dwWaitHint           = 0;
 
    service_handle = RegisterServiceCtrlHandler(
						SERVICE_NAME,
						service_ctrl );
 
	if( service_handle == NULL )
	{ 
		printf( "ii : unable to register service control handler\n" );
		return; 
	} 

	if( iked.init( 0 ) == LIBIKE_OK )
	{
		//
		// daemon initialized
		//

		service_status.dwCurrentState	= SERVICE_RUNNING;
		service_status.dwCheckPoint		= 0; 
		service_status.dwWaitHint		= 0; 

		SetServiceStatus( service_handle, &service_status );

		printf( "ii : service started\n" );
	}
	else
	{ 
		//
		// daemon initialization failed
		//

		service_status.dwCurrentState	= SERVICE_STOPPED;
		service_status.dwCheckPoint		= 0;
		service_status.dwWaitHint		= 0;
		service_status.dwWin32ExitCode	= -1;
		service_status.dwServiceSpecificExitCode = 0;

		SetServiceStatus( service_handle, &service_status ); 

		printf( "ii : service failed to start\n" );

		return; 
	}

	//
	// run daemon main loop
	//

	iked.loop();

	//
	// notify service has stopped
	//

	service_status.dwWin32ExitCode	= 0;
	service_status.dwCurrentState	= SERVICE_STOPPED;
	service_status.dwCheckPoint		= 0;
	service_status.dwWaitHint		= 0;

	SetServiceStatus( service_handle, &service_status );
}

#endif

#ifdef UNIX

//
// unix daemon specific section
//

void daemon_stop( int sig_num )
{
	//
	// stop daemon operation
	//

	iked.halt();
}

#endif

int main( int argc, char * argv[], char * envp[] )
{

#ifdef WIN32

	//
	// check command line parameters
	//

	bool service = false;

	for( long count = 1; count < argc; count++ )
	{
		if( !strcmp( argv[ count ], "-service" ) )
			service = true;

		if( !strcmp( argv[ count ], "-register" ) )
		{
			service_add( argv[ 0 ] );
			return 0;
		}

		if( !strcmp( argv[ count ], "-deregister" ) )
		{
			service_del( argv[ 0 ] );
			return 0;
		}
	}

	//
	// are we running as a service
	// or a foreground application
	//

	if( service )
	{
		//
		// running as a service
		//

		SERVICE_TABLE_ENTRY	ste[ 2 ];
		
		ste[ 0 ].lpServiceName = SERVICE_NAME;
		ste[ 0 ].lpServiceProc = service_main;
		ste[ 1 ].lpServiceName = NULL;
		ste[ 1 ].lpServiceProc = NULL;
		
		if( !StartServiceCtrlDispatcher( ste ) )
			printf( "ii : StartServiceCtrlDispatcher error = %d\n", GetLastError() );
	}
	else
	{
		//
		// running as an application
		//

		if( iked.init( 0 ) != LIBIKE_OK )
			return false;

		//
		// run daemon main loop
		//

		iked.loop();
	}

#endif

#ifdef UNIX

	//
	// check command line parameters
	//

	bool service = true;

	for( long count = 1; count < argc; count++ )
	{
		if( !strcmp( argv[ count ], "-F" ) )
			service = false;

	}

	//
	// setup stop signal
	//

	signal( SIGINT, daemon_stop );
	signal( SIGPIPE, SIG_IGN );

	//
	// initialize
	//

	if( iked.init( 0 ) != LIBIKE_OK )
		return -1;

	//
	// are we running as a deamon
	//

	if( service )
		daemon( 0, 0 );

	//
	// run daemon main loop
	//

	iked.loop();

#endif
	return 0;
}

