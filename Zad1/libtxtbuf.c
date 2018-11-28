#include <string.h>
#include "libtxtbuf.h"

int fillbuf( char * buf, int blen, char * txts[], int splen )
{
	int count = 0;
	for( char ** str = txts; *str; str++ )
	{
		int slen = strlen( *str );
		if( slen > blen ) return count;
		strncpy( buf, *str, slen );
		buf += slen;
		blen -= slen;
		count += slen;
		for( int sp = 0; sp < splen && blen > 0; sp++, blen--, count++ )
			*(buf++) = ' ';
	}
	return count;
}
