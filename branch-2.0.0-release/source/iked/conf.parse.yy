%skeleton "lalr1.cc"
%defines
%define "parser_class_name" "conf_parser"

%{

#include <string>

#include "libip.h"

typedef class _IKED IKED;

%}

// The parsing context.
%parse-param { IKED & iked }
%lex-param   { IKED & iked }
%locations
%debug
%error-verbose

// Symbols.
%union
{
	int		ival;
	BDATA *	bval;
};

%{

#include "iked.h"

IKE_PROPOSAL	proposal;
IKE_ILIST *	ilist;
IDB_PEER *	peer;

%}

%token		BCB		"begin section"
%token		ECB		"end section"
%token		BSB		"begin grouping"
%token		ESB		"end grouping"
%token		EOS		"end of statement"
%token		SEQ		"sequence"
%token		END 0		"end of file"

%token		ENABLE		"enable"
%token		DISABLE		"disable"
%token		FORCE		"force"

%token		DAEMON		"deamon section"
%token		SOCKET		"socket"
%token		IKE		"ike"
%token		NATT		"natt"
%token		LL_NONE		"none"
%token		LL_ERROR	"error"
%token		LL_INFO		"info"
%token		LL_DEBUG	"debug"
%token		LL_LOUD		"loud"
%token		LL_DECODE	"decode"
%token		LOG_LEVEL	"log level"
%token		LOG_FILE	"log file"
%token		PCAP_IKE	"ike pcap file"
%token		PCAP_PUB	"pub pcap file"
%token		RETRY_COUNT	"retry count"
%token		RETRY_DELAY	"retry delay"

%token		NETGROUP	"netgroup section"

%token		XAUTH_LDAP	"xauth ldap section"
%token		LD_VERSION	"version"
%token		LD_URL		"url"
%token		LD_BASE		"base"
%token		LD_SUBTREE	"subtree"
%token		LD_BIND_DN	"bind dn"
%token		LD_BIND_PW	"bind pw"
%token		LD_ATTR_USER	"user attribute"
%token		LD_ATTR_GROUP	"group attribute"
%token		LD_ATTR_MEMBER	"member attribute"

%token		XCONF_LOCAL	"xconfig local section"
%token		NETWORK4	"network4"
%token		DNSS4		"domain name server"
%token		NBNS4		"nerbios name server"
%token		DNS_SUFFIX	"dns suffix"
%token		DNS_LIST	"dns list"
%token		BANNER		"banner"
%token		PFS_GROUP	"pfs group"

%token		PEER		"peer section"
%token		CONTACT		"contact type"
%token		INITIATOR	"initiator"
%token		RESPONDER	"responder"
%token		EXCHANGE	"exchange type"
%token		MAIN		"main"
%token		AGGRESSIVE	"aggressive"
%token		NATT_MODE	"natt mode"
%token		NATT_RATE	"natt keepalive rate"
%token		DPD_MODE	"dpd mode"
%token		DPD_RATE	"dpd notifiy rate"
%token		FRAG_IKE_MODE	"frag ike mode"
%token		FRAG_IKE_SIZE	"frag ike size"
%token		FRAG_ESP_MODE	"frag esp mode"
%token		FRAG_ESP_SIZE	"frag esp size"
%token		PEERID		"peer id"
%token		LOCAL		"local"
%token		REMOTE		"remote"
%token		ADDR		"address"
%token		FQDN		"fqdn"
%token		UFQDN		"ufqdn"
%token		ASN1DN		"asn1dn"
%token		KEYID		"keyid"
%token		AUTHDATA	"authdata"
%token		PSK		"psk"
%token		CA		"ca"
%token		CERT		"cert"
%token		PKEY		"pkey"
%token		LIFE_CHECK	"lifetime check"
%token		OBEY		"obey"
%token		CLAIM		"claim"
%token		STRICT		"strict"
%token		EXACT		"exact"
%token		XAUTH_SOURCE	"xauth source"
%token		XCONF_SOURCE	"xconf source"
%token		PULL		"pull"
%token		PUSH		"push"
%token		LDAP		"ldap"
%token		PLCY_MODE	"policy mode"
%token		CONFIG		"config"
%token		COMPAT		"compat"

%token		PLCY_LIST	"policy list section"
%token		INCLUDE		"include"
%token		EXCLUDE		"exclude"

