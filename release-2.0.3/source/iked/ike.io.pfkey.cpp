
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
// ike pfkey io thread
//

long ITH_PFKEY::func( void * arg )
{
	IKED * iked = ( IKED * ) arg;
	return iked->loop_ike_pfkey();
}

long _IKED::loop_ike_pfkey()
{
	//
	// begin pfkey thread
	//

	log.txt( LOG_INFO, "ii : pfkey process thread begin ...\n" );

	refcount++;

	while( state == DSTATE_ACTIVE )
	{
		//
		// read the next pfkey message
		//

		PFKI_MSG msg;

		long result = pfki.next_msg( msg );

		if( result == PFKI_NODATA )
			continue;

		if( result != PFKI_OK )
		{
			if( pfki.open() != PFKI_OK )
			{
				Sleep( 100 );
				continue;
			}

			//
			// register for certain protocol types
			//

			if( ( pfki.send_register( SADB_SATYPE_AH ) != PFKI_OK ) ||
				( pfki.send_register( SADB_SATYPE_ESP ) != PFKI_OK ) ||
				( pfki.send_register( SADB_X_SATYPE_IPCOMP ) != PFKI_OK ) )
			{
				log.txt( LOG_ERROR, "!! : unable to send pfkey register message\n" );
				return LIBIKE_FAILED;
			}

			//
			// initiate an SPD dump
			//

			if( pfki.send_spdump() != PFKI_OK )
			{
				log.txt( LOG_ERROR, "!! : unable to send pfkey spd dump message\n" );
				return LIBIKE_FAILED;
			}

			continue;
		}

		if( msg.hdr->sadb_msg_errno )
		{
			log.txt( LOG_ERROR,
				"K! : recv %s message failure ( errno = %i )\n",
				pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
				msg.hdr->sadb_msg_errno );

			continue;
		}

		//
		// process the message by type
		//

		switch( msg.hdr->sadb_msg_type )
		{
			case SADB_REGISTER:

				if( msg.local() )
					log.txt( LOG_DEBUG,
						"K< : recv %s %s pfkey message\n",
						pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
						pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );
				else
					log.txt( LOG_DEBUG,
						"K< : recv %s %s pfkey message ( ignored )\n",
						pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
						pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				break;

			case SADB_FLUSH:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_flush( msg );

				break;

			case SADB_ACQUIRE:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_acquire( msg );

				break;

			case SADB_GETSPI:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_getspi( msg );

				break;

			case SADB_UPDATE:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				break;

			case SADB_DELETE:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				break;

			case SADB_X_SPDFLUSH:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_spflush( msg );

				break;

			case SADB_X_SPDDUMP:
			case SADB_X_SPDGET:
			case SADB_X_SPDADD:

				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_spinfo( msg );

				break;

			case SADB_X_SPDDELETE2:
				
				log.txt( LOG_DEBUG,
					"K< : recv %s %s pfkey message\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					pfki.name( NAME_SATYPE, msg.hdr->sadb_msg_satype ) );

				pfkey_recv_spdel( msg );

				break;


			default:

				log.txt( LOG_ERROR,
					"K! : unhandled pfkey message type %s ( %i )\n",
					pfki.name( NAME_MSGTYPE, msg.hdr->sadb_msg_type ),
					msg.hdr->sadb_msg_type );

				break;
		}
	}

	pfki.close();

	refcount--;

	log.txt( LOG_INFO, "ii : pfkey process thread exit ...\n" );

	return LIBIKE_OK;
}

bool _IKED::paddr_ph2id( PFKI_ADDR & paddr, IKE_PH2ID & ph2id )
{
	switch( paddr.saddr.sa_family )
	{
		case AF_INET:
		{
			if( paddr.proto != IPSEC_PROTO_ANY )
				ph2id.prot = paddr.proto;
			else
				ph2id.prot = 0;

			ph2id.prot = paddr.proto;
			ph2id.port = paddr.saddr4.sin_port;

			ph2id.type = ISAKMP_ID_IPV4_ADDR;
			ph2id.addr1 = paddr.saddr4.sin_addr;
			ph2id.addr2.s_addr = 0;

			if( paddr.prefix < 32 )
			{
				ph2id.type = ISAKMP_ID_IPV4_ADDR_SUBNET;

				for( long i = 0; i < paddr.prefix; i++ )
				{
					ph2id.addr2.s_addr >>= 1;
					ph2id.addr2.s_addr |= 0x80000000;
				}

				ph2id.addr2.s_addr = htonl( ph2id.addr2.s_addr );
			}

			return true;
		}

		default:
			log.txt( LOG_ERROR,
				"!! : ph2id -> pfkiaddr, unhandled address faimily %i\n",
				paddr.saddr.sa_family );
	}

	return false;
}

bool _IKED::ph2id_paddr( IKE_PH2ID & ph2id, PFKI_ADDR & paddr )
{
	switch( ph2id.type )
	{
		case ISAKMP_ID_IPV4_ADDR:
		case ISAKMP_ID_IPV4_ADDR_SUBNET:
		{
			paddr.saddr4.sin_family = AF_INET;
			SET_SALEN( &paddr.saddr4, sizeof( sockaddr_in  ) );
			paddr.saddr4.sin_addr = ph2id.addr1;
			paddr.saddr4.sin_port = ph2id.port;

			if( ph2id.prot )
				paddr.proto = ph2id.prot;
			else
				paddr.proto = IPSEC_PROTO_ANY;

			if( ph2id.type == ISAKMP_ID_IPV4_ADDR )
				paddr.prefix = 32;
			else
			{
				unsigned long mask = ntohl( ph2id.addr2.s_addr );

				while( mask & 0x80000000 )
				{
					mask <<= 1;
					paddr.prefix++;
				}
			}

			return true;
		}

		default:
			log.txt( LOG_ERROR,
				"!! : ph2id -> pfkiaddr, unhandled id type %i\n",
				ph2id.type );
	}

	return false;
}

long _IKED::pfkey_recv_spinfo( PFKI_MSG & msg )
{
	PFKI_SPINFO spinfo;
	memset( &spinfo, 0, sizeof( spinfo ) );

	if( pfki.read_policy( msg, spinfo ) != PFKI_OK )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read basic policy info\n" );

		return LIBIKE_FAILED;
	}

	if( ( pfki.read_address_src( msg, spinfo.paddr_src ) != PFKI_OK ) ||
		( pfki.read_address_dst( msg, spinfo.paddr_dst ) != PFKI_OK ) )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read policy address info\n" );

		return LIBIKE_FAILED;
	}

	char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
	char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

	text_addr( txtid_src, &spinfo.paddr_src, true, true );
	text_addr( txtid_dst, &spinfo.paddr_dst, true, true );

	log.txt( LOG_DECODE,
		"ii : - id   = %i\n"
		"ii : - type = %s\n"
		"ii : - dir  = %s\n"
		"ii : - src  = %s\n"
		"ii : - dst  = %s\n",
		spinfo.sp.id,
		pfki.name( NAME_SPTYPE, spinfo.sp.type ),
		pfki.name( NAME_SPDIR, spinfo.sp.dir ),
		txtid_src,
		txtid_dst );

	if( spinfo.sp.type == IPSEC_POLICY_IPSEC )
	{
		for( long xindex = 0; xindex < PFKI_MAX_XFORMS; xindex++ )
		{
			if( !spinfo.xforms[ xindex ].proto )
			{
				if( xindex )
					break;

				log.txt( LOG_ERROR, "!! : failed to add policy, no transforms defind\n" );
				return LIBIKE_FAILED;
			}

			char txtaddr_src[ LIBIKE_MAX_TEXTADDR ];
			char txtaddr_dst[ LIBIKE_MAX_TEXTADDR ];

			text_addr( txtaddr_src, &spinfo.xforms[ xindex ].saddr_src, false );
			text_addr( txtaddr_dst, &spinfo.xforms[ xindex ].saddr_dst, false );

			log.txt( LOG_DECODE,
				"ii : - transform #%i\n"
				"ii : -- proto = %i\n"
				"ii : -- level = %s\n"
				"ii : -- mode  = %s\n"
				"ii : -- reqid = %i\n"
				"ii : -- tsrc  = %s\n"
				"ii : -- tdst  = %s\n",
				xindex,
				spinfo.xforms[ xindex ].proto,
				pfki.name( NAME_SPLEVEL, spinfo.xforms[ xindex ].level ),
				pfki.name( NAME_SPMODE, spinfo.xforms[ xindex ].mode ),
				spinfo.xforms[ xindex ].reqid,
				txtaddr_src,
				txtaddr_dst );
		}
	}

	//
	// create a local policy entry
	//

	IDB_POLICY * policy = new IDB_POLICY( &spinfo );
	if( policy == NULL )
		return LIBIKE_FAILED;

	//
	// add the policy and cleanup
	//

	policy->add( true );
	policy->dec( true );

	return LIBIKE_OK;
}

