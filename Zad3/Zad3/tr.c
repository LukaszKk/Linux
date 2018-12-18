#include <stdio.h>
#include <unistd.h>
#include <string.h>


int main( int argc, char* argv[] )
{
	//input check
	if( argc != 2 )
	{
		printf( "Usage: %s <char>\n", argv[0] );
		return -1;
	}
	
	//read stdin
	char c;
	char* sc;
	int flag = 0;
	while( read( STDIN_FILENO, &c, 1 ) > 0 )
	{
		//if c in argv[1] then write
		sc = argv[1];
		for( int i = 0; i < strlen(argv[1]) && *sc != '\0'; i++, sc++ )
		{
			if( c == *sc )
			{
				flag = 1;
				break;
			}
		}
		if( !flag )
			write( STDOUT_FILENO, &c, 1 );
		flag = 0;
	}


	return 0;
}