%token		PROPOSAL	"proposal section"
%token		ISAKMP		"isakmp"
%token		AH		"ah"
%token		ESP		"esp"
%token		IPCOMP		"ipcomp"
%token		AUTH		"auth"
%token		HYB_XA_RSA	"hybrid xauth rsa"
%token		MUT_XA_RSA	"mutual xauth rsa"
%token		MUT_XA_PSK	"mutual xauth psk"
%token		MUT_RSA		"mutual rsa"
%token		MUT_PSK		"mutual psk"
%token		CIPH		"cipher"
%token		KLEN		"klen"
%token		HASH		"hash"
%token		MSGA		"hmac"
%token		COMP		"compress"
%token		DHGR		"dh group"
%token		ALG_AES		"aes"
%token		ALG_BLOWFISH	"blowfish"
%token		ALG_3DES	"3des"
%token		ALG_CAST	"cast"
%token		ALG_DES		"des"
%token		ALG_MD5		"md5"
%token		ALG_SHA1	"sha1"
%token		ALG_DHGR	"dhgr"
%token		ALG_DEFLATE	"deflate"
%token		ALG_LZS		"lzs"
%token		LIFE_SEC	"life sec"
%token		LIFE_KBS	"life kbs"

%token		NUMBER		"number value"
%token		QUOTED		"quoted value"
%token		LABEL		"label value"
%token		ADDRESS		"address value"
%token		NETWORK		"network value"

%type	<ival>	NUMBER		"number"
%type	<bval>	QUOTED		"quoted"
%type	<bval>	LABEL		"label"
%type	<bval>	ADDRESS		"address"
%type	<bval>	NETWORK		"network"

%destructor { delete $$; }	QUOTED
%destructor { delete $$; }	LABEL
%destructor { delete $$; }	ADDRESS
%destructor { delete $$; }	NETWORK

%%

/*
 * ROOT SECTION
 *
 */

sections
  :	/* nothing */
  |	sections section
  ;
section
  : 	daemon_section
  |	netgroup_section
  |	xauth_ldap_section
  |	xconf_local_section
  |	peer_section
  ;

/*
 * DAEMON SECTION
 *
 */

daemon_section
  :	DAEMON BCB daemon_lines ECB
  ;
daemon_lines
  :	/* nothing */
  |	daemon_lines daemon_line
  ;
daemon_line
  :	SOCKET IKE NUMBER
	{
		IKE_SADDR saddr;
		memset( &saddr, 0, sizeof( saddr ) );
		saddr.saddr4.sin_family = AF_INET;
		SET_SALEN( &saddr.saddr4, sizeof( sockaddr_in ) ); 
		saddr.saddr4.sin_port = htons( $3 );

		if( iked.socket_create( saddr, false ) != LIBIKE_OK )
			error( @$, std::string( "daemon network configuration failed\n" ) );
	}
	EOS
  |	SOCKET IKE ADDRESS NUMBER
	{
		IKE_SADDR saddr;
		memset( &saddr, 0, sizeof( saddr ) );
		SET_SALEN( &saddr.saddr4, sizeof( sockaddr_in ) );
		saddr.saddr4.sin_family = AF_INET;
		saddr.saddr4.sin_addr.s_addr = inet_addr( $3->text() );
		saddr.saddr4.sin_port = htons( $4 );

		if( iked.socket_create( saddr, false ) != LIBIKE_OK )
			error( @$, std::string( "daemon network configuration failed\n" ) );

		delete $3;
	}
	EOS
  |	SOCKET NATT NUMBER
	{
		IKE_SADDR saddr;
		memset( &saddr, 0, sizeof( saddr ) );
		SET_SALEN( &saddr.saddr4, sizeof( sockaddr_in ) );
		saddr.saddr4.sin_family = AF_INET;
		saddr.saddr4.sin_port = htons( $3 );

		if( iked.socket_create( saddr, true ) != LIBIKE_OK )
			error( @$, std::string( "daemon network configuration failed\n" ) );
	}
	EOS
  |	SOCKET NATT ADDRESS NUMBER
	{
		IKE_SADDR saddr;
		memset( &saddr, 0, sizeof( saddr ) );
		SET_SALEN( &saddr.saddr4, sizeof( sockaddr_in ) );
		saddr.saddr4.sin_family = AF_INET;
		saddr.saddr4.sin_addr.s_addr = inet_addr( $3->text() );
		saddr.saddr4.sin_port = htons( $4 );

		if( iked.socket_create( saddr, true ) != LIBIKE_OK )
			error( @$, std::string( "daemon network configuration failed\n" ) );

		delete $3;
	}
	EOS
  |	LOG_FILE QUOTED
	{
		snprintf( iked.path_log, MAX_PATH, $2->text() );
		delete $2;
	}
	EOS
  |	LOG_LEVEL LL_NONE
	{
		iked.level = LOG_NONE;
	}
	EOS
  |	LOG_LEVEL LL_ERROR
	{
		iked.level = LOG_ERROR;
	}
	EOS
  |	LOG_LEVEL LL_INFO
	{
		iked.level = LOG_INFO;
	}
	EOS
  |	LOG_LEVEL LL_DEBUG
	{
		iked.level = LOG_DEBUG;
	}
	EOS
  |	LOG_LEVEL LL_LOUD
	{
		iked.level = LOG_LOUD;
	}
	EOS
  |	LOG_LEVEL LL_DECODE
	{
		iked.level = LOG_DECODE;
	}
	EOS
  |	PCAP_IKE QUOTED
	{
		snprintf( iked.path_ike, MAX_PATH, $2->text() );
		iked.dump_ike = true;
		delete $2;
	}
	EOS
  |	PCAP_PUB QUOTED
	{
		snprintf( iked.path_pub, MAX_PATH, $2->text() );
		iked.dump_pub = true;
		delete $2;
	}
	EOS
  |	RETRY_DELAY NUMBER
	{
		iked.retry_delay = $2;
	}
	EOS
  |	RETRY_COUNT NUMBER
	{
		iked.retry_count = $2;
	}
	EOS
  ;

