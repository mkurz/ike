
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

#include "libpfk.h"

//
// PFKI MESSAGE CLASS
//

bool _PFKI_MSG::append( long size )
{
	long new_size = msg_size + size;
	unsigned char * new_buff = new unsigned char[ new_size ];
	if( new_buff == NULL )
		return false;

	memset( new_buff, 0, new_size );

	if( msg_buff != NULL )
	{
		memcpy( new_buff, msg_buff, msg_size );
		delete [] msg_buff;
	}

	msg_buff = new_buff;
	msg_size = new_size;

	hdr = ( sadb_msg * ) msg_buff;

	return true;
}

bool _PFKI_MSG::local()
{
	if( msg_buff == NULL )
		return false;

	return ( ( long ) hdr->sadb_msg_pid == getpid() );
}

void _PFKI_MSG::reset()
{
	if( msg_buff != NULL )
		delete [] msg_buff;

	msg_buff = NULL;
	msg_size = 0;
}

_PFKI_MSG::_PFKI_MSG()
{
	msg_buff = NULL;
	msg_size = 0;

	hdr = NULL;
}

_PFKI_MSG::~_PFKI_MSG()
{
	reset();
}

//
// PFKI UNIX Specific
//

#ifdef UNIX

// 
// GCC has problems resolving these
// functions due to c/c++ name space
// collisions
//

inline int sockclose( int sock )
{
	return close( sock );
}

inline int sockselect( int nfds, fd_set * rfds, fd_set * wfds, fd_set * xfds, struct timeval *to )
{
	return select( nfds, rfds, wfds, xfds, to );
}

_PFKI::_PFKI()
{
	sock = -1;
}

_PFKI::~_PFKI()
{
	close();
}

long _PFKI::wait_msg()
{
	if( sock == -1 )
		return PFKI_FAILED;

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	fd_set fdset;
	FD_ZERO( &fdset ); 
	FD_SET( sock, &fdset );

	long result = sockselect( sock + 1, &fdset, NULL, NULL, &tv );
	if( result > 0 )
		return PFKI_OK;

	return PFKI_NODATA;
}

long _PFKI::send_msg( PFKI_MSG & msg )
{
	if( sock == -1 )
		return PFKI_FAILED;

	long result = send( sock, msg.msg_buff, msg.msg_size, 0 );

	if( result < 1 )
		printf( "!! : pfki send error ( %i )\n", errno );

	if( result != msg.msg_size )
		return PFKI_FAILED;

	return PFKI_OK;
}

long _PFKI::recv_msg( PFKI_MSG & msg, bool peek )
{
	if( sock == -1 )
		return PFKI_FAILED;

	long flags = 0;
	if( peek )
		flags = MSG_PEEK;

	long result = recv( sock, msg.msg_buff, msg.msg_size, flags );

	if( result < 1 )
		printf( "!! : pfki recv error ( %i )\n", errno );

	if( result != msg.msg_size )
		return PFKI_FAILED;

	return PFKI_OK;
}

long _PFKI::open()
{
	close();

	//
	// open our pfkey socket
	//

	sock = socket( PF_KEY, SOCK_RAW, PF_KEY_V2 );
	if( sock < 0 )
		return PFKI_FAILED;

	//
	// set socket buffer size
	//

	const int buffsize = PFKEY_BUFFSIZE;
	setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof( buffsize ) );
	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof( buffsize ) );

	//
	// set socket to non-blocking
	//

	if( fcntl( sock, F_SETFL, O_NONBLOCK ) == -1 )
		return PFKI_FAILED;

	return PFKI_OK;
}

void _PFKI::close()
{
	if( sock != -1 )
		sockclose( sock );
}

#endif

//
// PFKI WIN32 Specific
//

#ifdef WIN32

_PFKI::_PFKI()
{
	hpipe = INVALID_HANDLE_VALUE;
	memset( &olapp, 0, sizeof( olapp ) );
	wait = false;
}

_PFKI::~_PFKI()
{
	CloseHandle( olapp.hEvent );
}

void CALLBACK msg_end( DWORD result, DWORD size, LPOVERLAPPED overlapped )
{
}

long _PFKI::wait_msg()
{
	if( !wait )
	{
		//
		// begin an overlapped read operation.
		// if successful, set wait to true so
		// we remember that the operation is
		// in progress
		//

		memset( &tmsg, 0, sizeof( tmsg ) );

		if( ReadFileEx( hpipe, &tmsg, sizeof( tmsg ), &olapp, &msg_end ) )
			wait = true;
		else
		{
			long result = GetLastError();

			if( ( result == ERROR_INVALID_HANDLE ) ||
				( result == ERROR_BROKEN_PIPE ) )
				return PFKI_FAILED;
		}
	}

	//
	// wait for our overlapped read
	// operation to complete.
	//

	if( !SleepEx( 10, true ) )
		return PFKI_NODATA;

	wait = false;

	return PFKI_OK;
}

long _PFKI::send_msg( PFKI_MSG & msg )
{
	DWORD result;

	WriteFile( hpipe, msg.msg_buff, msg.msg_size, &result, NULL );

	if( long( result ) != msg.msg_size )
		return PFKI_FAILED;

	return PFKI_OK;
}

long _PFKI::recv_msg( PFKI_MSG & msg, bool peek )
{
	DWORD result;

	if( peek )
	{
		if( !tmsg.sadb_msg_type || !tmsg.sadb_msg_len )
			return PFKI_FAILED;

		memcpy( msg.msg_buff, &tmsg, sizeof( tmsg ) );
		result = sizeof( tmsg );
	}
	else
	{
		ReadFile( hpipe, msg.msg_buff + sizeof( tmsg ), msg.msg_size - sizeof( tmsg ), &result, NULL );

		memcpy( msg.msg_buff, &tmsg, sizeof( tmsg ) );
		result += sizeof( tmsg );
	}

	if( long( result ) != msg.msg_size )
		return PFKI_FAILED;

	return PFKI_OK;
}

