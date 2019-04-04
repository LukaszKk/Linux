#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>

void errExit( char* str )
{
	perror( str );
	exit( EXIT_FAILURE );
}

int main( int argc, char* argv[] )
{
	//input check
	if( argc != 2 )
		errExit( "Usage: ./b.out <fifo file>" );
	
	//opening fifo to read
	int fop = open( argv[1], O_RDONLY | O_NONBLOCK );

	//preparing struct to sleep
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 57*10000000;
	
	//preparing buffers
	//char buf[4];

	char print[4];
	int cnt = 0;

	while( 1 )
	{
		if( ioctl( fop, FIONREAD, &cnt ) < 0  )
			errExit( "ioctl_err" );
		if( cnt > 600 )
			break;

		//printing count of unreadable
		sprintf( print, "%d", cnt );
		if( write( STDOUT_FILENO, &print, sizeof(print) ) == -1 )
			errExit( "write_err" );
		write( STDOUT_FILENO, "\n", 1 );

		//reading data
		//if( read(fop, &buf, sizeof(buf)) != -1 )
		//	errExit( "read_err" );
		
		//writing data to output
		//write( STDOUT_FILENO, "Data: ", sizeof("Data: ") );
		//write( STDOUT_FILENO, &buf, sizeof(buf) );
		//write( STDOUT_FILENO, "\n", 1 );

		//sleeping
		if( nanosleep( &ts, NULL ) == -1 )
			errExit( "sleep_err" );	
	}
	
	//closing fifo
	close( fop );

	return 0;
}
