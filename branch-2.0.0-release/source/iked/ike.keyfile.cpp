
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
// opsenssl version compatibility
//

#if OPENSSL_VERSION_NUMBER < 0x0090800fL
# define X509CONST
#else
# define X509CONST const
#endif

// openssl password callback

int keyfile_cb( char * buf, int size, int rwflag, void * userdata )
{
	BDATA * fpass = ( BDATA * ) userdata;

	if( size > int( fpass->size() ) )
		size = int( fpass->size() );

	memcpy( buf, fpass->buff(), size );

	return 0;
}

void _IKED::load_path( char * file, char * fpath )
{

#ifdef WIN32

	if( PathIsRelative( file ) )
	{
		sprintf( 
			fpath,
			"%s\\certificates\\%s",
			path_ins,
			file );
	}
	else

#endif

	{
		strcpy( fpath, file );
	}
}

bool _IKED::cert_2_bdata( BDATA & cert, X509 * x509 )
{
	int size = i2d_X509( x509, NULL );

	cert.set( 0, size );

	unsigned char * cert_buff = cert.buff();

	size = i2d_X509( x509, &cert_buff );
	if( !size )
		return false;

	return true;
}

bool _IKED::bdata_2_cert( X509 ** x509, BDATA & cert )
{
	X509CONST unsigned char * cert_buff = cert.buff();

	*x509 = d2i_X509( NULL, &cert_buff, cert.size() );
	if( !( *x509 ) )
		return false;

	return true;
}

long _IKED::cert_load_pem( BDATA & cert, char * file, bool ca, BDATA & pass )
{
	char fpath[ MAX_PATH ];
	load_path( file, fpath );

	FILE * fp = fopen( fpath, "rb" );
	if( !fp )
		return FILE_FAIL;

	X509 * x509 = PEM_read_X509( fp, NULL, NULL, NULL );

	if( pass.size() && ( x509 == NULL ) )
		x509 = PEM_read_X509( fp, NULL, keyfile_cb, &pass );
	
	fclose( fp );

	if( x509 == NULL )
		return FILE_PASS;

	cert_2_bdata( cert, x509 );

	X509_free( x509 );

	return FILE_OK;
}

long _IKED::cert_load_p12( BDATA & cert, char * file, bool ca, BDATA & pass )
{
	char fpath[ MAX_PATH ];
	load_path( file, fpath );

	FILE * fp = fopen( fpath, "rb" );
	if( !fp )
		return FILE_FAIL;

	PKCS12 * p12 = d2i_PKCS12_fp( fp, NULL );

	fclose( fp );

	if( p12 == NULL )
		return FILE_FAIL;

	X509 * x509 = NULL;

	if( ca )
	{
		STACK_OF( X509 ) * stack = NULL;

		if( PKCS12_parse( p12, NULL, NULL, NULL, &stack ) )
		{
			if( stack != NULL )
			{
				if( sk_X509_value( stack, 0 ) != NULL )
					x509 = sk_X509_value( stack, 0 );

				sk_X509_free( stack );
			}
		}

		stack = NULL;

		if( pass.size() && ( x509 == NULL ) )
		{
			if( PKCS12_parse( p12, ( const char * ) pass.buff(), NULL, NULL, &stack ) )
			{
				if( stack != NULL )
				{
					if( sk_X509_value( stack, 0 ) != NULL )
						x509 = sk_X509_value( stack, 0 );

					sk_X509_free( stack );
				}
			}
		}
	}
	else
	{
		PKCS12_parse( p12, NULL, NULL, &x509, NULL );

		if( pass.size() && ( x509 == NULL ) )
			PKCS12_parse( p12, ( const char * ) pass.buff(), NULL, &x509, NULL );
	}

	PKCS12_free( p12 );

	if( x509 == NULL )
		return FILE_PASS;

	cert_2_bdata( cert, x509 );
	X509_free( x509 );

	return FILE_OK;
}