long _PFKI::open()
{
	if( !WaitNamedPipe( PFKI_PIPE_NAME, 3000 ) )
		return false;

	hpipe = INVALID_HANDLE_VALUE;

	hpipe = CreateFile(
				PFKI_PIPE_NAME,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				NULL );

	if( hpipe == INVALID_HANDLE_VALUE )
		return PFKI_FAILED;

	return PFKI_OK;
}

void _PFKI::close()
{
	if( hpipe != INVALID_HANDLE_VALUE )
		CloseHandle( hpipe );
}

#endif

//
// SHARED HANDLERS
//

bool _PFKI::sockaddr_len( int safam, int & salen )
{
	switch( safam )
	{
		case AF_INET:
			salen = sizeof( sockaddr_in );
			return true;
	}

	printf( "XX : address family %i unhandled\n", safam );

	return false;
}

long _PFKI::buff_get_ext( PFKI_MSG & msg, sadb_ext ** ext, long type )
{
	unsigned char * buff = msg.msg_buff;
	int				size = msg.msg_size;

	buff += sizeof( sadb_msg );
	size -= sizeof( sadb_msg );

	while( 1 )
	{
		//
		// make sure the buffer holds
		// enough data to contain an
		// extension header
		//

		if( long( sizeof( sadb_ext ) ) > size )
		{
			printf( "XX : extension not found\n" );

			return PFKI_FAILED;
		}

		sadb_ext * ext_head = ( sadb_ext * ) buff;

		//
		// make sure the buffer holds
		// enough data to contain the
		// extension body
		//

		if( PFKEY_EXTLEN( ext_head ) > size )
		{
			printf( "XX : buffer too small for ext body ( %i > %i )\n",
				PFKEY_EXTLEN( ext_head ),
				size );

			return PFKI_FAILED;
		}

		//
		// stop evaluation if this is
		// the requested extension
		//

		if( ext_head->sadb_ext_type == type )
		{
			//
			// sainfo the pointers and size
			//

			*ext = ( sadb_ext * ) buff;

			break;
		}

		//
		// move to next extension
		//

		buff += PFKEY_EXTLEN( ext_head );
		size -= PFKEY_EXTLEN( ext_head );
	}

	return PFKI_OK;
}

long _PFKI::buff_add_ext( PFKI_MSG & msg, sadb_ext ** ext, long xlen, bool unit64 )
{
	int oset = msg.msg_size;
	xlen = PFKEY_ALIGN8( xlen );

	msg.append( xlen );
	*ext = ( sadb_ext * )( msg.msg_buff + oset );

	if( unit64 )
		( *ext )->sadb_ext_len = ( u_int16_t ) PFKEY_UNIT64( xlen );
	else
		( *ext )->sadb_ext_len = ( u_int16_t ) xlen;

	return PFKI_OK;
}

long _PFKI::buff_get_address( sadb_address * ext, PFKI_ADDR & addr )
{
	unsigned char * buff = ( unsigned char * ) ext;
	int size = PFKEY_UNUNIT64( ext->sadb_address_len );

	addr.proto = ext->sadb_address_proto;
	addr.prefix = ext->sadb_address_prefixlen;

	buff += sizeof( sadb_address );
	size -= sizeof( sadb_address );

	sockaddr * saddr = ( sockaddr * ) buff;

	int salen;
	if( !sockaddr_len( saddr->sa_family, salen ) )
		return PFKI_FAILED;

	if( size < salen )
	{
		printf( "!! : pfkey address size mismatch\n" );
		return PFKI_FAILED;
	}

	memcpy( &addr.saddr, saddr, salen );

	return PFKI_OK;
}

long _PFKI::buff_set_address( sadb_address * ext, PFKI_ADDR & addr )
{
	unsigned char * buff = ( unsigned char * ) ext;
	int size = PFKEY_UNUNIT64( ext->sadb_address_len );

	ext->sadb_address_proto = addr.proto;
	ext->sadb_address_prefixlen = addr.prefix;

	buff += sizeof( sadb_address );
	size -= sizeof( sadb_address );

	int salen;
	if( !sockaddr_len( addr.saddr.sa_family, salen ) )
		return PFKI_FAILED;

	if( size < salen )
	{
		printf( "!! : pfkey address size mismatch\n" );
		return PFKI_FAILED;
	}

	memcpy( buff, &addr.saddr, salen );

	return PFKI_OK;
}

long _PFKI::buff_set_key( sadb_key * ext, PFKI_KEY & key )
{
	unsigned char * buff = ( unsigned char * ) ext;
	int size = PFKEY_UNUNIT64( ext->sadb_key_len );

	ext->sadb_key_bits = key.length * 8;

	buff += sizeof( sadb_key );
	size -= sizeof( sadb_key );

	if( size < key.length )
	{
		printf( "!! : pfkey key size mismatch ( %i < %i )\n", size, key.length );
		return PFKI_FAILED;
	}

	memcpy( buff, key.keydata, key.length );

	return PFKI_OK;
}

long _PFKI::buff_get_key( sadb_key * ext, PFKI_KEY & key )
{
	unsigned char * buff = ( unsigned char * ) ext;
	int size = PFKEY_UNUNIT64( ext->sadb_key_len );

	buff += sizeof( sadb_key );
	size -= sizeof( sadb_key );

	if( !ext->sadb_key_bits )
		return PFKI_FAILED;

	key.length = ext->sadb_key_bits / 8;

	if( size < key.length )
	{
		printf( "!! : pfkey key size mismatch ( %i < %i )\n", size, key.length );
		return PFKI_FAILED;
	}

	memcpy( key.keydata, buff, key.length );

	return PFKI_OK;
}

