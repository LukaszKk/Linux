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
		errExit( "Usage: ./a.out <fifo file>" );
	
	//creating fifo file
	if( mkfifo( argv[1], S_IRWXU ) == -1 )
		errExit( "mkfifo_err" );
	
	//opening fifo to write
	int fop = open( argv[1], O_WRONLY );

	//preparing struct to sleep
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 200000000;
	//preparing struct to clock
	struct timespec tsc;
	
	//preparing buffers
	char* buf = (char*)calloc(32000, 1);
	char dtca[11];

	//writing to fifo
	for( int i = 'A'; i < 'Z'; i++ )
	{
		//preparing data
		for( unsigned int j = 0; j < 32000; j++ )
			buf[j] = i;
		
		//time writing before sending
		if( clock_gettime( CLOCK_REALTIME, &tsc ) == -1 )
			errExit( "gettime_err" );
		
		double timec = tsc.tv_sec*1000000000 + tsc.tv_nsec;
		sprintf( dtca, "%lf", timec );

		if( write( STDERR_FILENO, &dtca, sizeof(dtca) ) == -1 )
			errExit( "write_time_err" );
		write( STDERR_FILENO, "\n", 1 );

		//sending data
		double sended = 0;
		if( (sended=write( fop, buf, sizeof(buf) )) == -1 )
			errExit( "write_data_err" );
		
		//writing sended count
		write( STDERR_FILENO, "Sended: ", sizeof("Sended: ") );
		sprintf( dtca, "%lf", sended );
		if( write( STDERR_FILENO, &dtca, sizeof(dtca) ) == -1 )
			errExit( "write_sended_err" );
		write( STDERR_FILENO, "\n", 1 );
		
		//sleeping
		if( nanosleep( &ts, NULL ) == -1 )
			errExit( "sleep_err" );	
	}
	
	//closing fifo
	close( fop );
	unlink( argv[1] );

	//freeing memory
	free( buf );
	
	return 0;
}
