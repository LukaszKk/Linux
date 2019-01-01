#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

void errExit( char* str )
{
	perror( str );
	exit( EXIT_SUCCESS );
}

int main( int argc, char* argv[] )
{
	//param checking
	if( argc != 2 )
	{
		printf( "Usage: %s <fifo>\n", argv[0] );
		return -1;
	}

	//open fifo
	int fd = open( argv[1], O_RDONLY );
	if( fd == -1 )
		errExit( "open_err" );
	
	//create buffer, size 15B
	char buff[15];
	
	//poll struct
	struct pollfd fds[0];
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	
	while( 1 )
	{
		if( poll( fds, 1, -1 ) == -1 )
			errExit( "poll_err" );
	
		if( fds[0].revents == POLLIN )
		{
			if( fcntl( fd, O_NONBLOCK ) == F_SETFL )
				//ok
			if( read( fds[0].fd, &buff, sizeof(buff) ) == -1 )
				errExit( "read_err" );
		}
		
	}

	close( fd );

	return 0;
}