/*
 * NETGROUP SECTION
 *
 */

netgroup_section
  :	NETGROUP LABEL
	{
		ilist = new IKE_ILIST;
		if( ilist == NULL )
			error( @$, std::string( "unable to allocate idlist for netgroup\n" ) + $2->text() );

		ilist->name.set( *$2 );
		iked.list_netgrp.add_item( ilist );
		delete $2;
	}
	BCB netgroup_lines ECB
  ;
netgroup_lines
  :	/* nothing */
  |	netgroup_lines netgroup_line
  ;
netgroup_line
  :	NETWORK
	{
		char * pos = strchr( $1->text(), '/' );
		*pos = '\0';

		IKE_PH2ID ph2id;
		memset( &ph2id, 0, sizeof( ph2id ) );
		ph2id.type = ISAKMP_ID_IPV4_ADDR_SUBNET;
		ph2id.addr1.s_addr = inet_addr( $1->text() );
		ph2id.addr2.s_addr = 0;

		long bits = strtol( pos + 1, NULL, 10 );
		for( long i = 0; i < bits; i++ )
		{
			ph2id.addr2.s_addr >>= 1;
			ph2id.addr2.s_addr |= 0x80000000;
		}

		ph2id.addr2.s_addr = htonl( ph2id.addr2.s_addr );

		ilist->add( ph2id );
		delete $1;
	}
	EOS
  ;

/*
 * LDAP SECTION
 *
 */

xauth_ldap_section
  :	XAUTH_LDAP
	{
#ifndef OPT_LDAP
		error( @$, std::string( "iked was compiled without ldap support" ) );
#endif
	}
	BCB xauth_ldap_lines ECB
  ;
xauth_ldap_lines
  :	/* nothing */
  |	xauth_ldap_lines xauth_ldap_line
  ;
xauth_ldap_line
  :	LD_VERSION NUMBER
	{
#ifdef OPT_LDAP
		if( ( $2 < 2 ) && ( $2 > 3 ) )
			error( @$, std::string( "ldap version must be 2 or 3" ) );

		iked.xauth_ldap.version = $2;
#endif
	}
	EOS
  |	LD_URL QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.url.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_BASE QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.base.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_SUBTREE ENABLE
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.subtree = true;
#endif
	}
	EOS
  |	LD_SUBTREE DISABLE
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.subtree = false;
#endif
	}
	EOS
  |	LD_BIND_DN QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.bind_dn.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_BIND_PW QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.bind_pw.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_ATTR_USER QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.attr_user.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_ATTR_GROUP QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.attr_group.set( *$2 );
		delete $2;
#endif
	}
	EOS
  |	LD_ATTR_MEMBER QUOTED
	{
#ifdef OPT_LDAP
		iked.xauth_ldap.attr_member.set( *$2 );
		delete $2;
#endif
	}
	EOS
  ;

/*
 * XCONF LOCAL SECTION
 *
 */

xconf_local_section
  :	XCONF_LOCAL BCB xconf_local_lines ECB
  ;
xconf_local_lines
  :	/* nothing */
  |	xconf_local_lines xconf_local_line
  ;