long _IKED::cert_save( char * file, BDATA & cert )
{
	char fpath[ MAX_PATH ];
	load_path( file, fpath );

	X509 * x509;
	if( !bdata_2_cert( &x509, cert ) )
		return FILE_FAIL;

	FILE * fp = fopen( fpath, "wb" );
	if( !fp )
		return FILE_FAIL;
	
	PEM_write_X509( fp, x509 );

	fclose( fp );

	X509_free( x509 );

	return FILE_OK;
}

bool _IKED::cert_desc( BDATA & cert, BDATA & desc )
{
	X509 * x509;
	if( !bdata_2_cert( &x509, cert ) )
		return false;

	BIO * bio = BIO_new( BIO_s_mem() );
	if( bio == NULL )
	{
		X509_free( x509 );
		return false;
	}

	if( X509_print( bio, x509 ) != 1 )
	{
		BIO_free( bio );
		X509_free( x509 );
		return false;
	}

	unsigned char * bio_buff = NULL;

	int size = BIO_get_mem_data( bio, &bio_buff );

	desc.set( 0, size + 1 );
	memcpy( desc.buff(), bio_buff, size );

	BIO_free( bio );
	X509_free( x509 );

	return false;
}

bool _IKED::cert_subj( BDATA & cert, BDATA & subj )
{
	X509 * x509;
	if( !bdata_2_cert( &x509, cert ) )
		return false;

	X509_NAME * x509_name = X509_get_subject_name( x509 );
	if( x509_name == NULL )
	{
		X509_free( x509 );
		return false;
	}

	short size = i2d_X509_NAME( x509_name, NULL );

	if( size > LIBIKE_MAX_VARID )
	{
		X509_free( x509 );
		return false;
	}

	subj.set( 0, size );
	unsigned char * temp = subj.buff();
	
	size = i2d_X509_NAME( x509_name, &temp );

	X509_free( x509 );

	log.bin( LOG_DEBUG, LOG_DECODE,
		subj.buff(),
		subj.size(),
		"ii : obtained x509 cert subject" );

	return true;
}

bool _IKED::asn1_text( BDATA & data, BDATA & text )
{
	X509_NAME * x509_name = NULL;

	X509CONST unsigned char * buff = data.buff();
	if( buff == NULL )
		return false;

	d2i_X509_NAME( &x509_name, &buff, data.size() );

	if( x509_name == NULL )
		return false;

	char name[ 512 ];
	X509_NAME_oneline(
		x509_name,
		name,
		512 );

	text.set( name, strlen( name ) );

	X509_NAME_free( x509_name );

	return true;
}

