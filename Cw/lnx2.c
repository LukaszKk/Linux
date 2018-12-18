#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NANOS 1000000000L

int main( int argc, char * argv[] )
{
	if( argc != 2 )
	{
		printf( "Usage: %s <float>\n", argv[0] );
		return -1;
	}
	char * zm;
	double N = strtod( argv[1], &zm );
	if( *zm != '\0' )
	{
		printf( "Wrong float in %s at %ld\n", argv[0], zm - argv[0] + 1 );
		return -1;
	}
	long sec = (long)(N*NANOS)>>3;
	struct timespec ts = {
		.tv_sec = sec/NANOS,
		.tv_nsec = sec*NANOS
	};
	for( int i = 0; i < 20; i++ )
	{
		if( fork() == 0 )
		{
			nanosleep( &ts, NULL );
			printf( "Child %d\n", i );
			exit( 2 );
		}
	}
	int status;
	pid_t pid;
	while( pid != -1 )
	{
		pid = wait( &status );
		if( WIFEXITED( status ) )
			printf( "Died %d\n", pid );
	}

	return 0;
}