long _IKED::pfkey_recv_acquire( PFKI_MSG & msg )
{
	PFKI_SPINFO spinfo;
	memset( &spinfo, 0, sizeof( spinfo ) );

	if( pfki.read_policy( msg, spinfo ) != PFKI_OK )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read basic policy info\n" );

		return LIBIKE_FAILED;
	}

	if( spinfo.sp.type != IPSEC_POLICY_IPSEC )
	{
		log.txt( LOG_DECODE,
			"ii : - id   = %i\n"
			"ii : - type = %s\n"
			"ii : - dir  = %s\n",
			spinfo.sp.id,
			pfki.name( NAME_SPTYPE, spinfo.sp.type ),
			pfki.name( NAME_SPDIR, spinfo.sp.dir ) );
		
		return LIBIKE_OK;
	}

	if( ( pfki.read_address_src( msg, spinfo.paddr_src ) != PFKI_OK ) ||
		( pfki.read_address_dst( msg, spinfo.paddr_dst ) != PFKI_OK ) )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read policy address info\n" );

		return LIBIKE_FAILED;
	}

	char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
	char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

	text_addr( txtid_src, &spinfo.paddr_src, true, true );
	text_addr( txtid_dst, &spinfo.paddr_dst, true, true );

	log.txt( LOG_DECODE,
		"ii : - id   = %i\n"
		"ii : - type = %s\n"
		"ii : - dir  = %s\n"
		"ii : - src  = %s\n"
		"ii : - dst  = %s\n",
		spinfo.sp.id,
		pfki.name( NAME_SPTYPE, spinfo.sp.type ),
		pfki.name( NAME_SPDIR, spinfo.sp.dir ),
		txtid_src,
		txtid_dst );

	//
	// locate oubound policy by id
	//

	IDB_POLICY * policy_out;

	if( !get_policy(
		true,
		&policy_out,
		IPSEC_DIR_OUTBOUND,
		spinfo.sp.type,
		&spinfo.sp.id,
		NULL,
		NULL,
		NULL,
		NULL ) )
	{
		log.txt( LOG_ERROR, "!! : unable to locate outbound policy for acquire\n" );

		return LIBIKE_FAILED;
	}

	//
	// locate inbound policy by the
	// source and destination addrs
	// and ids
	//

	IKE_SADDR src, dst;
	policy_get_addrs( policy_out, src, dst );

	IKE_PH2ID ids, idd;
	paddr_ph2id( policy_out->paddr_src, ids );
	paddr_ph2id( policy_out->paddr_dst, idd );

	IDB_POLICY * policy_in;

	if( !get_policy(
		true,
		&policy_in,
		IPSEC_DIR_INBOUND,
		spinfo.sp.type,
		NULL,
		&dst,
		&src,
		&idd,
		&ids ) )
	{
		log.txt( LOG_ERROR, "!! : unable to locate inbound policy for acquire\n" );

		policy_out->dec( true );

		return LIBIKE_FAILED;
	}

	//
	// attempt to locate an existing
	// tunnel for the policy
	//

	IDB_TUNNEL * tunnel;
	if( !get_tunnel( true, &tunnel, NULL, &dst, false ) )
	{
		//
		// attempt to locate an existing
		// peer config for the policy
		//
		
		IDB_PEER * peer;
		if( !get_peer( true, &peer, &dst ) )
		{
			log.txt( LOG_ERROR, "!! : unable to locate peer config for policy\n" );

			policy_in->dec( true );
			policy_out->dec( true );

			return LIBIKE_FAILED;
		}

		//
		// attempt to locate the socket
		// for our address and the ike
		// port value
		//

		if( socket_locate( src ) != LIBIKE_OK )
		{
			char txtaddr[ LIBIKE_MAX_TEXTADDR ];
			text_addr( txtaddr, &src, false );

			log.txt( LOG_ERROR,
				"!! : unable to create tunnel, no socket for address %s\n",
				txtaddr );

			policy_in->dec( true );
			policy_out->dec( true );

			return LIBIKE_FAILED;
		}

		//
		// attempt to create and add a new
		// tunnel for the peer
		//

		tunnel = new IDB_TUNNEL( peer, &src, &peer->saddr );

		if( tunnel == NULL )
		{
			log.txt( LOG_ERROR, "!! : unable to create new tunnel object\n" );

			peer->dec( true );
			policy_in->dec( true );
			policy_out->dec( true );

			return LIBIKE_FAILED;
		}

		if( !tunnel->add( true ) )
		{
			log.txt( LOG_ERROR, "!! : unable to add tunnel object\n" );

			delete tunnel;

			peer->dec( true );
			policy_in->dec( true );
			policy_out->dec( true );

			return LIBIKE_FAILED;
		}

		peer->dec( true );
	}

	//
	// create a new phase2 handler
	// for the security association
	//

	IDB_PH2 * ph2 = new IDB_PH2( tunnel, true, 0, msg.hdr->sadb_msg_seq );
	if( ph2 == NULL )
	{
		tunnel->dec( true );
		policy_in->dec( true );
		policy_out->dec( true );

		return LIBIKE_FAILED;
	}

	if( !ph2->add( true ) != LIBIKE_OK )
	{
		delete ph2;

		tunnel->dec( true );
		policy_in->dec( true );
		policy_out->dec( true );

		return LIBIKE_FAILED;
	}

	//
	// configure the phase2 network ids
	//

	ph2->ph2id_ls = ids;
	ph2->ph2id_ld = idd;

	//
	// configure the phase2 proposal list
	//

	phase2_gen_prop( ph2, policy_out );

	//
	// configure the phase2 dh group
	//

	ph2->setup_dhgrp();

	//
	// acquire any needed pfkey spis
	//

	pfkey_send_getspi( policy_in, ph2 );

	//
	// cleanup
	//

	ph2->dec( true );
	tunnel->dec( true );
	policy_in->dec( true );
	policy_out->dec( true );

	return LIBIKE_OK;
}