long _PFKI::buff_get_ipsec( sadb_x_policy * ext, PFKI_SPINFO & spinfo )
{
	unsigned char * buff = ( unsigned char * ) ext;
	int size = PFKEY_UNUNIT64( ext->sadb_x_policy_len );

	buff += sizeof( sadb_x_policy );
	size -= sizeof( sadb_x_policy );

	long xindex = 0;

	while( size >= long( sizeof( sadb_x_ipsecrequest ) ) )
	{
		sadb_x_ipsecrequest * ipsr = ( sadb_x_ipsecrequest * ) buff;

		if( xindex >= PFKI_MAX_XFORMS )
			break;

		spinfo.xforms[ xindex ].proto	= ipsr->sadb_x_ipsecrequest_proto;
		spinfo.xforms[ xindex ].mode	= ipsr->sadb_x_ipsecrequest_mode;
		spinfo.xforms[ xindex ].level	= ipsr->sadb_x_ipsecrequest_level;
		spinfo.xforms[ xindex ].reqid	= ipsr->sadb_x_ipsecrequest_reqid;

		unsigned char * addr_buff = buff;
		long addr_size = size;

		addr_buff += sizeof( sadb_x_ipsecrequest );
		addr_size -= sizeof( sadb_x_ipsecrequest );

		if( addr_size >= 0 )
		{
			sockaddr * saddr_src = ( sockaddr * )( addr_buff );

			switch( saddr_src->sa_family )
			{
				case AF_INET:
				{
					if( addr_size < long( sizeof( sockaddr_in ) ) )
						break;

					memcpy(
						&spinfo.xforms[ xindex ].saddr_src,
						saddr_src,
						sizeof( sockaddr_in ) );

					addr_buff += sizeof( sockaddr_in );
					addr_size -= sizeof( sockaddr_in );
				}
			}

			sockaddr * saddr_dst = ( sockaddr * )( addr_buff );

			switch( saddr_dst->sa_family )
			{
				case AF_INET:
				{
					if( addr_size < long( sizeof( sockaddr_in ) ) )
						break;

					memcpy(
						&spinfo.xforms[ xindex ].saddr_dst,
						saddr_dst,
						sizeof( sockaddr_in ) );

					addr_buff += sizeof( sockaddr_in );
					addr_size -= sizeof( sockaddr_in );
				}
			}
		}

		buff += ipsr->sadb_x_ipsecrequest_len;
		size -= ipsr->sadb_x_ipsecrequest_len;

		xindex++;
	}

	return PFKI_OK;
}

long _PFKI::buff_add_ipsec( PFKI_MSG & msg, PFKI_SPINFO & spinfo )
{
	int size = sizeof( sadb_x_policy );
	int oset = msg.msg_size - size;

	long result;

	long xindex = 0;

	while( spinfo.xforms[ xindex ].proto )
	{
		if( xindex >= PFKI_MAX_XFORMS )
			break;

		long ext_size = sizeof( sadb_x_ipsecrequest );

		int salen_src = 0;
		int salen_dst = 0;

		if( spinfo.xforms[ xindex ].mode == IPSEC_MODE_TUNNEL )
		{
			if( !sockaddr_len( spinfo.xforms[ xindex ].saddr_src.sa_family, salen_src ) )
				return PFKI_FAILED;

			if( !sockaddr_len( spinfo.xforms[ xindex ].saddr_dst.sa_family, salen_dst ) )
				return PFKI_FAILED;

			if( salen_src != salen_dst )
				return PFKI_FAILED;

			ext_size += ( salen_src + salen_dst );
		}

		sadb_x_ipsecrequest * ipsr;

		result = buff_add_ext( msg, ( sadb_ext ** ) &ipsr, ext_size, false );
		if( result != PFKI_OK )
			return result;

		ipsr->sadb_x_ipsecrequest_proto	= spinfo.xforms[ xindex ].proto;
		ipsr->sadb_x_ipsecrequest_mode	= spinfo.xforms[ xindex ].mode;
		ipsr->sadb_x_ipsecrequest_level	= spinfo.xforms[ xindex ].level;
		ipsr->sadb_x_ipsecrequest_reqid	= spinfo.xforms[ xindex ].reqid;

		unsigned char * ext_buff = ( unsigned char * ) ipsr;

		ext_buff += sizeof( sadb_x_ipsecrequest );

		if( salen_src )
			memcpy( ext_buff, &spinfo.xforms[ xindex ].saddr_src, salen_src );

		ext_buff += salen_src;

		if( salen_dst )
			memcpy( ext_buff, &spinfo.xforms[ xindex ].saddr_dst, salen_dst );

		size += ext_size;

		xindex++;
	}

	//
	// reset the policy size
	//

	unsigned char * buff = msg.msg_buff + oset;
	sadb_x_policy * xpl = ( sadb_x_policy * ) buff;
	xpl->sadb_x_policy_len = ( u_int16_t ) PFKEY_UNIT64( size );

	return PFKI_OK;
}

