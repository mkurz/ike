
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

#ifndef _LIBITH_H_
#define _LIBITH_H_

#ifdef WIN32
# include <windows.h>
# include <assert.h>
#endif

#ifdef UNIX
# ifdef __linux__
#  include <time.h>
#  include <errno.h>
#  include <assert.h>
#  include <unistd.h>
#  include <pthread.h>
#  include <sys/time.h>
# else
#  include <errno.h>
#  include <unistd.h>
#  include <assert.h>
#  include <pthread.h>
#  include <sys/time.h>
# endif
# include "compat/winstring.h"
#endif

#include <stdio.h>
#include "export.h"

//
// Win32 specific
//

#ifdef WIN32

typedef LARGE_INTEGER ITH_TIMEVAL;

#endif

//
// Unix specific
//

#ifdef UNIX

typedef timeval ITH_TIMEVAL; 
#define Sleep( T ) usleep( T * 1000 )

#endif

//
// thread execution class
//

typedef class DLX _ITH_EXEC
{

#ifdef UNIX

	pthread_t thread;

#endif

	public:

	_ITH_EXEC();

	bool			exec( void * arg );
	virtual long	func( void * arg ) = 0;

}ITH_EXEC;

//
// mutex lock class
//

typedef class DLX _ITH_LOCK
{
	private:

	char		name[ 20 ];
	unsigned long	count;

#ifdef WIN32

	HANDLE	mutex;

#endif

#ifdef UNIX

	pthread_mutex_t mutex;
	pthread_mutexattr_t attr;

#endif

	public:

	_ITH_LOCK();
	~_ITH_LOCK();

	void	setname( const char * lkname );

	bool	lock();
	bool	unlock();

}ITH_LOCK;

//
// execution event class
//

typedef class DLX _ITH_EVENT
{
	public:

	long	delay;

	virtual	bool func() = 0;

}ITH_EVENT;

//
// execution timer class
//

typedef struct _ITH_ENRTY
{
	_ITH_ENRTY * next;
	_ITH_EVENT * event;

	ITH_TIMEVAL	sched;

}ITH_ENTRY;

typedef class DLX _ITH_TIMER : public _ITH_EXEC
{
	private:

	ITH_ENTRY *	head;

	ITH_LOCK	lock;

	long	tres;
	bool	stop;
	bool	exit;

	void	tval_set( ITH_TIMEVAL & tval, long delay = 0 );
	long	tval_cmp( ITH_TIMEVAL & tval1, ITH_TIMEVAL & tval2 );

	public:

	_ITH_TIMER();
	virtual	~_ITH_TIMER();

	virtual long func( void * arg );

	bool	run( long res );
	void	end();

	bool	add( ITH_EVENT * event );
	bool	del( ITH_EVENT * event );

}ITH_TIMER;

#endif