long _IKED::pfkey_recv_getspi( PFKI_MSG & msg )
{
	if( !msg.local() )
	{
		log.txt( LOG_DECODE,
			"ii : - message ignored ( not local )\n" );

		return LIBIKE_OK;
	}

	PFKI_SA sa;
	memset( &sa, 0, sizeof( sa ) );

	if( pfki.read_sa( msg, sa ) != PFKI_OK )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read security association info\n" );

		return LIBIKE_FAILED;
	}

	PFKI_ADDR paddr_src;
	PFKI_ADDR paddr_dst;

	if( ( pfki.read_address_src( msg, paddr_src ) != PFKI_OK ) ||
		( pfki.read_address_dst( msg, paddr_dst ) != PFKI_OK ) )
	{
		log.txt( LOG_ERROR,
			"K! : failed to read policy address info\n" );

		return LIBIKE_FAILED;
	}

	char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
	char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

	text_addr( txtid_src, &paddr_src, true, true );
	text_addr( txtid_dst, &paddr_dst, true, true );

	log.txt( LOG_DECODE,
		"ii : - seq  = 0x%08x\n"
		"ii : - spi  = 0x%08x\n"
		"ii : - src  = %s\n"
		"ii : - dst  = %s\n",
		msg.hdr->sadb_msg_seq,
		ntohl( sa.spi ),
		txtid_src,
		txtid_dst );

	//
	// convert the pfkey sainfo type
	// to a known isakmp protocol
	//

	unsigned char proto;

	switch( msg.hdr->sadb_msg_satype )
	{
		case SADB_SATYPE_AH:
			proto = ISAKMP_PROTO_IPSEC_AH;
			break;

		case SADB_SATYPE_ESP:
			proto = ISAKMP_PROTO_IPSEC_ESP;
			break;

		case SADB_X_SATYPE_IPCOMP:
			proto = ISAKMP_PROTO_IPCOMP;
			break;

		default:
		{
			log.txt( LOG_ERROR,
				"!! : unhandled pfkey spi protocol type %i\n",
				msg.hdr->sadb_msg_satype );

			return LIBIKE_FAILED;
		}
	}

	//
	// locate the phase2 handler by
	// the message seqid
	//

	IDB_PH2 * ph2;
	if( !get_phase2( true, &ph2, NULL, 0, 0, &msg.hdr->sadb_msg_seq, NULL, NULL, NULL ) )
	{
		log.txt( LOG_ERROR,
			"!! : unable to locate phase2 for getspi update ( msg seq = %u )\n",
			msg.hdr->sadb_msg_seq );

		return LIBIKE_FAILED;
	}

	//
	// we only need to process update
	// message data associated with
	// inbound sas.
	//

	if( msg.hdr->sadb_msg_seq == ph2->seqid_out )
	{
		ph2->dec( true );
		return LIBIKE_OK;
	}

	//
	// step through the appropriate list
	// of proposals / transforms and set
	// the spi for all matched protocols
	//

	IKE_PROPOSAL * proposal;

	long pindex = 0;
	long pcount = 0;
	long tindex;
	long tcount;

	while( ph2->plist_l.nextp( &proposal, pindex, tindex, tcount ) )
	{
		if( proposal->proto != proto )
			continue;

		while( ph2->plist_l.nextt( &proposal, tindex ) )
		{
			switch( proto )
			{
				case ISAKMP_PROTO_IPSEC_AH:
				case ISAKMP_PROTO_IPSEC_ESP:
					proposal->spi.spi = sa.spi;
					proposal->spi.size = ISAKMP_SPI_SIZE;
					break;

				case ISAKMP_PROTO_IPCOMP:
					proposal->spi.cpi = ntohs( ( unsigned short ) ntohl( sa.spi ) );
					proposal->spi.size = ISAKMP_CPI_SIZE;
					break;
			}
		}

		pcount++;
	}

	log.txt( LOG_DEBUG,
		"ii : updated spi for %i %s proposal\n",
		pcount,
		find_name( NAME_PROTOCOL, proto ) );

	//
	// once all spis have been accounted
	// for, we can initiate phase2 with
	// our peer
	//

	ph2->spicount--;

	if( ph2->spicount )
		log.txt( LOG_DEBUG, "ii : waiting for %i spi updates\n", ph2->spicount );
	else
	{
		ph2->lstate |= LSTATE_HASSPI;

		//
		// attempt to locate a phase1 sa
		// for this tunnel
		//

		IDB_PH1 * ph1;
		if( !get_phase1( true, &ph1, ph2->tunnel, 0, 0, NULL ) )
		{
			//
			// mark phase2 as pending
			//

			ph2->lstate |= LSTATE_PENDING;

			//
			// initiate a new phase1
			//

			ph1 = new IDB_PH1( ph2->tunnel, true, NULL );

			if( ph1 == NULL )
			{
				ph2->dec( true );
				return LIBIKE_FAILED;
			}

			if( !ph1->add( true ) )
			{
				delete ph1;

				ph2->dec( true );
				return LIBIKE_FAILED;
			}

			process_phase1_send( ph1 );
		}

		if( ph1->lstate & LSTATE_MATURE )
			process_phase2_send( ph1, ph2 );

		ph1->dec( true );
	}

	//
	// cleanup
	//

	ph2->dec( true );

	return LIBIKE_OK;
}