bool _IKED::text_asn1( BDATA & text, BDATA & asn1 )
{
	BDATA temp;

	//
	// create a copy of our text
	// and reset the output buff
	//

	temp.set( text );
	temp.add( 0, 1 );
	asn1.del( true );
        
	X509_NAME * name = X509_NAME_new();

	unsigned char *	fbuff = NULL;

	char *	tbuff = ( char * ) temp.buff();
	long	tsize = 0;
	long	tnext = 0;

	char *	field = NULL;
	long	fsize = 0;

	char *	value = NULL;
	long	vsize = 0;

	bool	pair = false;
	bool	stop = false;

	while( !stop )
	{
		//
		// obtain the length of
		// the current segment
		//

		tsize = strcspn( tbuff, ",/=" );

		//
		// check for null length
		//

		if( !tsize )
		{
			tbuff++;
			continue;
		}

		//
		// check the delimiter type
		//

		switch( tbuff[ tsize ] )
		{
			//
			// are we delimiting between a
			// field and value or between
			// a field value pair
			//

			case '=':
			{
				if( field == NULL )
					field = tbuff;

				fsize += tsize;

				break;
			}

			case ',':
			case '/':
			case '\0':
			{
				if( field == NULL )
					goto text_asn1_failed;

				if( value == NULL )
					value = tbuff;

				vsize += tsize;

				if( !value[ vsize + 1 ] )
				{
					pair = true;
					stop = true;
					break;
				}

				tnext = strcspn( tbuff + tsize + 1, ",/=" );

				switch( value[ vsize + tnext + 1 ] )
				{
					case ',':
					case '/':
					case '\0':
						vsize++;
						break;

					default:
						pair = true;
						break;
				}
			}
		}

		//
		// check for field value pair
		//

		if( pair )
		{
			//
			// trim pair
			//

			while( ( field[ 0 ] == ' ' ) && fsize )
			{
				field++;
				fsize--;
			}

			while( ( field[ fsize - 1 ] == ' ' ) && fsize )
				fsize--;

			while( ( value[ 0 ] == ' ' ) && vsize )
			{
				value++;
				vsize--;
			}

			while( ( value[ vsize - 1 ] == ' ' ) && vsize )
				vsize--;

			//
			// null terminate
			//

			field[ fsize ] = 0;
			value[ vsize ] = 0;

			//
			// add the pair
			//

			X509_NAME_add_entry_by_txt(
				name,
				field,
				MBSTRING_ASC,
				( unsigned char * ) value,
				-1,	-1, 0 );

			log.txt( LOG_DECODE,
				"ii : asn1_text %s = %s\n",
				field,
				value );

			//
			// cleanup for next pair
			//

			field = NULL;
			fsize = 0;
			value = NULL;
			vsize = 0;

			pair = false;
		}

		tbuff += ( tsize + 1 );
	}

	//
	// copy to buffer
	//

	tsize = i2d_X509_NAME( name, NULL );
	if( !tsize )
		goto text_asn1_failed;

	asn1.set( 0, tsize );
	fbuff = asn1.buff();

    tsize = i2d_X509_NAME( name, &fbuff );
	if( !tsize )
		goto text_asn1_failed;

	//
	// return success
	//

	X509_NAME_free( name );

	return true;

	//
	// return failure
	//

	text_asn1_failed:

	X509_NAME_free( name );

	return false;
}

static int verify_cb( int ok, X509_STORE_CTX * store_ctx )
{
	if( !ok )
	{
		long ll = LOG_ERROR;
		char name[ 512 ];

		X509_NAME * x509_name = X509_get_subject_name( store_ctx->current_cert );

		X509_NAME_oneline(
			x509_name,
			name,
			512 );

		switch( store_ctx->error )
		{
			case X509_V_ERR_UNABLE_TO_GET_CRL:
				ok = 1;
				ll = LOG_INFO;
				break;
		}

		iked.log.txt(
			ll,
			"ii : %s(%d) at depth:%d\n"
			"ii : subject :%s\n",
			X509_verify_cert_error_string( store_ctx->error ),
			store_ctx->error,
			store_ctx->error_depth,
			name );
	}

	ERR_clear_error();

	return ok;
}

bool _IKED::cert_verify( BDATA & cert, BDATA & ca )
{
	char fpath[ MAX_PATH ];
	sprintf( fpath, "%s\\debug\\remote.crt", path_ins );
	cert_save( fpath, cert );

	X509 * x509_ca;
	if( !bdata_2_cert( &x509_ca, ca ) )
		return false;

	X509 * x509_cert;
	if( !bdata_2_cert( &x509_cert, cert ) )
	{
		X509_free( x509_ca );
		return false;
	}

	X509_STORE * store;
	store = X509_STORE_new();
	if( store == NULL )
	{
		X509_free( x509_cert );
		X509_free( x509_ca );
		return false;
	}

	X509_STORE_set_verify_cb_func( store, verify_cb );
	X509_STORE_add_cert( store, x509_ca );

	X509_LOOKUP * lookup = X509_STORE_add_lookup( store, X509_LOOKUP_file() );
	if( lookup == NULL )
	{
		X509_STORE_free( store );
		X509_free( x509_cert );
		X509_free( x509_ca );
		return false;
	}

#ifdef WIN32

	char tmppath[ MAX_PATH ];
	snprintf( tmppath, MAX_PATH, "%s\\certificates\\*.*", path_ins );

	WIN32_FIND_DATA ffd;
	memset( &ffd, 0, sizeof( ffd ) );
	ffd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	HANDLE hff = FindFirstFile(
					tmppath,
					&ffd );

	while( hff != NULL )
	{
		snprintf( tmppath, MAX_PATH, "%s\\certificates\\%s", path_ins, ffd.cFileName );

		if( X509_LOOKUP_load_file( lookup, tmppath, X509_FILETYPE_PEM ) != NULL )
			log.txt( LOG_DEBUG, "ii : added %s to x509 store\n", ffd.cFileName );

		if( !FindNextFile( hff, &ffd ) )
			hff = NULL;
	}

#endif

	X509_STORE_CTX * store_ctx;
	store_ctx = X509_STORE_CTX_new();
	if( store_ctx == NULL )
	{
		X509_STORE_free( store );
		X509_free( x509_cert );
		X509_free( x509_ca );
		return false;
	}

	X509_STORE_CTX_init( store_ctx, store, x509_cert, NULL );
	X509_STORE_CTX_set_flags( store_ctx, X509_V_FLAG_CRL_CHECK );
	X509_STORE_CTX_set_flags( store_ctx, X509_V_FLAG_CRL_CHECK_ALL );

	long result = X509_verify_cert( store_ctx );

	X509_STORE_CTX_cleanup( store_ctx );
	X509_STORE_free( store );
	X509_free( x509_cert );
	X509_free( x509_ca );

	if( result )
		return true;

	return false;
}

