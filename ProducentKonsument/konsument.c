#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>



void errExit( char* mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

void InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr );

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
	int cnt;
	double r;
	int port;
	char addr[16] = { '\0' };
	InputCheck( argc, argv, &cnt, &r, &port, addr );

	printf( "cnt: %d, r: %f, port: %d, addr: %s\n", cnt, r, port, addr );

	return 0;
}

//===================================================================
//===================================================================

void InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr )
{
	if( argc != 6 && argc != 8 )
	{
		printf( "Usage: %s -# <cnt> -r <dly> | -s <dly> [<addr>:]port\n", argv[0] );
		errExit( "Input Error" );
	}

	int opt;
	char* tmps;
	char* tmpr;
	char* tmpcnt;
	while( (opt=getopt(argc, argv, "#:r:s:")) != -1 )
	{
		switch( opt )
		{
		case 's': tmps = optarg; break;
		case 'r': tmpr = optarg; break;
		case '#': *cnt = strtol( optarg, NULL, 10 ); break;
		default: break;
		}
	}
	
	//port and addr
	int flag = 0, flag2 = 0;
	int mult = 10;
	*port = 0;
	for( int i = 0; i < strlen(argv[argc-1]); i++ )
	{
		if( argv[argc-1][i] == ':' )
		{
			flag = 1;
			continue;
		}
		if( !flag )
			addr[i] = argv[argc-1][i];
		else
		{
			flag2 = 1;
			*port = *port*mult + (argv[argc-1][i]-'0');
		}
	}
	if( !flag2 )
	{
		*port = strtol( addr, NULL, 10 );
		strcpy( addr, "localhost" );
	}
	if( strlen(addr) > 15 )
		errExit( "adress error" );
}

//===================================================================
