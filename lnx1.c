#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int fileLen( int file )
{
	int len = 0;
	int c = 0;
	while( 1 )
	{
		if( (char)c == '\n' )
			len++;
		if( read( file, &c, 1 ) == 0 )
			break;
	}
	return len;
}

int main( int argc, char* argv[] )
{
	if( argc < 2 || argc > 3 )
	{
		printf( "Usage: <./%s> <integer> [<input file>]\n", argv[0] );
		return -1;
	}
	int file;
	char name[12];
	if( argc == 2 )
		strcpy( name, "outFile.txt" );
	else
		strcpy( name, argv[2] );
	file = open( name, O_CREAT | O_RDWR );
	int x = fileLen( file );
	write( file, "\033[3;3H", 1 );
	write( file, "myk", 3 );

	close( file );
	return 0;
}