long _IKED::pfkey_recv_flush( PFKI_MSG & msg )
{
	lock_sdb.lock();

	long count = iked.list_phase2.get_count();
	long index = 0;

	for( ; index < count; index++ )
	{
		IDB_PH2 * ph2 = ( IDB_PH2 * ) iked.list_phase2.get_item( index );

		ph2->inc( false );
		ph2->lstate |= ( LSTATE_DELETE | LSTATE_FLUSHED );

		if( ph2->dec( false ) )
		{
			index--;
			count--;
		}
	}

	lock_sdb.unlock();

	return LIBIKE_OK;
}

long _IKED::pfkey_recv_spdel( PFKI_MSG & msg )
{
	PFKI_SPINFO	spinfo;
	memset( &spinfo, 0, sizeof( spinfo ) );

	if( pfki.read_policy( msg, spinfo ) != PFKI_OK )
	{
		log.txt( LOG_ERROR,
			"!! : failed to read spdel policy data\n" );

		return LIBIKE_FAILED;
	}

	log.txt( LOG_DECODE,
		"ii : - id   = %i\n"
		"ii : - type = %s\n"
		"ii : - dir  = %s\n",
		spinfo.sp.id,
		pfki.name( NAME_SPTYPE, spinfo.sp.type ),
		pfki.name( NAME_SPDIR, spinfo.sp.dir ) );

	//
	// locate the sp by id
	//

	IDB_POLICY * policy;
	if( !get_policy(
			true,
			&policy,
			spinfo.sp.dir,
			spinfo.sp.type,
			&spinfo.sp.id,
			NULL,
			NULL,
			NULL,
			NULL ) )
	{
		log.txt( LOG_ERROR,
			"!! : failed to locate policy by id %i\n",
			spinfo.sp.id );

		return LIBIKE_FAILED;
	}

	//
	// attempt to remove sp
	//

	policy->lstate = LSTATE_DELETE;
	policy->dec( true );

	return LIBIKE_OK;
}