xconf_local_line
  :	NETWORK4 NETWORK
	{
		iked.xconf_local.config.opts |= ( IPSEC_OPTS_ADDR | IPSEC_OPTS_MASK );

		char * pos = strchr( $2->text(), '/' );
		*pos = '\0';

		in_addr base;
		base.s_addr = inet_addr( $2->text() );
		long bits = strtol( pos + 1, NULL, 10 );

		iked.xconf_local.pool4_set( base, bits, 0 );
		delete $2;
	}
	EOS
  |	NETWORK4 NETWORK NUMBER
	{
		iked.xconf_local.config.opts |= ( IPSEC_OPTS_ADDR | IPSEC_OPTS_MASK );

		char * pos = strchr( $2->text(), '/' );
		*pos = '\0';

		in_addr base;
		base.s_addr = inet_addr( $2->text() );
		long bits = strtol( pos + 1, NULL, 10 );

		iked.xconf_local.pool4_set( base, bits, $3 );
		delete $2;
	}
	EOS
  |	DNSS4 ADDRESS
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_DNSS;
		iked.xconf_local.config.dnss.s_addr = inet_addr( $2->text() );
		delete $2;
	}
	EOS
  |	NBNS4 ADDRESS
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_NBNS;
		iked.xconf_local.config.nbns.s_addr = inet_addr( $2->text() );
		delete $2;
	}
	EOS
  |	DNS_SUFFIX QUOTED
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_DOMAIN;

		long len = $2->size();
		if( len >= CONF_STRLEN )
			len = CONF_STRLEN - 1;

		memcpy( iked.xconf_local.config.suffix, $2->text(), len );
		iked.xconf_local.config.suffix[ len ] = 0;
		delete $2;
	}
	EOS
  |	DNS_LIST xconf_local_dns_names
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_SPLITDNS;
	}
	EOS
  |	BANNER QUOTED
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_BANNER;

		FILE * fp = fopen( $2->text(), "r" );
		if( fp == NULL )
			error( @$, std::string( "unable to load file " ) + $2->text() );

		long size;
		char buff[ CONF_STRLEN ];
		while( ( size = fread( buff, 1, CONF_STRLEN, fp ) ) > 0 )
			iked.xconf_local.banner.add( buff, size );
		delete $2;
	}
	EOS
  |	PFS_GROUP NUMBER
	{
		iked.xconf_local.config.opts |= IPSEC_OPTS_PFS;
		iked.xconf_local.config.dhgr = $2;
	}
	EOS
  ;

xconf_local_dns_names
  :	/* nothing */
  |	xconf_local_dns_names xconf_local_dns_name
  ;
xconf_local_dns_name
  :	QUOTED
	{
		iked.xconf_local.dlist.add( *$1 );
		delete $1;
	}
  ;


/*
 * PEER SECTION
 *
 */

peer_section
  :	PEER ADDRESS
	{
		//
		// new peer object
		//

		peer = new IDB_PEER( NULL );

		//
		// set peer default values
		//

		peer->contact = IPSEC_CONTACT_BOTH;
		peer->natt_rate = 30;
		peer->dpd_rate = 15;
		peer->frag_ike_size = 520;
		peer->frag_esp_size = 520;
		peer->life_check = LTIME_CLAIM;
		SET_SALEN( &peer->saddr.saddr4, sizeof( sockaddr_in ) );
		peer->saddr.saddr4.sin_family = AF_INET;
		peer->saddr.saddr4.sin_port = htons( LIBIKE_IKE_PORT );
		peer->saddr.saddr4.sin_addr.s_addr = inet_addr( $2->text() );

		peer->xauth_source = &iked.xauth_local;
		peer->xconf_source = &iked.xconf_local;
		peer->xconf_mode = CONFIG_MODE_PULL;

		delete $2;
	}
	BCB peer_lines ECB
	{
		peer->add( true );
		peer->dec( true );
	}
  |	PEER ADDRESS NUMBER
	{
		//
		// new peer object
		//

		peer = new IDB_PEER( NULL );

		//
		// set peer default values
		//

		peer->contact = IPSEC_CONTACT_BOTH;
		peer->natt_rate = 30;
		peer->dpd_rate = 15;
		peer->frag_ike_size = 520;
		peer->frag_esp_size = 520;
		peer->life_check = LTIME_CLAIM;
		SET_SALEN( &peer->saddr.saddr4, sizeof( sockaddr_in ) );
		peer->saddr.saddr4.sin_family = AF_INET;
		peer->saddr.saddr4.sin_port = htons( $3 );
		peer->saddr.saddr4.sin_addr.s_addr = inet_addr( $2->text() );

		peer->xauth_source = &iked.xauth_local;
		peer->xconf_source = &iked.xconf_local;
		peer->xconf_mode = CONFIG_MODE_PULL;

		delete $2;
	}
	BCB peer_lines ECB
	{
		peer->add( true );
		peer->dec( true );
	}
  ;
peer_lines
  :	/* nothing */
  |	peer_lines peer_line
  ;
