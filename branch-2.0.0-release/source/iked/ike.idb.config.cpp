
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

//
// transaction configuration class
//

_IDB_CFG::_IDB_CFG( IDB_TUNNEL * set_tunnel, bool set_initiator, unsigned long set_msgid )
{
	refcount = 0;

	msgid = 0;
	mtype = 0;
	ident = 0;

	tunnel = set_tunnel;
	initiator = set_initiator;

	if( set_msgid )
		msgid = set_msgid;
	else
	{
		iked.rand_bytes( &msgid, sizeof( msgid ) );
		iked.rand_bytes( &ident, sizeof( ident ) );
	}
}

_IDB_CFG::~_IDB_CFG()
{
	clean();
}

bool _IDB_CFG::attr_add_b( unsigned short atype, unsigned short bdata )
{
	IKE_ATTR * attr = new IKE_ATTR;
	if( attr == NULL )
		return false;

	attr->atype = atype;
	attr->basic = true;
	attr->bdata = bdata;

	return attrs.add_item( attr );
}

bool _IDB_CFG::attr_add_v( unsigned short atype, void * vdata, unsigned long vsize )
{
	IKE_ATTR * attr = new IKE_ATTR;
	if( attr == NULL )
		return false;

	attr->atype = atype;
	attr->basic = false;
	attr->vdata.set( ( char * ) vdata, vsize );

	return attrs.add_item( attr );
}

bool _IDB_CFG::attr_has( unsigned short atype )
{
	long index = 0;
	long count = attr_count();

	for( ; index < count; index++ )
	{
		IKE_ATTR * attr = attr_get( index );

		if( attr->atype == atype )
			return true;
	}
	return false;
}

IKE_ATTR * _IDB_CFG::attr_get( long index )
{
	return ( IKE_ATTR * ) attrs.get_item( index );
}

long _IDB_CFG::attr_count()
{
	return attrs.get_count();
}

void _IDB_CFG::attr_reset()
{
	while( attrs.get_count() )
	{
		IKE_ATTR * attr = attr_get( 0 );
		attrs.del_item( attr );
		delete attr;
	}
}

bool _IDB_CFG::setup()
{
	return true;
}

void _IDB_CFG::clean()
{
	attr_reset();
}

bool _IKED::get_config( bool lock, IDB_CFG ** cfg, IDB_TUNNEL * tunnel, unsigned long msgid )
{
	if( cfg != NULL )
		*cfg = NULL;

	if( lock )
		lock_sdb.lock();

	//
	// step through our list of configs
	// and see if they match the msgid
	//

	long count = list_config.get_count();
	long index = 0;

	for( ; index < count; index++ )
	{
		//
		// get the next config in our list
		//

		IDB_CFG * tmp_cfg = ( IDB_CFG * ) list_config.get_item( index );

		//
		// match the tunnel id
		//

		if( tmp_cfg->tunnel != tunnel )
			continue;

		//
		// match the msgid
		//

		if( tmp_cfg->msgid != msgid )
			continue;

		log.txt( LOG_DEBUG, "DB : config found\n" );

		//
		// increase our refrence count
		//

		if( cfg != NULL )
		{
			tmp_cfg->inc( false );
			*cfg = tmp_cfg;
		}

		if( lock )
			lock_sdb.unlock();

		return true;
	}

	log.txt( LOG_DEBUG, "DB : config not found\n" );

	if( lock )
		lock_sdb.unlock();

	return false;
}

bool _IDB_CFG::add( bool lock )
{
	if( lock )
		iked.lock_sdb.lock();

	inc( false );
	tunnel->inc( false );

	bool result = iked.list_config.add_item( this );

	iked.log.txt( LOG_DEBUG, "DB : config added\n" );

	if( lock )
		iked.lock_sdb.unlock();
	
	return result;

}

bool _IDB_CFG::inc( bool lock )
{
	if( lock )
		iked.lock_sdb.lock();

	refcount++;

	iked.log.txt( LOG_LOUD,
		"DB : config ref increment ( ref count = %i, config count = %i )\n",
		refcount,
		iked.list_config.get_count() );

	if( lock )
		iked.lock_sdb.unlock();

	return true;
}

bool _IDB_CFG::dec( bool lock )
{
	if( lock )
		iked.lock_sdb.lock();

	//
	// if we are marked for deletion,
	// attempt to remove any events
	// that may be scheduled
	//

	if( lstate & LSTATE_DELETE )
	{
		if( iked.ith_timer.del( &event_resend ) )
		{
			refcount--;
			iked.log.txt( LOG_DEBUG,
				"DB : config resend event canceled ( ref count = %i )\n",
				refcount );
		}
	}

	assert( refcount > 0 );

	refcount--;

	if( refcount || !( lstate & LSTATE_DELETE ) )
	{
		iked.log.txt( LOG_LOUD,
			"DB : config ref decrement ( ref count = %i, config count = %i )\n",
			refcount,
			iked.list_config.get_count() );

		if( lock )
			iked.lock_sdb.unlock();

		return false;
	}

	//
	// remove from our list
	//

	iked.list_config.del_item( this );

	//
	// log deletion
	//

	iked.log.txt( LOG_DEBUG,
		"DB : config deleted ( config count %i )\n",
		iked.list_config.get_count() );

	//
	// dereference our tunnel
	//

	tunnel->dec( false );

	if( lock )
		iked.lock_sdb.unlock();

	//
	// free
	//

	delete this;

	return true;
}