long _IKED::pfkey_recv_spflush( PFKI_MSG & msg )
{
	lock_sdb.lock();

	long count = iked.list_policy.get_count();
	long index = 0;

	for( ; index < count; index++ )
	{
		IDB_POLICY * policy = ( IDB_POLICY * ) iked.list_policy.get_item( index );

		policy->inc( false );
		policy->lstate |= LSTATE_DELETE;

		if( policy->dec( false ) )
		{
			index--;
			count--;
		}
	}

	lock_sdb.unlock();

	return LIBIKE_OK;
}

long _IKED::pfkey_send_getspi( IDB_POLICY * policy, IDB_PH2 * ph2 )
{
	PFKI_SAINFO sainfo;
	memset( &sainfo, 0, sizeof( sainfo ) );

	//
	// determine natt port usage
	//

	bool use_ports = false;
	if( ph2->tunnel->natt_v != IPSEC_NATT_NONE )
		use_ports = true;

	//
	// convert source and destination
	// and store sequence ids
	//

	sainfo.paddr_src.proto = IPSEC_PROTO_ANY;
	sainfo.paddr_dst.proto = IPSEC_PROTO_ANY;

	switch( policy->sp.dir )
	{
		case IPSEC_DIR_INBOUND:
			cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_src.saddr, use_ports );
			cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_dst.saddr, use_ports );
			sainfo.seq = ph2->seqid_in;
			break;

		case IPSEC_DIR_OUTBOUND:
			cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_src.saddr, use_ports );
			cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_dst.saddr, use_ports );
			sainfo.seq = ph2->seqid_out;
			break;
	}

	//
	// configure the spiinfo parameters
	//

	char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
	char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

	text_addr( txtid_src, &sainfo.paddr_src, true, true );
	text_addr( txtid_dst, &sainfo.paddr_dst, true, true );

	//
	// send a getspi request for
	// each policy protocol
	//

	long xindex = 0;
	while( xindex < PFKI_MAX_XFORMS )
	{
		if( !policy->xforms[ xindex ].proto )
			break;

		sainfo.sa2.mode = policy->xforms[ xindex ].mode;
		sainfo.sa2.reqid = policy->xforms[ xindex ].reqid;

		unsigned char proto;

		switch( policy->xforms[ xindex ].proto )
		{
			case PROTO_IP_AH:
				proto = ISAKMP_PROTO_IPSEC_AH;
				sainfo.satype = SADB_SATYPE_AH;
				break;

			case PROTO_IP_ESP:
				proto = ISAKMP_PROTO_IPSEC_ESP;
				sainfo.satype = SADB_SATYPE_ESP;
				break;

			case PROTO_IP_IPCOMP:
				proto = ISAKMP_PROTO_IPCOMP;
				sainfo.satype = SADB_X_SATYPE_IPCOMP;
				sainfo.range.min = 0x100;
				sainfo.range.max = 0xffff;
				break;
		}

		//
		// step through all proposals and
		// store the request id to be used
		// when sending the update message.
		// we also need to acquire the spi
		// to be used for outbound sas
		//

		if( policy->sp.dir == IPSEC_DIR_INBOUND )
		{
			IKE_PROPOSAL * proposal;
			long pindex = 0;

			while( ph2->plist_l.get( &proposal, pindex++ ) )
			{
				//
				// match the protocol type
				//

				if( proposal->proto == proto )
				{
					//
					// copy the request id
					//

					proposal->reqid = ( uint16_t ) sainfo.sa2.reqid;
				}
			}
		}

		if( policy->sp.dir == IPSEC_DIR_OUTBOUND )
		{
			//
			// step through all proposals and
			// store the request id to be used
			// when sending the update message
			//

			IKE_PROPOSAL * proposal;
			long pindex = 0;

			while( ph2->plist_r.get( &proposal, pindex++ ) )
			{
				//
				// if the protocol type matches
				// then store the request id and
				// obtain the spi value
				//

				if( proposal->proto == proto )
				{
					//
					// copy the request id
					//

					proposal->reqid = ( uint16_t ) sainfo.sa2.reqid;

					//
					// copy the spi value
					//

					if( proto != ISAKMP_PROTO_IPCOMP )
					{
						sainfo.range.min = ntohl( proposal->spi.spi );
						sainfo.range.max = ntohl( proposal->spi.spi );
					}
					else
					{
						sainfo.range.min = ntohs( proposal->spi.cpi );
						sainfo.range.max = ntohs( proposal->spi.cpi );
					}
				}
			}
		}
		else
		{
			//
			// inbound spis are dictated by
			// our kernel. we will need to
			// wait for an update response
			//

			ph2->spicount++;
		}

		//
		// send our getspi request to pfkey
		//

		log.txt( LOG_DEBUG,
			"K> : send %s %s pfkey message\n",
			pfki.name( NAME_MSGTYPE, SADB_GETSPI ),
			pfki.name( NAME_SATYPE, sainfo.satype ) );

		log.txt( LOG_DECODE,
			"ii : - seq   = 0x%08x\n"
			"ii : - mode  = %s\n"
			"ii : - reqid = 0x%08x\n"
			"ii : - min   = 0x%08x\n"
			"ii : - max   = 0x%08x\n"
			"ii : - src   = %s\n"
			"ii : - dst   = %s\n",
			sainfo.seq,
			pfki.name( NAME_SPMODE, sainfo.sa2.mode ),
			sainfo.sa2.reqid,
			ntohl( sainfo.range.min ),
			ntohl( sainfo.range.max ),
			txtid_src,
			txtid_dst );

		pfki.send_getspi( sainfo );

		xindex++;
	}

	return LIBIKE_OK;
}