long _PFKI::send_sainfo( u_int8_t sadb_msg_type, PFKI_SAINFO & sainfo, bool serv )
{
	PFKI_MSG msg;
	msg.append( sizeof( sadb_msg ) );

	sadb_sa * xsa;
	sadb_x_sa2 * xsa2;
	sadb_address *xas, *xad;
	sadb_lifetime *xlh, *xls, *xlc;
	sadb_key *xke, *xka;
	sadb_spirange *xsr;

	long result;

	//
	// sa extension
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_DELETE:
		case SADB_GETSPI:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GETSPI ) && !serv )
				break;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xsa, sizeof( sadb_sa ) );
			if( result != PFKI_OK )
				return result;

			xsa->sadb_sa_exttype = SADB_EXT_SA;
			xsa->sadb_sa_spi = sainfo.sa.spi;
			xsa->sadb_sa_replay = sainfo.sa.replay;
			xsa->sadb_sa_state = sainfo.sa.state;
			xsa->sadb_sa_auth = sainfo.sa.auth;
			xsa->sadb_sa_encrypt = sainfo.sa.encrypt;
			xsa->sadb_sa_flags = sainfo.sa.flags;

			break;
	}

	if( sainfo.error )
		goto sainfo_error;

	//
	// sa2 extension
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_GETSPI:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GETSPI ) && serv )
				break;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xsa2, sizeof( sadb_x_sa2 ) );
			if( result != PFKI_OK )
				return result;

			xsa2->sadb_x_sa2_exttype = SADB_X_EXT_SA2;
			xsa2->sadb_x_sa2_mode = sainfo.sa2.mode;
			xsa2->sadb_x_sa2_reqid = sainfo.sa2.reqid;
			xsa2->sadb_x_sa2_sequence = sainfo.sa2.sequence;

			break;
	}

	//
	// address extensions
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_DELETE:
		case SADB_GETSPI:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			int salen_src;
			if( !sockaddr_len( sainfo.paddr_src.saddr.sa_family, salen_src ) )
				return PFKI_FAILED;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xas, sizeof( sadb_address ) + salen_src );
			if( result != PFKI_OK )
				return result;

			xas->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			result = buff_set_address( xas, sainfo.paddr_src );
			if( result != PFKI_OK )
				return result;

			int salen_dst;
			if( !sockaddr_len( sainfo.paddr_dst.saddr.sa_family, salen_dst ) )
				return PFKI_FAILED;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xad, sizeof( sadb_address ) + salen_dst );
			if( result != PFKI_OK )
				return result;

			xad->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			result = buff_set_address( xad, sainfo.paddr_dst );
			if( result != PFKI_OK )
				return result;

			break;
	}

	//
	// soft and hard lifetime extensions
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xlh, sizeof( sadb_lifetime ) );
			if( result != PFKI_OK )
				return result;

			xlh->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
			xlh->sadb_lifetime_allocations = sainfo.ltime_hard.allocations;
			xlh->sadb_lifetime_bytes = sainfo.ltime_hard.bytes;
			xlh->sadb_lifetime_addtime = sainfo.ltime_hard.addtime;
			xlh->sadb_lifetime_usetime = sainfo.ltime_hard.usetime;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xls, sizeof( sadb_lifetime ) );
			if( result != PFKI_OK )
				return result;

			xls->sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
			xls->sadb_lifetime_allocations = sainfo.ltime_soft.allocations;
			xls->sadb_lifetime_bytes = sainfo.ltime_soft.bytes;
			xls->sadb_lifetime_addtime = sainfo.ltime_soft.addtime;
			xls->sadb_lifetime_usetime = sainfo.ltime_soft.usetime;

			break;
	}

	//
	// current lifetime extension
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xlc, sizeof( sadb_lifetime ) );
			if( result != PFKI_OK )
				return result;

			xlc->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
			xlc->sadb_lifetime_allocations = sainfo.ltime_curr.allocations;
			xlc->sadb_lifetime_bytes = sainfo.ltime_curr.bytes;
			xlc->sadb_lifetime_addtime = sainfo.ltime_curr.addtime;
			xlc->sadb_lifetime_usetime = sainfo.ltime_curr.usetime;

			break;
	}

	//
	// key data extension
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			if( sainfo.ekey.length )
			{
				result = buff_add_ext( msg, ( sadb_ext ** ) &xke, sizeof( sadb_key ) + sainfo.ekey.length );
				if( result != PFKI_OK )
					return result;

				xke->sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
				result = buff_set_key( xke, sainfo.ekey );
				if( result != PFKI_OK )
					return result;
			}

			if( sainfo.akey.length )
			{
				result = buff_add_ext( msg, ( sadb_ext ** ) &xka, sizeof( sadb_key ) + sainfo.akey.length );
				if( result != PFKI_OK )
					return result;

				xka->sadb_key_exttype = SADB_EXT_KEY_AUTH;
				result = buff_set_key( xka, sainfo.akey );
				if( result != PFKI_OK )
					return result;
			}

			break;
	}

#ifdef OPT_NATT

	//
	// natt extension
	//

	switch( sadb_msg_type )
	{
		case SADB_DUMP:
		case SADB_ADD:
		case SADB_GET:
		case SADB_UPDATE:

			if( ( sadb_msg_type == SADB_DUMP ) && !serv )
				break;

			if( ( sadb_msg_type == SADB_GET ) && !serv )
				break;

			if( sainfo.natt.type )
			{
				sadb_x_nat_t_type *xnt;
				sadb_x_nat_t_port *xnps, *xnpd;

				result = buff_add_ext( msg, ( sadb_ext ** ) &xnt, sizeof( sadb_x_nat_t_type ) );
				if( result != PFKI_OK )
					return result;

				xnt->sadb_x_nat_t_type_exttype = SADB_X_EXT_NAT_T_TYPE;
				xnt->sadb_x_nat_t_type_type = sainfo.natt.type;

				result = buff_add_ext( msg, ( sadb_ext ** ) &xnps, sizeof( sadb_x_nat_t_port ) );
				if( result != PFKI_OK )
					return result;

				xnps->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_SPORT;
				xnps->sadb_x_nat_t_port_port = sainfo.natt.port_src;

				result = buff_add_ext( msg, ( sadb_ext ** ) &xnpd, sizeof( sadb_x_nat_t_port ) );
				if( result != PFKI_OK )
					return result;

				xnpd->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_DPORT;
				xnpd->sadb_x_nat_t_port_port = sainfo.natt.port_dst;
			}

			break;
	}

#endif

	//
	// spi range extension
	//

	switch( sadb_msg_type )
	{
		case SADB_GETSPI:

			if( sainfo.range.min || sainfo.range.max )
			{
				result = buff_add_ext( msg, ( sadb_ext ** ) &xsr, sizeof( sadb_spirange ) );
				if( result != PFKI_OK )
					return result;
				
				xsr->sadb_spirange_exttype = SADB_EXT_SPIRANGE;
				xsr->sadb_spirange_min = sainfo.range.min;
				xsr->sadb_spirange_max = sainfo.range.max;
			}

			break;
	}

	sainfo_error:

	if( !serv )
		sainfo.pid = getpid();

	msg.hdr->sadb_msg_version = PF_KEY_V2;
	msg.hdr->sadb_msg_type = sadb_msg_type;
	msg.hdr->sadb_msg_errno = sainfo.error;
	msg.hdr->sadb_msg_satype = sainfo.satype;
	msg.hdr->sadb_msg_len = ( u_int16_t ) PFKEY_UNIT64( msg.msg_size );
	msg.hdr->sadb_msg_reserved = 0;
	msg.hdr->sadb_msg_seq = sainfo.seq;
	msg.hdr->sadb_msg_pid = sainfo.pid;

	return send_msg( msg );
}

