#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define MAX 1250000

void errExit( char * mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

char* InputCheck( int argc, char* argv[], char** r, double* t, int* port );
timer_t create_timer( double t );
void handler( int signal );
volatile int generate_data = 0;


int main( int argc, char* argv[] )
{
	char* r;
	double t;
	int port;
	char* addr;
	addr = InputCheck( argc, argv, &r, &t, &port );
	
	char* magazine = malloc( MAX );
	int magindex = 0;

	struct sigaction sa;
	sa.sa_handler = handler;
	if( sigaction(SIGCHLD, &sa, NULL) == -1 )
		errExit( "Sigaction error" );
	
	timer_t timerid = create_timer( t );

	int i = 65;
	while( 1 )
	{
		if( generate_data )
		{
			for( int j = 0; j < 640; j++, magindex++ )
			{
				if( magindex >= MAX-1 )
					break;
				magazine[magindex] = i;
			}
			if( magindex < MAX-1 )
				magindex++;
			i++;
			generate_data = 0;
		}

		if( i == 122 ) i = 64;
		else if( i == 90 ) i = 96;
		if( magindex >= 640*3 )
			break;
	}
	

	if( timer_delete( timerid ) == -1 )
		errExit( "timer_delete error" );
	free( magazine );

	return 0;
}


char* InputCheck( int argc, char* argv[], char** r, double* t, int* port )
{
	if( argc != 6 )
	{
		printf( "Usage: %s -r <path> -t <val> [<addr>:]port\n", argv[0] );
		errExit( "Input Error" );
	}

	int opt;
	while( (opt=getopt(argc, argv, "r:t:")) != -1 )
	{
		switch( opt )
		{
		case 'r': *r = optarg; break;
		case 't': *t = strtof( optarg, NULL ); break;
		}
	}
	if( *t < 1 || *t > 8 )
		errExit( "Param T must be between 1 and 8" );
	*t = *t/96;
	
	int flag = 0, flag2 = 0;
	int mult = 10;
	*port = 0;
	char* addr = malloc(50);
	for( int i = 0; i < strlen(argv[5]); i++ )
	{
		if( argv[5][i] == ':' )
		{
			flag = 1;
			continue;
		}
		if( !flag )
			addr[i] = argv[5][i];
		else
		{
			flag2 = 1;
			*port = *port*mult + (argv[5][i]-'0');
		}
	}
	if( !flag2 )
	{
		*port = strtol( addr, NULL, 10 );
		strcpy( addr, "localhost" );
	}

	return addr;
}

timer_t create_timer( double t )
{
	struct sigevent sev;
	timer_t timerid;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGCHLD;
	if( timer_create(CLOCK_REALTIME, &sev, &timerid) == -1 )
		errExit( "timer_create error" );

	struct itimerspec its;
	its.it_value.tv_sec = t*100;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = t * 100;
	its.it_interval.tv_nsec = 0;

	if( timer_settime(timerid, 0, &its, NULL) == -1 )
		errExit( "timer_settime error" );

	return timerid;
}


void handler( int signal )
{
	generate_data = 1;
}