long _IKED::pfkey_send_update( IDB_PH2 * ph2, IKE_PROPOSAL * proposal, BDATA & ekey, BDATA & akey, long dir )
{
	PFKI_SAINFO sainfo;
	memset( &sainfo, 0, sizeof( sainfo ) );

	//
	// convert mode
	//

	switch( proposal->encap )
	{
		case ISAKMP_ENCAP_TRANSPORT:
		case ISAKMP_ENCAP_RFC_UDP_TRANSPORT:
		case ISAKMP_ENCAP_V02_UDP_TRANSPORT:
			sainfo.sa2.mode = IPSEC_MODE_TRANSPORT;
			break;

		case ISAKMP_ENCAP_TUNNEL:
		case ISAKMP_ENCAP_RFC_UDP_TUNNEL:
		case ISAKMP_ENCAP_V02_UDP_TUNNEL:
			sainfo.sa2.mode = IPSEC_MODE_TUNNEL;
			break;
	}

	//
	// copy the request id
	//

	sainfo.sa2.reqid = proposal->reqid;

	//
	// convert encryption and message
	// authentication algorithms. also
	// include natt information if esp
	// protocol is using udp encap
	//

	switch( proposal->proto )
	{
		case ISAKMP_PROTO_IPSEC_ESP:
		{
			sainfo.satype = SADB_SATYPE_ESP;
			sainfo.sa.spi = proposal->spi.spi;
			sainfo.sa.replay = PFKI_WINDSIZE;

			switch( proposal->xform )
			{
				case ISAKMP_ESP_DES_IV64:
				case ISAKMP_ESP_DES:
					sainfo.sa.encrypt = SADB_EALG_DESCBC;
					break;

				case ISAKMP_ESP_3DES:
					sainfo.sa.encrypt = SADB_EALG_3DESCBC;
					break;

//				case ISAKMP_ESP_CAST:
//					sainfo.sa.encrypt = SADB_X_EALG_CAST128CBC;
//					break;

				case ISAKMP_ESP_BLOWFISH:
					sainfo.sa.encrypt = SADB_X_EALG_BLOWFISHCBC;
					break;

				case ISAKMP_ESP_3IDEA:
					sainfo.sa.encrypt = SADB_EALG_DESCBC;
					break;

				case ISAKMP_ESP_DES_IV32:
					sainfo.sa.encrypt = SADB_EALG_DESCBC;
					break;

				case ISAKMP_ESP_AES:
					sainfo.sa.encrypt = SADB_X_EALG_AESCBC;
					break;

				case ISAKMP_ESP_NULL:
					sainfo.sa.encrypt = SADB_EALG_NULL;
					break;

				default:
				{
					log.txt( LOG_ERROR,
						"!! : unhandled ESP transform %s ( %i )\n",
						find_name( NAME_XFORM_ESP, proposal->xform ),
						proposal->xform );

					return LIBIKE_FAILED;
				}
			}

			switch( proposal->hash_id )
			{
				case ISAKMP_AUTH_HMAC_MD5:
					sainfo.sa.auth = SADB_AALG_MD5HMAC;
					break;

				case ISAKMP_AUTH_HMAC_SHA:
					sainfo.sa.auth = SADB_AALG_SHA1HMAC;
					break;

				default:
				{
					log.txt( LOG_ERROR,
						"!! : unhandled ESP auth type %s ( %i )\n",
						find_name( NAME_MAUTH, proposal->hash_id ),
						proposal->hash_id );

					return LIBIKE_FAILED;
				}
			}

#ifdef OPT_NATT

			switch( proposal->encap )
			{
				case ISAKMP_ENCAP_RFC_UDP_TUNNEL:
				case ISAKMP_ENCAP_V02_UDP_TUNNEL:
				case ISAKMP_ENCAP_RFC_UDP_TRANSPORT:
				case ISAKMP_ENCAP_V02_UDP_TRANSPORT:
				{
					sainfo.natt.type = UDP_ENCAP_ESPINUDP;

					switch( dir )
					{
						case IPSEC_DIR_INBOUND:
							get_sockport( ph2->tunnel->saddr_r.saddr, sainfo.natt.port_src );
							get_sockport( ph2->tunnel->saddr_l.saddr, sainfo.natt.port_dst );
							break;

						case IPSEC_DIR_OUTBOUND:
							get_sockport( ph2->tunnel->saddr_l.saddr, sainfo.natt.port_src );
							get_sockport( ph2->tunnel->saddr_r.saddr, sainfo.natt.port_dst );
							break;
					}

					break;
				}
			}

#endif

			break;
		}

		case ISAKMP_PROTO_IPSEC_AH:
		{
			sainfo.satype = SADB_SATYPE_AH;
			sainfo.sa.spi = proposal->spi.spi;

			switch( proposal->xform )
			{
				case ISAKMP_AH_MD5:
					sainfo.sa.auth = SADB_AALG_MD5HMAC;
					break;

				case ISAKMP_AH_SHA:
					sainfo.sa.auth = SADB_AALG_SHA1HMAC;
					break;

				default:
				{
					log.txt( LOG_ERROR,
						"!! : unhandled AH transform %s ( %i )\n",
						find_name( NAME_XFORM_AH, proposal->xform ),
						proposal->xform );

					return LIBIKE_FAILED;
				}
			}

			break;
		}

		case ISAKMP_PROTO_IPCOMP:
		{
			sainfo.satype = SADB_X_SATYPE_IPCOMP;
			sainfo.sa.spi = htonl( ntohs( proposal->spi.cpi ) );

			switch( proposal->xform )
			{
				case ISAKMP_IPCOMP_OUI:
					sainfo.sa.encrypt = SADB_X_CALG_OUI;
					break;

				case ISAKMP_IPCOMP_DEFLATE:
					sainfo.sa.encrypt = SADB_X_CALG_DEFLATE;
					break;

				case ISAKMP_IPCOMP_LZS:
					sainfo.sa.encrypt = SADB_X_CALG_LZS;
					break;

				default:
				{
					log.txt( LOG_ERROR,
						"!! : unhandled IPCOMP transform %s ( %i )\n",
						find_name( NAME_XFORM_AH, proposal->xform ),
						proposal->xform );

					return LIBIKE_FAILED;
				}
			}

			break;
		}

		default:
		{
			log.txt( LOG_ERROR,
				"!! : unhandled ike protocol type %i\n",
				proposal->proto );

			return LIBIKE_FAILED;
		}
	}

	//
	// convert lifetime values
	//

	sainfo.ltime_hard.addtime	= proposal->life_sec;
	sainfo.ltime_hard.bytes		= proposal->life_kbs * 1024;
	
	sainfo.ltime_soft.addtime	= proposal->life_sec;
	sainfo.ltime_soft.addtime	*= PFKEY_SOFT_LIFETIME_RATE;
	sainfo.ltime_soft.addtime	/= 100;
	sainfo.ltime_soft.bytes		= proposal->life_kbs * 1024;
	sainfo.ltime_soft.bytes		*= PFKEY_SOFT_LIFETIME_RATE;
	sainfo.ltime_soft.bytes		/= 100;

	//
	// determine natt port usage
	//

	bool use_ports = false;
	if( ph2->tunnel->natt_v != IPSEC_NATT_NONE )
		use_ports = true;

	//
	// convert source and destination
	// and store sequence ids
	//

	sainfo.paddr_src.proto = IPSEC_PROTO_ANY;
	sainfo.paddr_dst.proto = IPSEC_PROTO_ANY;

	switch( dir )
	{
		case IPSEC_DIR_INBOUND:
			cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_src.saddr, use_ports );
			cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_dst.saddr, use_ports );
			sainfo.seq = ph2->seqid_in;
			break;

		case IPSEC_DIR_OUTBOUND:
			cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_src.saddr, use_ports );
			cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_dst.saddr, use_ports );
			sainfo.seq = ph2->seqid_out;
			break;
	}

	//
	// convert encryption keys
	//

	if( ekey.size() )
	{
		sainfo.ekey.length = ( u_int16_t ) ekey.size();
		memcpy( sainfo.ekey.keydata, ekey.buff(), ekey.size() );
	}

	if( akey.size() )
	{
		sainfo.akey.length = ( u_int16_t ) akey.size();
		memcpy( sainfo.akey.keydata, akey.buff(), akey.size() );
	}

	//
	// send sa update to pfkey
	//

	char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
	char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

	text_addr( txtid_src, &sainfo.paddr_src, true, true );
	text_addr( txtid_dst, &sainfo.paddr_dst, true, true );

	long nametype = NAME_SAENCR;
	if( sainfo.satype == SADB_X_SATYPE_IPCOMP )
		nametype = NAME_SACOMP;

	log.txt( LOG_DEBUG,
		"K> : send %s %s pfkey message\n",
		pfki.name( NAME_MSGTYPE, SADB_UPDATE ),
		pfki.name( NAME_SATYPE, sainfo.satype ) );

	log.txt( LOG_DECODE,
		"ii : - spi  = 0x%08x\n"
		"ii : - src  = %s\n"
		"ii : - dst  = %s\n"
		"ii : - encr = %s\n"
		"ii : - ekey = %i bits\n"
		"ii : - auth = %s\n"
		"ii : - akey = %i bits\n"
		"ii : - hard = %i\n"
		"ii : - soft = %i\n"
		"ii : - natt = %s\n"
		"ii : - nsrc = %i\n"
		"ii : - ndst = %i\n",
		ntohl( sainfo.sa.spi ),
		txtid_src,
		txtid_dst,
		pfki.name( nametype, sainfo.sa.encrypt ),
		sainfo.ekey.length * 8,
		pfki.name( NAME_SAAUTH, sainfo.sa.auth ),
		sainfo.akey.length * 8,
		long( sainfo.ltime_hard.addtime ),
		long( sainfo.ltime_soft.addtime ),
		pfki.name( NAME_NTTYPE, sainfo.natt.type ),
		ntohs( sainfo.natt.port_src ),
		ntohs( sainfo.natt.port_dst ) );

	pfki.send_update( sainfo );

	return LIBIKE_OK;
}