long _PFKI::send_spinfo( u_int8_t sadb_msg_type, PFKI_SPINFO & spinfo, bool serv )
{
	PFKI_MSG msg;
	msg.append( sizeof( sadb_msg ) );

	sadb_x_policy * xpl;
	sadb_address *xas, *xad;

	long result;

	if( spinfo.error )
		goto spinfo_error;

	//
	// sp extension
	//

	switch( sadb_msg_type )
	{
		case SADB_ACQUIRE:
		case SADB_X_SPDDUMP:
		case SADB_X_SPDADD:
		case SADB_X_SPDDELETE2:

			if( ( sadb_msg_type == SADB_X_SPDDUMP ) && !serv )
				break;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xpl, sizeof( sadb_x_policy ) );
			if( result != PFKI_OK )
				return result;

			xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
			xpl->sadb_x_policy_type = spinfo.sp.type;
			xpl->sadb_x_policy_id = spinfo.sp.id;
			xpl->sadb_x_policy_dir = spinfo.sp.dir;

			if( spinfo.sp.type == IPSEC_POLICY_IPSEC )
			{
				result = buff_add_ipsec( msg, spinfo );
				if( result != PFKI_OK )
					return result;
			}

			break;
	}

	//
	// address extensions
	//

	switch( sadb_msg_type )
	{
		case SADB_ACQUIRE:
		case SADB_X_SPDDUMP:
		case SADB_X_SPDADD:

			if( ( sadb_msg_type == SADB_X_SPDDUMP ) && !serv )
				break;

			int salen_src;
			if( !sockaddr_len( spinfo.paddr_src.saddr.sa_family, salen_src ) )
				return PFKI_FAILED;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xas, sizeof( sadb_address ) + salen_src );
			if( result != PFKI_OK )
				return result;

			xas->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
			result = buff_set_address( xas, spinfo.paddr_src );
			if( result != PFKI_OK )
				return result;

			int salen_dst;
			if( !sockaddr_len( spinfo.paddr_dst.saddr.sa_family, salen_dst ) )
				return PFKI_FAILED;

			result = buff_add_ext( msg, ( sadb_ext ** ) &xad, sizeof( sadb_address ) + salen_dst );
			if( result != PFKI_OK )
				return result;

			xad->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
			result = buff_set_address( xad, spinfo.paddr_dst );
			if( result != PFKI_OK )
				return result;

			break;
	}

	spinfo_error:

	if( !serv )
		spinfo.pid = getpid();

	msg.hdr->sadb_msg_version = PF_KEY_V2;
	msg.hdr->sadb_msg_type = sadb_msg_type;
	msg.hdr->sadb_msg_errno = spinfo.error;
	msg.hdr->sadb_msg_satype = SADB_SATYPE_UNSPEC;
	msg.hdr->sadb_msg_len = ( u_int16_t ) PFKEY_UNIT64( msg.msg_size );
	msg.hdr->sadb_msg_reserved = 0;
	msg.hdr->sadb_msg_seq = spinfo.seq;
	msg.hdr->sadb_msg_pid = spinfo.pid;

	return send_msg( msg );
}

