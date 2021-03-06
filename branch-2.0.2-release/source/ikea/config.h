
/*
 * Copyright (c) 2007
 *      Shrew Soft Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is strictly prohibited. The copywright holder of this
 * software is the sole owner and no other party should have access
 * unless explicit permission was granted by an authorized person.
 *
 * AUTHOR : Matthew Grooms
 *          mgrooms@shrew.net
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "utils.list.h"

#define MAX_CONFSTRING		128
#define MAX_CONFBINARY		512

#define DATA_STRING		1
#define DATA_NUMBER		2
#define DATA_BINARY		3

#define CONFIG_OK		0
#define CONFIG_FAILED	1
#define CONFIG_CANCEL	2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef class _CFGDAT
{
	friend class _CONFIG;
	
protected:
	
	const char *	key;
	long		type;
	
	union
	{
		const char *	sval;
		char *		bval;
		long		nval;
	};
	
	long size;
	
	_CFGDAT();
	
}CFGDAT;

typedef class DLX _CONFIG
{
	protected:
	
	const char * id;
	LIST	data;
	
	CFGDAT *	get_data( long type, const char * key, bool add = false );
	
	public:
	
	_CONFIG();
	~_CONFIG();
	
	_CONFIG & operator = ( _CONFIG & value );
	
	bool	file_read( char * path );
	bool	file_write( char * path );
	
	bool	set_id( const char * id );
	const char *	get_id();
	
	void	del( const char * key );
	void	del_all();
	
	bool	add_string( const char * key, const char * val, int size );
	bool	set_string( const char * key, const char * val, int size );
	long	has_string( const char * key, const char * val, int size );
	bool	get_string( const char * key, char * val, int size, int index );
	
	bool	set_number( const char * key, long val );
	bool	get_number( const char * key, long * val );
	
	bool	set_binary( const char * key, char * val, long size );
	bool	get_binary( const char * key, char * val, long size );
	
}CONFIG;

#endif