peer_line
  :	CONTACT INITIATOR
	{
		peer->contact = IPSEC_CONTACT_INIT;
	}
	EOS
  |	CONTACT RESPONDER
	{
		peer->contact = IPSEC_CONTACT_RESP;
	}
	EOS
  |	EXCHANGE MAIN
	{
		peer->exchange = ISAKMP_EXCH_IDENT_PROTECT;
	}
	EOS
  |	EXCHANGE AGGRESSIVE
	{
		peer->exchange = ISAKMP_EXCH_AGGRESSIVE;
	}
	EOS
  |	NATT_MODE ENABLE
	{
		peer->natt_mode = IPSEC_NATT_ENABLE;
	}
	EOS
  |	NATT_MODE DISABLE
	{
		peer->natt_mode = IPSEC_NATT_DISABLE;
	}
	EOS
  |	NATT_MODE FORCE
	{
		peer->natt_mode = IPSEC_NATT_FORCE;
	}
	EOS
  |	NATT_RATE NUMBER
	{
		peer->natt_rate = $2;
	}
	EOS
  |	DPD_MODE ENABLE
	{
		peer->dpd_mode = IPSEC_DPD_ENABLE;
	}
	EOS
  |	DPD_MODE DISABLE
	{
		peer->dpd_mode = IPSEC_DPD_DISABLE;
	}
	EOS
  |	DPD_MODE FORCE
	{
		peer->dpd_mode = IPSEC_DPD_FORCE;
	}
	EOS
  |	DPD_RATE NUMBER
	{
		peer->dpd_rate = $2;
	}
	EOS
  |	FRAG_IKE_MODE ENABLE
	{
		peer->frag_ike_mode = IPSEC_FRAG_ENABLE;
	}
	EOS
  |	FRAG_IKE_MODE DISABLE
	{
		peer->frag_ike_mode = IPSEC_FRAG_DISABLE;
	}
	EOS
  |	FRAG_IKE_MODE FORCE
	{
		peer->frag_ike_mode = IPSEC_FRAG_FORCE;
	}
	EOS
  |	FRAG_IKE_SIZE NUMBER
	{
		peer->frag_ike_size = $2;
	}
	EOS
  |	FRAG_ESP_MODE ENABLE
	{
		peer->frag_esp_mode = IPSEC_FRAG_ENABLE;
	}
	EOS
  |	FRAG_ESP_MODE DISABLE
	{
		peer->frag_esp_mode = IPSEC_FRAG_DISABLE;
	}
	EOS
  |	FRAG_ESP_SIZE NUMBER
	{
		peer->frag_esp_size = $2;
	}
	EOS
  |	PEERID LOCAL ADDR
	{
		peer->idtype_l = ISAKMP_ID_IPV4_ADDR;
	}
	EOS
  |	PEERID LOCAL ADDR ADDRESS
	{
		peer->idtype_l = ISAKMP_ID_IPV4_ADDR;
		peer->iddata_l.set( *$4 );
		delete $4;
	}
	EOS
  |	PEERID LOCAL FQDN QUOTED
	{
		peer->idtype_l = ISAKMP_ID_FQDN;
		peer->iddata_l.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID LOCAL UFQDN QUOTED
	{
		peer->idtype_l = ISAKMP_ID_USER_FQDN;
		peer->iddata_l.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID LOCAL KEYID QUOTED
	{
		peer->idtype_l = ISAKMP_ID_KEY_ID;
		peer->iddata_l.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID LOCAL ASN1DN
	{
		peer->idtype_l = ISAKMP_ID_ASN1_DN;
	}
	EOS
  |	PEERID LOCAL ASN1DN QUOTED
	{
		peer->idtype_l = ISAKMP_ID_ASN1_DN;
		peer->iddata_l.set( *$4 );
		delete $4;
	}
	EOS
  |	PEERID REMOTE ADDR
	{
		peer->idtype_r = ISAKMP_ID_IPV4_ADDR;
	}
	EOS
  |	PEERID REMOTE ADDR ADDRESS
	{
		peer->idtype_r = ISAKMP_ID_IPV4_ADDR;
		peer->iddata_r.set( *$4 );
		delete $4;
	}
	EOS
  |	PEERID REMOTE FQDN QUOTED
	{
		peer->idtype_r = ISAKMP_ID_FQDN;
		peer->iddata_r.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID REMOTE UFQDN QUOTED
	{
		peer->idtype_r = ISAKMP_ID_USER_FQDN;
		peer->iddata_r.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID REMOTE KEYID QUOTED
	{
		peer->idtype_r = ISAKMP_ID_KEY_ID;
		peer->iddata_r.set( $4->buff(), $4->size() - 1 );
		delete $4;
	}
	EOS
  |	PEERID REMOTE ASN1DN
	{
		peer->idtype_r = ISAKMP_ID_ASN1_DN;
	}
	EOS
  |	PEERID REMOTE ASN1DN QUOTED
	{
		peer->idtype_r = ISAKMP_ID_ASN1_DN;
		peer->iddata_r.set( *$4 );
		delete $4;
	}
	EOS
  |	AUTHDATA PSK QUOTED
	{
		peer->psk.set( *$3 );
		delete $3;
	}
	EOS
  |	AUTHDATA CA QUOTED
	{
		peer->fpass.del();

		if( iked.cert_load_pem(
			peer->cert_r,
			$3->text(),
			true,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );
		delete $3;
	}
	EOS
  |	AUTHDATA CA QUOTED QUOTED
	{
		peer->fpass.set( *$4 );

		if( iked.cert_load_pem(
			peer->cert_r,
			$3->text(),
			true,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );

		delete $3;
		delete $4;
	}
	EOS
  |	AUTHDATA CERT QUOTED
	{
		peer->fpass.del();
		
		if( iked.cert_load_pem(
			peer->cert_l,
			$3->text(),
			false,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );

		delete $3;
	}
	EOS
  |	AUTHDATA CERT QUOTED QUOTED
	{
		peer->fpass.set( *$4 );
		
		if( iked.cert_load_pem(
			peer->cert_l,
			$3->text(),
			false,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );

		delete $3;
		delete $4;
	}
	EOS
  |	AUTHDATA PKEY QUOTED
	{
		peer->fpass.del();
		
		if( iked.prvkey_rsa_load_pem(
			$3->text(),
			&peer->key,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );

		delete $3;
	}
	EOS
  |	AUTHDATA PKEY QUOTED QUOTED
	{
		peer->fpass.set( *$4 );
		
		if( iked.prvkey_rsa_load_pem(
			$3->text(),
			&peer->key,
			peer->fpass ) != FILE_OK )
			error( @$, std::string( "unable to load file " ) + $3->text() );

		delete $3;
		delete $4;
	}
	EOS
  |	LIFE_CHECK OBEY
	{
		peer->life_check = LTIME_OBEY;
	}
	EOS
  |	LIFE_CHECK CLAIM
	{
		peer->life_check = LTIME_CLAIM;
	}
	EOS
  |	LIFE_CHECK STRICT
	{
		peer->life_check = LTIME_STRICT;
	}
	EOS
  |	LIFE_CHECK EXACT
	{
		peer->life_check = LTIME_EXACT;
	}
	EOS
  |	XAUTH_SOURCE LOCAL
	{
		peer->xauth_source = &iked.xauth_local;
	}
	EOS
  |	XAUTH_SOURCE LOCAL QUOTED
	{
		peer->xauth_source = &iked.xauth_local;
		peer->xauth_group.set( *$3 );
		delete $3;
	}
	EOS
  |	XAUTH_SOURCE LDAP
	{
#ifdef OPT_LDAP
		if( !iked.xauth_ldap.url.size() )
			error( @$, std::string( "conf source is ldap but no url is defined" ) );

		peer->xauth_source = &iked.xauth_ldap;
#else
		error( @$, std::string( "iked was compiled without ldap support" ) );
#endif
	}
	EOS
  |	XAUTH_SOURCE LDAP QUOTED
	{
#ifdef OPT_LDAP
		if( !iked.xauth_ldap.url.size() )
			error( @$, std::string( "conf source is ldap but no url is defined" ) );

		peer->xauth_source = &iked.xauth_ldap;
		peer->xauth_group.set( *$3 );
		delete $3;
#else
		error( @$, std::string( "iked was compiled without ldap support" ) );
#endif
	}
	EOS
  |	XCONF_SOURCE LOCAL
	{
		peer->xconf_source = &iked.xconf_local;
		peer->xconf_mode = CONFIG_MODE_PULL;
	}
	EOS
  |	XCONF_SOURCE LOCAL PULL
	{
		peer->xconf_source = &iked.xconf_local;
		peer->xconf_mode = CONFIG_MODE_PULL;
	}
	EOS
  |	XCONF_SOURCE LOCAL PUSH
	{
		peer->xconf_source = &iked.xconf_local;
		peer->xconf_mode = CONFIG_MODE_PUSH;
	}
	EOS
  |	PLCY_MODE DISABLE
	{
		peer->plcy_mode = POLICY_MODE_DISABLE;
	}
	EOS
  |	PLCY_MODE COMPAT
	{
		peer->plcy_mode = POLICY_MODE_COMPAT;
	}
	EOS
  |	PLCY_MODE CONFIG
	{
		peer->plcy_mode = POLICY_MODE_CONFIG;
	}
	EOS
  |	PLCY_LIST BCB plcy_list_lines ECB
  |	PROPOSAL ISAKMP
	{
		memset( &proposal, 0, sizeof( proposal ) );
		proposal.proto = ISAKMP_PROTO_ISAKMP;
		proposal.xform = ISAKMP_KEY_IKE;
		proposal.dhgr_id = IKE_GRP_GROUP2;
	}
	BCB proposal_lines_isakmp ECB
	{
		peer->prop_list.add( &proposal, true );
	}
  |	PROPOSAL AH
	{
		memset( &proposal, 0, sizeof( proposal ) );
		proposal.proto = ISAKMP_PROTO_IPSEC_AH;
	}
	BCB proposal_lines_ah ECB
	{
		peer->prop_list.add( &proposal, true );
	}
  |	PROPOSAL ESP
	{
		memset( &proposal, 0, sizeof( proposal ) );
		proposal.proto = ISAKMP_PROTO_IPSEC_ESP;
	}
	BCB proposal_lines_esp ECB
	{
		peer->prop_list.add( &proposal, true );
	}
  |	PROPOSAL IPCOMP
	{
		memset( &proposal, 0, sizeof( proposal ) );
		proposal.proto = ISAKMP_PROTO_IPCOMP;
	}
	BCB proposal_lines_ipcomp ECB
	{
		peer->prop_list.add( &proposal, true );
	}
  ;

plcy_list_lines
  :	/* nothing */
  |	plcy_list_lines plcy_list_line
  ;
plcy_list_line
  :	INCLUDE LABEL
	{
		long index = 0;
		while( true )
		{
			ilist = ( IKE_ILIST * ) iked.list_netgrp.get_item( index++ );
			if( ilist == NULL )
				break;

			if( !strcmp( $2->text(), ilist->name.text() ) )
				break; 
		}

		if( ilist == NULL )
			error( @$, std::string( "unknown netgroup " ) + $2->text() );

		peer->netmap_add( ilist, UNITY_SPLIT_INCLUDE, NULL );

		delete $2;
	}
	EOS
  |	INCLUDE LABEL QUOTED
	{
		long index = 0;
		while( true )
		{
			ilist = ( IKE_ILIST * ) iked.list_netgrp.get_item( index++ );
			if( ilist == NULL )
				break;

			if( !strcmp( $2->text(), ilist->name.text() ) )
				break; 
		}

		if( ilist == NULL )
			error( @$, std::string( "unknown netgroup " ) + $2->text() );

		peer->netmap_add( ilist, UNITY_SPLIT_INCLUDE, $3 );

		delete $2;
		delete $3;
	}
	EOS
  |	EXCLUDE LABEL
	{
		long index = 0;
		while( true )
		{
			ilist = ( IKE_ILIST * ) iked.list_netgrp.get_item( index++ );
			if( ilist == NULL )
				break;

			if( !strcmp( $2->text(), ilist->name.text() ) )
				break; 
		}

		if( ilist == NULL )
			error( @$, std::string( "unknown netgroup " ) + $2->text() );

		peer->netmap_add( ilist, UNITY_SPLIT_EXCLUDE, NULL );

		delete $2;
	}
	EOS
  |	EXCLUDE LABEL QUOTED
	{
		long index = 0;
		while( true )
		{
			ilist = ( IKE_ILIST * ) iked.list_netgrp.get_item( index++ );
			if( ilist == NULL )
				break;

			if( !strcmp( $2->text(), ilist->name.text() ) )
				break; 
		}

		if( ilist == NULL )
			error( @$, std::string( "unknown netgroup " ) + $2->text() );

		peer->netmap_add( ilist, UNITY_SPLIT_EXCLUDE, $3 );

		delete $2;
		delete $3;
	}
	EOS
  ;

proposal_lines_isakmp
  :	/* nothing */
  |	proposal_lines_isakmp proposal_line_isakmp
  ;
proposal_line_isakmp
  :	AUTH HYB_XA_RSA
	{
		proposal.auth_id = HYBRID_AUTH_INIT_RSA;
	}
	EOS
  |	AUTH MUT_XA_RSA
	{
		proposal.auth_id = XAUTH_AUTH_INIT_RSA;
	}
	EOS
  |	AUTH MUT_XA_PSK
	{
		proposal.auth_id = XAUTH_AUTH_INIT_PSK;
	}
	EOS
  |	AUTH MUT_RSA
	{
		proposal.auth_id = IKE_AUTH_SIG_RSA;
	}
	EOS
  |	AUTH MUT_PSK
	{
		proposal.auth_id = IKE_AUTH_PRESHARED_KEY;
	}
	EOS
  |	CIPH ALG_AES
	{
		proposal.ciph_id = IKE_CIPHER_AES;
	}
	EOS
  |	CIPH ALG_BLOWFISH
	{
		proposal.ciph_id = IKE_CIPHER_BLOWFISH;
	}
	EOS
  |	CIPH ALG_3DES
	{
		proposal.ciph_id = IKE_CIPHER_3DES;
	}
	EOS
  |	CIPH ALG_CAST
	{
		proposal.ciph_id = IKE_CIPHER_CAST;
	}
	EOS
  |	CIPH ALG_DES
	{
		proposal.ciph_id = IKE_CIPHER_DES;
	}
	EOS
  |	KLEN NUMBER
	{
		proposal.ciph_kl = $2;
	}
	EOS
  |	HASH ALG_MD5
	{
		proposal.hash_id = IKE_HASH_MD5;
	}
	EOS
  |	HASH ALG_SHA1
	{
		proposal.hash_id = IKE_HASH_SHA1;
	}
	EOS
  |	DHGR NUMBER
	{
		switch( $2 )
		{
			case 1:
				proposal.dhgr_id = IKE_GRP_GROUP1;
				break;

			case 2:
				proposal.dhgr_id = IKE_GRP_GROUP2;
				break;

			case 5:
				proposal.dhgr_id = IKE_GRP_GROUP5;
				break;

			case 14:
				proposal.dhgr_id = IKE_GRP_GROUP14;
				break;

			case 15:
				proposal.dhgr_id = IKE_GRP_GROUP15;
				break;

			case 16:
				proposal.dhgr_id = IKE_GRP_GROUP16;
				break;

			default:
				error( @$, std::string( "invalid dhgrp id" ) );
				break;
		}
	}
	EOS
  |	LIFE_SEC NUMBER
	{
		proposal.life_sec = $2;
	}
	EOS
  |	LIFE_KBS NUMBER
	{
		proposal.life_kbs = $2;
	}
	EOS
  ;

proposal_lines_ah
  :	/* nothing */
  |	proposal_lines_ah proposal_line_ah
  ;
proposal_line_ah
  :	MSGA ALG_MD5
	{
		proposal.xform = ISAKMP_AH_MD5;
		proposal.hash_id = ISAKMP_AUTH_HMAC_MD5;
	}
	EOS
  |	MSGA ALG_SHA1
	{
		proposal.xform = ISAKMP_AH_SHA;
		proposal.hash_id = ISAKMP_AUTH_HMAC_SHA;
	}
	EOS
  |	DHGR NUMBER
	{
		switch( $2 )
		{
			case 1:
				proposal.dhgr_id = IKE_GRP_GROUP1;
				break;

			case 2:
				proposal.dhgr_id = IKE_GRP_GROUP2;
				break;

			case 5:
				proposal.dhgr_id = IKE_GRP_GROUP5;
				break;

			case 14:
				proposal.dhgr_id = IKE_GRP_GROUP14;
				break;

			case 15:
				proposal.dhgr_id = IKE_GRP_GROUP15;
				break;

			case 16:
				proposal.dhgr_id = IKE_GRP_GROUP16;
				break;

			default:
				error( @$, std::string( "invalid dhgrp id" ) );
				break;
		}
	}
	EOS
  |	LIFE_SEC NUMBER
	{
		proposal.life_sec = $2;
	}
	EOS
  |	LIFE_KBS NUMBER
	{
		proposal.life_kbs = $2;
	}
	EOS
  ;

proposal_lines_esp
  :	/* nothing */
  |	proposal_lines_esp proposal_line_esp
  ;
proposal_line_esp
  :	CIPH ALG_AES
	{
		proposal.xform = ISAKMP_ESP_AES;
	}
	EOS
  |	CIPH ALG_BLOWFISH
	{
		proposal.xform = ISAKMP_ESP_BLOWFISH;
	}
	EOS
  |	CIPH ALG_3DES
	{
		proposal.xform = ISAKMP_ESP_3DES;
	}
	EOS
  |	CIPH ALG_CAST
	{
		proposal.xform = ISAKMP_ESP_CAST;
	}
	EOS
  |	CIPH ALG_DES
	{
		proposal.xform = ISAKMP_ESP_DES;
	}
	EOS
  |	KLEN NUMBER
	{
		proposal.ciph_kl = $2;
	}
	EOS
  |	MSGA ALG_MD5
	{
		proposal.hash_id = ISAKMP_AUTH_HMAC_MD5;
	}
	EOS
  |	MSGA ALG_SHA1
	{
		proposal.hash_id = ISAKMP_AUTH_HMAC_SHA;
	}
	EOS
  |	DHGR NUMBER
	{
		switch( $2 )
		{
			case 1:
				proposal.dhgr_id = IKE_GRP_GROUP1;
				break;

			case 2:
				proposal.dhgr_id = IKE_GRP_GROUP2;
				break;

			case 5:
				proposal.dhgr_id = IKE_GRP_GROUP5;
				break;

			case 14:
				proposal.dhgr_id = IKE_GRP_GROUP14;
				break;

			case 15:
				proposal.dhgr_id = IKE_GRP_GROUP15;
				break;

			case 16:
				proposal.dhgr_id = IKE_GRP_GROUP16;
				break;

			default:
				error( @$, std::string( "invalid dhgrp id" ) );
				break;
		}
	}
	EOS
  |	LIFE_SEC NUMBER
	{
		proposal.life_sec = $2;
	}
	EOS
  |	LIFE_KBS NUMBER
	{
		proposal.life_kbs = $2;
	}
	EOS
  ;

proposal_lines_ipcomp
  :	/* nothing */
  |	proposal_lines_ipcomp proposal_line_ipcomp
  ;
proposal_line_ipcomp
  :	COMP ALG_DEFLATE
	{
		proposal.xform = ISAKMP_IPCOMP_DEFLATE;
	}
	EOS
  |	COMP ALG_LZS
	{
		proposal.xform = ISAKMP_IPCOMP_LZS;
	}
	EOS
  |	LIFE_SEC NUMBER
	{
		proposal.life_sec = $2;
	}
	EOS
  |	LIFE_KBS NUMBER
	{
		proposal.life_kbs = $2;
	}
	EOS
  ;

%%

void yy::conf_parser::error(
	const yy::conf_parser::location_type & l,
        const std::string & m )
{
	iked.log.txt( LOG_ERROR,
		"%s ( line %i, col %i )\n",
		m.c_str(),
		l.end.line,
		l.end.column );

	exit( -1 );

	iked.conf_fail = true;
}