char * _PFKI::name( long type, long value )
{
	static char * unknown = "unknown";

	switch( type )
	{
		case NAME_MSGTYPE:
		{
			static char * msgtype_00 = "RESERVED";
			static char * msgtype_01 = "GETSPI";
			static char * msgtype_02 = "UPDATE";
			static char * msgtype_03 = "ADD";
			static char * msgtype_04 = "DELETE";
			static char * msgtype_05 = "GET";
			static char * msgtype_06 = "ACQUIRE";
			static char * msgtype_07 = "REGISTER";
			static char * msgtype_08 = "EXPIRE";
			static char * msgtype_09 = "FLUSH";
			static char * msgtype_10 = "DUMP";
			static char * msgtype_11 = "X_PROMISC";
			static char * msgtype_12 = "X_PCHANGE";
			static char * msgtype_13 = "X_SPDUPDATE";
			static char * msgtype_14 = "X_SPDADD";
			static char * msgtype_15 = "X_SPDDELETE";
			static char * msgtype_16 = "X_SPDGET";
			static char * msgtype_17 = "X_SPDACQUIRE";
			static char * msgtype_18 = "X_SPDDUMP";
			static char * msgtype_19 = "X_SPDFLUSH";
			static char * msgtype_20 = "X_SPDSETIDX";
			static char * msgtype_21 = "X_SPDEXPIRE";
			static char * msgtype_22 = "X_SPDDELETE2";
//			static char * msgtype_23 = "X_NAT_T_NEW_MAPPING";

			switch( value )
			{
				case SADB_RESERVED:
					return msgtype_00;

				case SADB_GETSPI:
					return msgtype_01;

				case SADB_UPDATE:
					return msgtype_02;

				case SADB_ADD:
					return msgtype_03;

				case SADB_DELETE:
					return msgtype_04;

				case SADB_GET:
					return msgtype_05;

				case SADB_ACQUIRE:
					return msgtype_06;

				case SADB_REGISTER:
					return msgtype_07;

				case SADB_EXPIRE:
					return msgtype_08;

				case SADB_FLUSH:
					return msgtype_09;

				case SADB_DUMP:
					return msgtype_10;

				case SADB_X_PROMISC:
					return msgtype_11;

				case SADB_X_PCHANGE:
					return msgtype_12;

				case SADB_X_SPDUPDATE:
					return msgtype_13;

				case SADB_X_SPDADD:
					return msgtype_14;

				case SADB_X_SPDDELETE:
					return msgtype_15;

				case SADB_X_SPDGET:
					return msgtype_16;

				case SADB_X_SPDACQUIRE:
					return msgtype_17;

				case SADB_X_SPDDUMP:
					return msgtype_18;

				case SADB_X_SPDFLUSH:
					return msgtype_19;

				case SADB_X_SPDSETIDX:
					return msgtype_20;

				case SADB_X_SPDEXPIRE:
					return msgtype_21;

				case SADB_X_SPDDELETE2:
					return msgtype_22;

//				case SADB_X_NAT_T_NEW_MAPPING:
//					return msgtype_23;

				default:
					return unknown;

			}
		}

		case NAME_SAENCR:
		{
			static char * encrtype_02 = "DES-CBC";
			static char * encrtype_03 = "3DES-CBC";
			static char * encrtype_06 = "CAST128-CBC";
			static char * encrtype_07 = "BLOWFISH-CBC";
			static char * encrtype_12 = "AES-CBC";

			switch( value )
			{
				case SADB_EALG_DESCBC:
					return encrtype_02;

				case SADB_EALG_3DESCBC:
					return encrtype_03;

//				case SADB_X_EALG_CAST128CBC:
//					return encrtype_06;

				case SADB_X_EALG_BLOWFISHCBC:
					return encrtype_07;

				case SADB_X_EALG_AESCBC:
					return encrtype_12;

				default:
					return unknown;
			}
		}

		case NAME_SACOMP:
		{
			static char * comptype_01 = "OUI";
			static char * comptype_02 = "DEFLATE";
			static char * comptype_03 = "LZS";

			switch( value )
			{
				case SADB_X_CALG_OUI:
					return comptype_01;

				case SADB_X_CALG_DEFLATE:
					return comptype_02;

				case SADB_X_CALG_LZS:
					return comptype_03;

				default:
					return unknown;
			}
		}

		case NAME_SAAUTH:
		{
			static char * authtype_02 = "HMAC-MD5";
			static char * authtype_03 = "HMAC-SHA1";

			switch( value )
			{
				case SADB_AALG_MD5HMAC:
					return authtype_02;

				case SADB_AALG_SHA1HMAC:
					return authtype_03;

				default:
					return unknown;
			}
		}


		case NAME_SATYPE:
		{
			static char * satype_00 = "UNSPEC";
			static char * satype_02 = "AH";
			static char * satype_03 = "ESP";
			static char * satype_05 = "RSVP";
			static char * satype_06 = "OSPFV2";
			static char * satype_07 = "RIPV2";
			static char * satype_08 = "MIP";
			static char * satype_09 = "IPCOMP";
			static char * satype_11 = "TCPSIGNATURE";

			switch( value )
			{
				case SADB_SATYPE_UNSPEC:
					return satype_00;

				case SADB_SATYPE_AH:
					return satype_02;

				case SADB_SATYPE_ESP:
					return satype_03;

				case SADB_SATYPE_RSVP:
					return satype_05;

				case SADB_SATYPE_OSPFV2:
					return satype_06;

				case SADB_SATYPE_RIPV2:
					return satype_07;

				case SADB_SATYPE_MIP:
					return satype_08;

				case SADB_X_SATYPE_IPCOMP:
					return satype_09;

//				case SADB_X_SATYPE_TCPSIGNATURE:
//					return satype_11;

				default:
					return unknown;
			}
		}

		case NAME_SPMODE:
		{
			static char * plcymode_00 = "ANY";
			static char * plcymode_01 = "TANSPORT";
			static char * plcymode_02 = "TUNNEL";

			switch( value )
			{
				case IPSEC_MODE_ANY:
					return plcymode_00;

				case IPSEC_MODE_TRANSPORT:
					return plcymode_01;

				case IPSEC_MODE_TUNNEL:
					return plcymode_02;

				default:
					return unknown;
			}
		}

		case NAME_SPTYPE:
		{
			static char * plcytype_00 = "DISCARD";
			static char * plcytype_01 = "NONE";
			static char * plcytype_02 = "IPSEC";
			static char * plcytype_03 = "ENTRUST";
			static char * plcytype_04 = "BYPASS";

			switch( value )
			{
				case IPSEC_POLICY_DISCARD:
					return plcytype_00;

				case IPSEC_POLICY_NONE:
					return plcytype_01;

				case IPSEC_POLICY_IPSEC:
					return plcytype_02;

				case IPSEC_POLICY_ENTRUST:
					return plcytype_03;

				case IPSEC_POLICY_BYPASS:
					return plcytype_04;

				default:
					return unknown;
			}
		}

		case NAME_SPDIR:
		{
			static char * plcydir_00 = "ANY";
			static char * plcydir_01 = "INBOUND";
			static char * plcydir_02 = "OUTBOUND";
			static char * plcydir_03 = "MAX";
			static char * plcydir_04 = "INVALID";

			switch( value )
			{
				case IPSEC_DIR_ANY:
					return plcydir_00;

				case IPSEC_DIR_INBOUND:
					return plcydir_01;

				case IPSEC_DIR_OUTBOUND:
					return plcydir_02;

				case IPSEC_DIR_MAX:
					return plcydir_03;

				case IPSEC_DIR_INVALID:
					return plcydir_04;

				default:
					return unknown;
			}
		}

		case NAME_SPLEVEL:
		{
			static char * plcylevel_00 = "ANY";
			static char * plcylevel_01 = "USE";
			static char * plcylevel_02 = "REQUIRE";
			static char * plcylevel_03 = "UNIQUE";

			switch( value )
			{
				case IPSEC_LEVEL_DEFAULT:
					return plcylevel_00;

				case IPSEC_LEVEL_USE:
					return plcylevel_01;

				case IPSEC_LEVEL_REQUIRE:
					return plcylevel_02;

				case IPSEC_LEVEL_UNIQUE:
					return plcylevel_03;

				default:
					return unknown;
			}
		}
	}

	return unknown;
}

