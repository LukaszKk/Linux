#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

void errExit( char * s )
{
	write( STDERR_FILENO, s, strlen(s) );
	exit( EXIT_FAILURE );
}

void Handler( int );

int main( int argc, char* argv[] )
{	
	//sigusr2 - do nothing
	struct sigaction act2;
	act2.sa_handler = &Handler;
	if( sigaction( SIGUSR2, &act2, NULL ) == -1 )
		errExit( "sigaction_err" );

	//input check
	if( argc != 2 && argc != 4 )
	{
		printf( "Usage: %s [-d <length>] <text>\n", argv[0] );
		errExit( "Input error" );
	}

	//param -d (time interval)
	float interval = 1;
	//param <text> (slogan)
	char* slogan = argv[1];
	
	//get -d
	int opt;
	while( (opt=getopt(argc, argv, "d:")) != -1 )
	{
		switch( opt )
		{
		case 'd': 
			interval = strtof( optarg, NULL )*2.5; 
			if( !strcmp(argv[1], "-d") ) 
				slogan = argv[3]; 
			break;
		}
	}

	//sleep struct
	struct timespec req;
	struct timespec rem;
	req.tv_sec = interval;
	req.tv_nsec = 0;
	
	//sigaction struct
	struct sigaction act;
	act.sa_handler = SIG_DFL;

	//wait for signal
	sigset_t set;
	sigemptyset( &set );
	sigaddset( &set, SIGUSR2 );
	if( sigwaitinfo( &set, NULL) == -1 )
		errExit( "sigwaitinfo_err" );

	//do something in loop
	while( 1 )
	{
		//write process pid
		pid_t pid = getpid();
		char buf[50];
		sprintf( buf, "%d: ", pid );
		strcat( buf, slogan );
		strcat( buf, "\n" );
		//write slogan
		if( write( STDOUT_FILENO, buf, strlen(buf) ) == -1 )
			errExit( "write_err" );
		
		//open urandom
		int randomData = open( "/dev/urandom", O_RDONLY );
		if( randomData == -1 )
			errExit( "open_err" );

		//change signal action
		int random_data;
		if( read(randomData, &random_data, sizeof(random_data) ) == -1 )
			errExit( "read_err" );
		random_data = abs(random_data) % 2;
		if( random_data )
			act.sa_handler = SIG_IGN;
		else
			act.sa_handler = SIG_DFL;
		if( sigaction( SIGUSR1, &act, NULL ) == -1 )
			errExit( "sigaction_err" );
		
		//send signal to group
		pid_t pgid = getpgid(pid);
		if( read(randomData, &random_data, sizeof(random_data) ) == -1 )
			errExit( "read_err" );
		
		//close urandom
		if( close( randomData ) == -1 )
			errExit( "close_err" );
	
		random_data = abs(random_data) % 10;
		if( !random_data )
		{
			if( killpg( pgid, SIGUSR1 ) == -1 )
				errExit( "killpg_err" );
		}
		
		//sleep
		while( nanosleep( &req, &rem ) == -1 )
		{
			req.tv_sec = rem.tv_sec;
			req.tv_nsec = rem.tv_nsec;
		}
	}
		
	return 0;
}

void Handler( int signum )
{

}
