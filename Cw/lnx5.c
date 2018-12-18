#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

volatile int flag = 1;
struct timespec t;
void Handler( int sig_num );

int main( int argc, char* argv[] )
{
	if( argc != 3 )
	{
		printf( "Usage: %s -x <int>\n", argv[0] );
		return -1;
	}
	int opt;
	int N;
	while( (opt=getopt(argc,argv,"x:d:")) != -1 )
	{
		switch(opt)
		{
		case 'x': N=strtol(optarg, NULL, 10); break;
		}
	}
	
	struct sigaction sa1;
	struct sigaction sa2;
	sa1.sa_handler = SIG_IGN;
	sa2.sa_handler = &Handler;
	sigaction( SIGUSR1, &sa1, NULL );
	sigaction( SIGUSR2, &sa2, NULL );
	while( N > 0 )
	{
		if( flag == 0 )
		{
			printf( "time; %ld\n", t.tv_sec*1000000 + t.tv_nsec );
			N--;
			flag = 1;
		}
	}


	return 0;
}


void Handler( int sig_num )
{
	if( flag == 1 )
	{
		if( clock_gettime( CLOCK_REALTIME, &t ) == -1 )
			exit( 0 );
		flag = 0;
	}
}