long _PFKI::next_msg( PFKI_MSG & msg )
{
	long result = wait_msg();
	if( result != PFKI_OK )
		return result;

	msg.reset();

	if( !msg.append( sizeof( sadb_msg ) ) )
		return PFKI_FAILED;

	result = recv_msg( msg, true );
	if( result != PFKI_OK )
		return result;

	if( !msg.append( PFKEY_UNUNIT64( msg.hdr->sadb_msg_len ) - msg.msg_size ) )
		return PFKI_FAILED;

	result = recv_msg( msg );
	if( result != PFKI_OK )
		return result;

	return PFKI_OK;
}

//
// extension functions
//

long _PFKI::read_sa( PFKI_MSG & msg, PFKI_SA & sa )
{
	//
	// read policy extension
	//

	sadb_sa * xsa;

	long result = buff_get_ext( msg, ( sadb_ext ** ) &xsa, SADB_EXT_SA );
	if( result != PFKI_OK )
		return result;

	sa.spi		= xsa->sadb_sa_spi;
	sa.replay	= xsa->sadb_sa_replay;
	sa.state	= xsa->sadb_sa_state;
	sa.auth		= xsa->sadb_sa_auth;
	sa.encrypt	= xsa->sadb_sa_encrypt;
	sa.flags	= xsa->sadb_sa_flags;

	return PFKI_OK;
}

long _PFKI::read_sa2( PFKI_MSG & msg, PFKI_SA2 & sa2 )
{
	//
	// read policy extension
	//

	sadb_x_sa2 * xsa2;

	long result = buff_get_ext( msg, ( sadb_ext ** ) &xsa2, SADB_X_EXT_SA2 );
	if( result != PFKI_OK )
		return result;

	sa2.mode		= xsa2->sadb_x_sa2_mode;
	sa2.sequence	= xsa2->sadb_x_sa2_sequence;
	sa2.reqid		= xsa2->sadb_x_sa2_reqid;

	return true;
}

long _PFKI::read_range( PFKI_MSG & msg, PFKI_RANGE & range )
{
	//
	// read address extension
	//

	sadb_spirange * xsr;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xsr, SADB_EXT_SPIRANGE );
	if( result != PFKI_OK )
		return result;

	range.min = xsr->sadb_spirange_min;
	range.max = xsr->sadb_spirange_max;

	return PFKI_OK;
}

long _PFKI::read_ltime_curr( PFKI_MSG & msg, PFKI_LTIME & ltime )
{
	//
	// read lifetime extension
	//

	sadb_lifetime * xlt;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xlt, SADB_EXT_LIFETIME_CURRENT );
	if( result != PFKI_OK )
		return result;

	ltime.allocations = xlt->sadb_lifetime_allocations;
	ltime.addtime = xlt->sadb_lifetime_addtime;
	ltime.usetime = xlt->sadb_lifetime_usetime;
	ltime.bytes = xlt->sadb_lifetime_bytes;

	return PFKI_OK;
}

long _PFKI::read_ltime_hard( PFKI_MSG & msg, PFKI_LTIME & ltime )
{
	//
	// read lifetime extension
	//

	sadb_lifetime * xlt;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xlt, SADB_EXT_LIFETIME_HARD );
	if( result != PFKI_OK )
		return result;

	ltime.allocations = xlt->sadb_lifetime_allocations;
	ltime.addtime = xlt->sadb_lifetime_addtime;
	ltime.usetime = xlt->sadb_lifetime_usetime;
	ltime.bytes = xlt->sadb_lifetime_bytes;

	return PFKI_OK;
}

long _PFKI::read_ltime_soft( PFKI_MSG & msg, PFKI_LTIME & ltime )
{
	//
	// read lifetime extension
	//

	sadb_lifetime * xlt;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xlt, SADB_EXT_LIFETIME_SOFT );
	if( result != PFKI_OK )
		return result;

	ltime.allocations = xlt->sadb_lifetime_allocations;
	ltime.addtime = xlt->sadb_lifetime_addtime;
	ltime.usetime = xlt->sadb_lifetime_usetime;
	ltime.bytes = xlt->sadb_lifetime_bytes;

	return PFKI_OK;
}

long _PFKI::read_key_a( PFKI_MSG & msg, PFKI_KEY & akey )
{
	//
	// read key extension
	//

	sadb_key * xka;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xka, SADB_EXT_KEY_AUTH );
	if( result != PFKI_OK )
		return result;

	result = buff_get_key( xka, akey );
	if( result != PFKI_OK )
		return result;

	return PFKI_OK;
}

long _PFKI::read_key_e( PFKI_MSG & msg, PFKI_KEY & ekey )
{
	//
	// read key extension
	//

	sadb_key * xke;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xke, SADB_EXT_KEY_ENCRYPT );
	if( result != PFKI_OK )
		return result;

	result = buff_get_key( xke, ekey );
	if( result != PFKI_OK )
		return result;

	return PFKI_OK;
}

long _PFKI::read_address_src( PFKI_MSG & msg, PFKI_ADDR & addr )
{
	//
	// read address extension
	//

	sadb_address * xad;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xad, SADB_EXT_ADDRESS_SRC );
	if( result != PFKI_OK )
		return result;

	result = buff_get_address( xad, addr );
	if( result != PFKI_OK )
		return result;

	return PFKI_OK;
}

long _PFKI::read_address_dst( PFKI_MSG & msg, PFKI_ADDR & addr )
{
	//
	// read address extension
	//

	sadb_address * xad;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xad, SADB_EXT_ADDRESS_DST );
	if( result != PFKI_OK )
		return result;

	result = buff_get_address( xad, addr );
	if( result != PFKI_OK )
		return result;

	return PFKI_OK;
}

