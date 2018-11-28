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
	if( raise( SIGSTOP ) != 0 )						//wyslanie sygnalu do siebie
		return -1;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = strtof(argv[1], NULL) * 10000000;
	while( 1 )
	{
		if( nanosleep( &ts, 0 ) == -1 )					//spoczynek
			return -1;
		int i = 1 + rand() % 3;			
		switch( i )						//losowanie sygnalu i wysalnie do siebie
		{
		case 1: if( raise( SIGSTOP ) != 0) return -1; break;
		case 2: if( raise( SIGTSTP ) != 0) return -1; break;
		case 3: if( raise( SIGTERM ) != 0) return -1; break;
		}
	}

	return 0;
}
