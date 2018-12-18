#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

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
	pid_t pid, pid2, pid3[20];
	if( (pid = fork()) == 0 )
	{
		if( (pid2 = setpgrp()) == 0 )
		{
			long sec = (long)(N*NANOS)>>3;
			struct timespec ts = {
				.tv_sec = sec/NANOS,
				.tv_nsec = sec*NANOS
			};
			for( int i = 0; i < 20; i++ )
			{
				if( (pid3[i] = fork()) == 0 )
				{
					nanosleep( &ts, NULL );
					printf( "Child %d\n", i );
					exit( 2 );
				}
			}
		}
	}
	int status;
	waitpid( pid, &status, 0 );
	if( WIFEXITED( status ) )
	{
		kill( -pid2, SIGKILL );
	}
	for( int i = 0; i < 20; i++ )
	{
		kill( pid3[i], 0 );
		if( errno != ESRCH )
			printf( "%d lives...\n", pid3[i] );
	}

	return 0;
}
