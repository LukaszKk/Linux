#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>


int main( int argc, char* argv[] )
{
	if( argc != 2 )
	{
		printf( "Usage: %s <float>\n", argv[0] );
		return 1;
	}
	srand(time(NULL));
	raise( SIGSTOP );
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = strtof(argv[1], NULL) * 10000000;
	while( 1 )
	{
		nanosleep( &ts, 0 );
		int i = 1 + rand() % 3;
		switch( i )
		{
		case 1: raise( SIGSTOP ); break;
		case 2: raise( SIGTSTP ); break;
		case 3: raise( SIGTERM ); break;
		}
	}

	return 0;
}