long _IKED::prvkey_rsa_load_pem( char * file, EVP_PKEY ** evp_pkey, BDATA & pass )
{
	char fpath[ MAX_PATH ];
	load_path( file, fpath );

	FILE * fp = fopen( fpath, "rb" );
	if( !fp )
		return FILE_FAIL;

	*evp_pkey = PEM_read_PrivateKey( fp, NULL, NULL, NULL );

	if( pass.size() && ( *evp_pkey == NULL ) )
		*evp_pkey = PEM_read_PrivateKey( fp, NULL, keyfile_cb, &pass );

	fclose( fp );

	if( *evp_pkey == NULL )
		return FILE_PASS;

	return FILE_OK;
}

long _IKED::prvkey_rsa_load_p12( char * file, EVP_PKEY ** evp_pkey, BDATA & pass )
{
	char fpath[ MAX_PATH ];
	load_path( file, fpath );

	FILE * fp = fopen( fpath, "rb" );
	if( !fp )
		return FILE_FAIL;

	PKCS12 * p12 = d2i_PKCS12_fp( fp, NULL );

	fclose( fp );

	if( p12 == NULL )
		return FILE_FAIL;

	PKCS12_parse( p12, NULL, evp_pkey, NULL, NULL );

	if( pass.size() && ( *evp_pkey == NULL ) )
		PKCS12_parse( p12, ( const char * ) pass.buff(), evp_pkey, NULL, NULL );

	PKCS12_free( p12 );

	if( *evp_pkey == NULL )
		return FILE_PASS;

	return FILE_OK;

}

bool _IKED::pubkey_rsa_read( BDATA & cert, EVP_PKEY ** evp_pkey )
{
	X509 * x509;
	if( !bdata_2_cert( &x509, cert ) )
		return false;

	*evp_pkey = X509_get_pubkey( x509 );

	X509_free( x509 );

	if( !( *evp_pkey ) )
		return false;

	return true;
}

bool _IKED::prvkey_rsa_encrypt( EVP_PKEY * evp_pkey, BDATA & data )
{
	int size = RSA_size( evp_pkey->pkey.rsa );

	BDATA sign;
	sign.set( 0, size );

	size = RSA_private_encrypt(
				data.size(),
				data.buff(),
				sign.buff(),
				evp_pkey->pkey.rsa,
				RSA_PKCS1_PADDING );

	if( size == -1 )
		return false;

	data.set( sign );

	return true;
}

bool _IKED::pubkey_rsa_decrypt( EVP_PKEY * evp_pkey, BDATA & sign )
{
	int size = RSA_size( evp_pkey->pkey.rsa );

	BDATA data;
	data.set( 0, size );

	size = RSA_public_decrypt(
				sign.size(),
				sign.buff(),
				data.buff(),
				evp_pkey->pkey.rsa,
				RSA_PKCS1_PADDING );

	if( size == -1 )
		return false;

	sign.set( data );

	return true;
}
