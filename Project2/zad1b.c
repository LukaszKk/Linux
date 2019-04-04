#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

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
	int fop = open( argv[1], O_RDONLY );

	//preparing struct to sleep
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 600000000;
	//preparing struct to clock
	struct timespec tsc;
	
	//preparing buffers
	char* buf = (char*)calloc(16000, 1);
	char dtca[11];
	
	//time writing before reading
	if( clock_gettime( CLOCK_REALTIME, &tsc ) == -1 )
		errExit( "gettime_err" );
	
	double timec = tsc.tv_sec*1000000000 + tsc.tv_nsec;
	sprintf( dtca, "%lf", timec );

	if( write( STDERR_FILENO, &dtca, sizeof(dtca) ) == -1 )
		errExit( "write_time_err" );
	write( STDERR_FILENO, "\n", 1 );
	
	double received;

	//writing to fifo
	while( (received=read( fop, &buf, sizeof(buf))) != -1 )
	{
		
		//writing received count
		write( STDERR_FILENO, "Received: ", sizeof("Received: ") );
		sprintf( dtca, "%lf", received );
		if( write( STDERR_FILENO, &dtca, sizeof(dtca) ) == -1 )
			errExit( "write_sended_err" );
		write( STDERR_FILENO, "\n", 1 );
		
		//sleeping
		write( STDOUT_FILENO, &buf, sizeof(buf) );
		write( STDERR_FILENO, "\n", 1 );
		if( nanosleep( &ts, NULL ) == -1 )
			errExit( "sleep_err" );	
		
		//time writing before reading
		if( clock_gettime( CLOCK_REALTIME, &tsc ) == -1 )
			errExit( "gettime_err" );
		
		timec = tsc.tv_sec*1000000000 + tsc.tv_nsec;
		
		sprintf( dtca, "%lf", timec );
		if( write( STDERR_FILENO, &dtca, sizeof(dtca) ) == -1 )
			errExit( "write_time_err" );
		write( STDERR_FILENO, "\n", 1 );
	}
	
	//closing fifo
	close( fop );

	//freeing memory
	free( buf );
	
	return 0;
}
