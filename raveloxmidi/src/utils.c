/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <errno.h>
extern int errno;

#include "config.h"
#include "logging.h"

extern int errno;

uint64_t ntohll(const uint64_t value)
{
	enum { TYP_INIT, TYP_SMLE, TYP_BIGE };
      	union
	{
		uint64_t ull;
		uint8_t  c[8];
	} x;

	// Test if on Big Endian system.
	static int typ = TYP_INIT;

	if (typ == TYP_INIT)
	{
		x.ull = 0x01;
		typ = (x.c[7] == 0x01) ? TYP_BIGE : TYP_SMLE;
	}

	// System is Big Endian; return value as is.
	if (typ == TYP_BIGE)
	{
		return value;
	}

	// else convert value to Big Endian
	x.ull = value;
	int8_t c = 0;
	c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
	c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
	c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
	c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
	return x.ull;
}

uint64_t htonll(const uint64_t value)
{
	return ntohll(value);
}

void get_uint16( void *dest, unsigned char **src, size_t *len )
{
	uint16_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint16_t) );
	*src += sizeof( uint16_t );
	*len -= sizeof( uint16_t );
	value = ntohs( value );
	memcpy( dest, &value, sizeof(uint16_t) );
}

void put_uint16( unsigned char **dest, uint16_t src, size_t *len )
{
	uint16_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htons( src );
	memcpy( *dest, &value, sizeof(uint16_t) );
	*dest += sizeof( uint16_t );
	*len += sizeof( uint16_t );
}

void put_uint32( unsigned char **dest, uint32_t src, size_t *len )
{
	uint32_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htonl( src );
	memcpy( *dest, &value, sizeof(uint32_t) );
	*dest += sizeof( uint32_t );
	*len += sizeof( uint32_t );
}

void put_uint64( unsigned char **dest, uint64_t src, size_t *len )
{
	uint64_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htonll( src );
	memcpy( *dest, &value, sizeof(uint64_t) );
	*dest += sizeof( uint64_t );
	*len += sizeof( uint64_t );
}

void get_uint32( void *dest, unsigned char **src, size_t *len )
{
	uint32_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint32_t) );
	*src += sizeof( uint32_t );
	*len -= sizeof( uint32_t );
	value = ntohl( value );
	memcpy( dest, &value, sizeof(uint32_t) );
}

void get_uint64( void *dest, unsigned char **src, size_t *len )
{
	uint64_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint64_t) );
	*src += sizeof( uint64_t );
	*len -= sizeof( uint64_t );
	value = ntohll( value );
	memcpy( dest, &value, sizeof(uint64_t) );
}

void hex_dump( unsigned char *buffer, size_t len )
{
	size_t i = 0 ;
	unsigned char c = 0 ;

/* Only do a hex dump at debug level */
	DEBUG_ONLY;

	logging_prefix_disable();

	logging_printf(LOGGING_DEBUG, "hexdump(%p , %u)\n", buffer, len );
	if( ! buffer ) return;
	if( len <= 0 ) return;

	for( i = 0 ; i < len ; i++ )
	{
		if( i % 8 == 0 )
		{
			logging_printf( LOGGING_DEBUG, "\n");
		}
		c = buffer[i];
		logging_printf(LOGGING_DEBUG, "%02x %c\t", c, isprint(c)  ? c : '.' );
	}
	logging_printf(LOGGING_DEBUG, "\n");
	logging_printf(LOGGING_DEBUG, "-- end hexdump\n");

	logging_prefix_enable();
}

void FREENULL( const char *description, void **ptr )
{
	if( ! ptr ) return;
	if( ! *ptr ) return;

	logging_printf(LOGGING_DEBUG,"FREENULL: description=\"%s\",ptr=%p\n", description, *ptr);
	free( *ptr );
	*ptr = NULL;
}

int check_file_security( const char *filepath )
{
	struct stat buf;

	if( ! filepath ) return 0;
	
	if( stat( filepath, &buf ) != 0 )
	{
		if( errno == ENOENT )
		{
			return 1;
		}
		logging_printf(LOGGING_ERROR, "stat() error: %s\t%s\n", errno, filepath, strerror( errno) );
		return 0;
	}

	if( ! S_ISREG( buf.st_mode ) || ( buf.st_mode & ( S_IXUSR | S_IXGRP | S_IXOTH) ) )
	{
		return 0;
	}
	
	return 1;
}

int is_yes( const char *value )
{
	if( ! value ) return 0;
	if( strcasecmp( value, "yes" ) == 0 ) return 1;
	if( strcasecmp( value, "y" ) == 0 ) return 1;
	if( strcasecmp( value, "1" ) == 0 ) return 1;

	return 0;
}

int is_no( const char *value )
{
	if( ! value ) return 0;
	if( strcasecmp( value, "no" ) == 0 ) return 1;
	if( strcasecmp( value, "n" ) == 0 ) return 1;
	if( strcasecmp( value, "0" ) == 0 ) return 1;

	return 0;
}

char *get_ip_string( struct sockaddr *sa, char *s, size_t maxlen )
{
        switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;  
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;  
        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
        }       
        return s;
}

int get_sock_addr( char *ip_address, int port, struct sockaddr *socket, socklen_t *socklen)
{
        struct addrinfo hints;
        struct addrinfo *result;
        int val;
        char portaddr[32];
	char tmp_address[ INET6_ADDRSTRLEN ];

        if( ! ip_address ) return 1;
	if( ! socket ) return 1;

        memset( &hints, 0, sizeof( hints ) );
        memset( portaddr, 0, sizeof( portaddr ) );
        sprintf( portaddr, "%d", port ); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        val = getaddrinfo(ip_address, portaddr, &hints, &result );
	if( val )
	{
		logging_printf(LOGGING_WARN, "get_sock_addr: Invalid address: [%s]:%d\n", ip_address, port );
		freeaddrinfo( result );
		return 1;
	}

	memset( socket, 0, sizeof( struct sockaddr ) );
	memcpy( socket, result->ai_addr, result->ai_addrlen );
	*socklen = result->ai_addrlen;

	freeaddrinfo( result );
	return 0;
}

int get_addr_family(char *ip_address, int port)
{
        struct addrinfo hints;
        struct addrinfo *result;
        int val;
        char portaddr[32];
	char tmp_address[ INET6_ADDRSTRLEN ];
	int family = AF_UNSPEC;

        if( ! ip_address ) return AF_UNSPEC;

        memset( &hints, 0, sizeof( hints ) );
        memset( portaddr, 0, sizeof( portaddr ) );
        sprintf( portaddr, "%d", port ); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        val = getaddrinfo(ip_address, portaddr, &hints, &result );
	if( val )
	{
		logging_printf(LOGGING_WARN, "get_addr_family: Invalid address: [%s]:%d\n", ip_address, port );
		freeaddrinfo( result );
		return AF_UNSPEC;
	}

	family = result->ai_family;
	freeaddrinfo( result );
	return family;

}