long _PFKI::read_natt( PFKI_MSG & msg, PFKI_NATT & natt )
{

#ifdef OPT_NATT

	sadb_x_nat_t_type * xnt;
	sadb_x_nat_t_port * xnps, * xnpd;

	long result;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xnt, SADB_X_EXT_NAT_T_TYPE );
	if( result != PFKI_OK )
		return result;

	natt.type = xnt->sadb_x_nat_t_type_type;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xnps, SADB_X_EXT_NAT_T_SPORT );
	if( result != PFKI_OK )
		return result;

	natt.port_src = xnps->sadb_x_nat_t_port_port;

	result = buff_get_ext( msg, ( sadb_ext ** ) &xnpd, SADB_X_EXT_NAT_T_DPORT );
	if( result != PFKI_OK )
		return result;

	natt.port_dst = xnpd->sadb_x_nat_t_port_port;

	return PFKI_OK;

#else

	return PFKI_FAILED;

#endif

}

long _PFKI::read_policy( PFKI_MSG & msg, PFKI_SPINFO & spinfo )
{
	//
	// read policy extension
	//

	sadb_x_policy * xpl;

	long result;
	
	result = buff_get_ext( msg, ( sadb_ext ** ) &xpl, SADB_X_EXT_POLICY );
	if( result != PFKI_OK )
		return result;

	spinfo.sp.type	= xpl->sadb_x_policy_type;
	spinfo.sp.id	= xpl->sadb_x_policy_id;
	spinfo.sp.dir	= xpl->sadb_x_policy_dir;

	if( spinfo.sp.type == IPSEC_POLICY_IPSEC )
	{
		result = buff_get_ipsec( xpl, spinfo );
		if( result != PFKI_OK )
			return result;
	}

	return PFKI_OK;
}

//
// client functions
//

long _PFKI::send_register( u_int8_t satype )
{
	PFKI_SAINFO sainfo;
	memset( &sainfo, 0, sizeof( sainfo ) );
	sainfo.satype = satype;
	return send_sainfo( SADB_REGISTER, sainfo, false );
}

long _PFKI::send_dump()
{
	PFKI_SAINFO sainfo;
	memset( &sainfo, 0, sizeof( sainfo ) );
	return send_sainfo( SADB_DUMP, sainfo, false );
}

long _PFKI::send_add( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_ADD, sainfo, false );
}

long _PFKI::send_get( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_GET, sainfo, false );
}

long _PFKI::send_del( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_DELETE, sainfo, false );
}

long _PFKI::send_getspi( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_GETSPI, sainfo, false );
}

long _PFKI::send_update( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_UPDATE, sainfo, false );
}

long _PFKI::send_spdump()
{
	PFKI_SPINFO spinfo;
	memset( &spinfo, 0, sizeof( spinfo ) );
	return send_spinfo( SADB_X_SPDDUMP, spinfo, false );
}

long _PFKI::send_spadd( PFKI_SPINFO & spinfo )
{
	return send_spinfo( SADB_X_SPDADD, spinfo, false );
}

long _PFKI::send_spdel( PFKI_SPINFO & spinfo )
{
	return send_spinfo( SADB_X_SPDDELETE2, spinfo, false );
}

//
// server functions
//

long _PFKI::serv_dump( PFKI_SAINFO & sainfo )
{
 	return send_sainfo( SADB_DUMP, sainfo, true );
}

long _PFKI::serv_add( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_ADD, sainfo, true );
}

long _PFKI::serv_get( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_GET, sainfo, true );
}

long _PFKI::serv_del( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_DELETE, sainfo, true );
}

long _PFKI::serv_acquire( PFKI_SPINFO & spinfo )
{
	return send_spinfo( SADB_ACQUIRE, spinfo, true );
}

long _PFKI::serv_getspi( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_GETSPI, sainfo, true );
}

long _PFKI::serv_update( PFKI_SAINFO & sainfo )
{
	return send_sainfo( SADB_UPDATE, sainfo, true );
}

long _PFKI::serv_spdump( PFKI_SPINFO & spinfo )
{
 	return send_spinfo( SADB_X_SPDDUMP, spinfo, true );
}

long _PFKI::serv_spadd( PFKI_SPINFO & spinfo )
{
	return send_spinfo( SADB_X_SPDADD, spinfo, true );
}

long _PFKI::serv_spdel( PFKI_SPINFO & spinfo )
{
	return send_spinfo( SADB_X_SPDDELETE2, spinfo, true );
}

#ifdef WIN32

_PFKS::_PFKS()
{
	hsrvc = NULL;
	hpipe = INVALID_HANDLE_VALUE;
}

_PFKS::~_PFKS()
{
}

bool _PFKS::init()
{
	hsrvc = OpenEvent(
				EVENT_ALL_ACCESS,
				false,
				PFKI_EVENT_NAME );

	if( hsrvc )
	{
		CloseHandle( hsrvc );
		return false;
	}

	hsrvc =	CreateEvent(
				NULL,
				false,
				false,
				PFKI_EVENT_NAME );

	if( hsrvc == NULL )
		return false;

	return true;
}

PFKI * _PFKS::accept()
{
	if( hpipe == INVALID_HANDLE_VALUE )
	{
		hpipe = CreateNamedPipe(
				PFKI_PIPE_NAME,
				PIPE_ACCESS_DUPLEX |
				FILE_FLAG_OVERLAPPED,
			    PIPE_TYPE_MESSAGE |
				PIPE_READMODE_MESSAGE |
				PIPE_NOWAIT,
				PIPE_UNLIMITED_INSTANCES,
				8192,
				8192,
			    10,
				NULL );
	}

	if( ( ConnectNamedPipe( hpipe, NULL ) == TRUE ) ||
		( GetLastError() == ERROR_PIPE_CONNECTED ) )
	{
		PFKI * pfki = new PFKI;
		if( pfki == NULL )
			return NULL;

		pfki->hpipe = hpipe;
		hpipe = INVALID_HANDLE_VALUE;

		return pfki;
	}

	return NULL;
}

#endif
