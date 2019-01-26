#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

void errExit( char* mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

int InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr );
int randomizeL( unsigned int L1, unsigned int L2 );
double randomizeD( double D1, double D2 );

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
	int cnt;
	double r;
	int port;
	char addr[16] = { '\0' };
	int flags = InputCheck( argc, argv, &cnt, &r, &port, addr );

	printf( "flags: %d cnt: %d, r: %f, port: %d, addr: %s\n", flags, cnt, r, port, addr );

	return 0;
}

//===================================================================
//===================================================================

int InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr )
{
	if( argc < 4 || argc > 8 )
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
		case '#': tmpcnt = optarg; break;
		default: break;
		}
	}

	//port and addr
	int flag = 0, flag2 = 0;
	double mult = 10;
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
	
	//cnt
	flag = 0;
	unsigned int L1 = 0, L2 = 0;
	for( int i = 0; i < strlen(tmpcnt); i++ )
	{
		if( tmpcnt[i] == ':' )
		{
			flag = 1;
			continue;
		}
		if( !flag )
			L1 = L1*mult + (tmpcnt[i]-'0');
		else
			L2 = L2*mult + (tmpcnt[i]-'0');
	}
	
	if( L2 <= L1 )
		*cnt = L1;
	else
		*cnt = randomizeL( L1, L2 );
	
	//r/s
	int flags = 0;
	if( tmpr == NULL )
	{
		flags = 1;
		tmpr = tmps;
	}

	flag = 0;
	flag2 = 0;
	double D1 = 0, D2 = 0;
	for( int i = 0; i < strlen(tmpr); i++ )
	{
		if( tmpr[i] == ':' )
		{
			flag = 1;
			flag2 = 0;
			mult = 10;
			continue;
		}
		if( tmpr[i] == '.' )
		{
			flag2 = 1;
			mult = 0.1;
			continue;
		}

		if( !flag )
		{
			if( !flag2 )
				D1 = D1*mult + (tmpr[i]-'0');
			else
			{
				D1 = D1 + (tmpr[i]-'0')*mult;
				mult *= 0.1;
			}
		}
		else
		{
			if( !flag2 )
				D2 = D2*mult + (tmpr[i]-'0');
			else
			{
				D2 = D2 + (tmpr[i]-'0')*mult;
				mult *= 0.1;
			}
		}
	}
	
	if( D2 <= D1 )
		*r = D1;
	else
		*r = randomizeD( D1, D2 );

	return flags;
}

int randomizeL( unsigned int L1, unsigned int L2 )
{
	srand(time(NULL));
	return rand()%L2 + L1;
}

double randomizeD( double D1, double D2 )
{
	srand(time(NULL));
	double range = D2 - D1;
	double div = RAND_MAX / range;
	return D1 + (double)(rand()/div);
}

//===================================================================

