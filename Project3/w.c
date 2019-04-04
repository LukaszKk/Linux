#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

void errExit( char* str )
{
	perror( str );
	exit( EXIT_FAILURE );
}

int main( int argc, char* argv[] )
{
	srand( time(NULL) );
	//input check
	if( argc != 2 )
		errExit( "Usage: ./a.out <fifo file>" );
	
	//creating fifo file
	if( mkfifo( argv[1], 0666 ) == -1 )
		errExit( "mkfifo_err" );
	
	//opening fifo to write
	int fop = open( argv[1], O_WRONLY );

	//preparing struct to sleep
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 42*10000000;
	
	//preparing data
	char buf1[1] = { 'A' };
	char buf2[2] = { 'B' };
	char buf3[3] = { 'C' };
	char buf4[4] = { 'D' };

	//dealing with signals
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, NULL );

	//sended count
	unsigned int send = 0;
	//char print[2];

	//random int
	int rd = open( "/dev/urandom", O_RDONLY );
	char readed[5];

	//writing to fifo
	while( 1 )
	{
		read( rd, &readed, 4 );
		int r = 1 + strtol( readed, NULL, 10 ) % 4;

		//preparing data
		switch( r )
		{
		case 1: if((send=write(fop, buf1, sizeof(buf1)))==-1) break; break;
		case 2: if((send=write(fop, buf2, sizeof(buf2)))==-1) break; break;
		case 3: if((send=write(fop, buf3, sizeof(buf3)))==-1) break; break;
		case 4: if((send=write(fop, buf4, sizeof(buf4)))==-1) break; break;
		}
		if( errno == EPIPE )
			errExit( "errno_write_err" );
		
		//printing sended
		//sprintf( print, "%d", send );
		//if( write( STDOUT_FILENO, &print, sizeof(print) ) == -1 )
		//	errExit( "write_err" );

		//sleeping
		if( nanosleep( &ts, NULL ) == -1 )
			errExit( "sleep_err" );	
	}
	
	//closing fifo
	close( fop );
	unlink( argv[1] );
	
	return 0;
}