long _IKED::pfkey_send_delete( IDB_PH2 * ph2 )
{
	//
	// send a delete request for
	// each local protocol sa
	//

	IKE_PROPOSAL * proposal;
	long pindex = 0;

	while( ph2->plist_r.get( &proposal, pindex++ ) )
	{
		PFKI_SAINFO sainfo;
		memset( &sainfo, 0, sizeof( sainfo ) );

		//
		// determine the sa endpoint addresses
		//

		bool use_ports = false;
		if( ph2->tunnel->natt_v != IPSEC_NATT_NONE )
			use_ports = true;

		cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_src.saddr, use_ports );
		cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_dst.saddr, use_ports );

		char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
		char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

		text_addr( txtid_src, &sainfo.paddr_src, true, true );
		text_addr( txtid_dst, &sainfo.paddr_dst, true, true );

		//
		// determine the sa type and spi
		//

		switch( proposal->proto )
		{
			case ISAKMP_PROTO_IPSEC_AH:
				sainfo.satype = SADB_SATYPE_AH;
				sainfo.sa.spi = proposal->spi.spi;
				break;

			case ISAKMP_PROTO_IPSEC_ESP:
				sainfo.satype = SADB_SATYPE_ESP;
				sainfo.sa.spi = proposal->spi.spi;
				break;

			case ISAKMP_PROTO_IPCOMP:
				sainfo.satype = SADB_X_SATYPE_IPCOMP;
				sainfo.sa.spi = htonl( ntohs( proposal->spi.cpi ) );
				break;
		}

		log.txt( LOG_DEBUG,
			"K> : send %s %s pfkey message\n",
			pfki.name( NAME_MSGTYPE, SADB_DELETE ),
			pfki.name( NAME_SATYPE, sainfo.satype ) );

		log.txt( LOG_DECODE,
			"ii : - spi   = 0x%08x\n"
			"ii : - src   = %s\n"
			"ii : - dst   = %s\n",
			htonl( sainfo.sa.spi ),
			txtid_src,
			txtid_dst );

		pfki.send_del( sainfo );
	}

	pindex = 0;

	while( ph2->plist_l.get( &proposal, pindex++ ) )
	{
		PFKI_SAINFO sainfo;
		memset( &sainfo, 0, sizeof( sainfo ) );

		//
		// determine the sa endpoint addresses
		//

		bool use_ports = false;
		if( ph2->tunnel->natt_v != IPSEC_NATT_NONE )
			use_ports = true;

		cpy_sockaddr( ph2->tunnel->saddr_r.saddr, sainfo.paddr_src.saddr, use_ports );
		cpy_sockaddr( ph2->tunnel->saddr_l.saddr, sainfo.paddr_dst.saddr, use_ports );

		char txtid_src[ LIBIKE_MAX_TEXTP2ID ];
		char txtid_dst[ LIBIKE_MAX_TEXTP2ID ];

		text_addr( txtid_src, &sainfo.paddr_src, true, true );
		text_addr( txtid_dst, &sainfo.paddr_dst, true, true );

		//
		// determine the sa type and spi
		//

		switch( proposal->proto )
		{
			case ISAKMP_PROTO_IPSEC_AH:
				sainfo.satype = SADB_SATYPE_AH;
				sainfo.sa.spi = proposal->spi.spi;
				break;

			case ISAKMP_PROTO_IPSEC_ESP:
				sainfo.satype = SADB_SATYPE_ESP;
				sainfo.sa.spi = proposal->spi.spi;
				break;

			case ISAKMP_PROTO_IPCOMP:
				sainfo.satype = SADB_X_SATYPE_IPCOMP;
				sainfo.sa.spi = htonl( ntohs( proposal->spi.cpi ) );
				break;
		}

		log.txt( LOG_DEBUG,
			"K> : send %s %s pfkey message\n",
			pfki.name( NAME_MSGTYPE, SADB_DELETE ),
			pfki.name( NAME_SATYPE, sainfo.satype ) );

		log.txt( LOG_DECODE,
			"ii : - spi   = 0x%08x\n"
			"ii : - src   = %s\n"
			"ii : - dst   = %s\n",
			htonl( sainfo.sa.spi ),
			txtid_src,
			txtid_dst );

		pfki.send_del( sainfo );
	}

	return LIBIKE_OK;
}

long _IKED::pfkey_send_spadd( PFKI_SPINFO * spinfo )
{
	log.txt( LOG_DEBUG,
		"K> : send %s %s pfkey message\n",
		pfki.name( NAME_MSGTYPE, SADB_X_SPDADD ),
		pfki.name( NAME_SATYPE, SADB_SATYPE_UNSPEC ) );

	long result = pfki.send_spadd( *spinfo );
	if( result != PFKI_OK )
		return LIBIKE_FAILED;

	return LIBIKE_OK;
}

long _IKED::pfkey_send_spdel( PFKI_SPINFO * spinfo )
{
	log.txt( LOG_DEBUG,
		"K> : send %s %s pfkey message\n",
		pfki.name( NAME_MSGTYPE, SADB_X_SPDDELETE2 ),
		pfki.name( NAME_SATYPE, SADB_SATYPE_UNSPEC ) );

	long result = pfki.send_spdel( *spinfo );
	if( result != PFKI_OK )
		return LIBIKE_FAILED;

	return LIBIKE_OK;
}
