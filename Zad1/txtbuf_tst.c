#include <stdio.h>
#include "libtxtbuf.h"

char B0[12], B1[12], B2[14];

int main()
{
	char * tab[] = { "ala", "ma", "kota", NULL };

	fillbuf( B0, 12, tab, 2 );
	printf( "buf='%s'\n", B0 );
	fillbuf( B1, 12, tab, 3 );
	printf( "buf='%s'\n", B1 );
	fillbuf( B2, 12, tab, 1 );
	printf( "buf='%s',\n", B2 );
}
