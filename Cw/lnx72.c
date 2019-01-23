#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


void err( char* mes )
{
	perror( mes );
	exit( 0 );
}

int main( int argc, char* argv[] )
{
	if( argc != 2 )
		err( "input" );
	int sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sock_fd == -1 )
		err( "sock" );
	struct sockaddr_in A;
	A.sin_family = AF_INET;
	A.sin_port = htons(12345);
	inet_aton(argv[1], &A.sin_addr);

	int c = connect( sock_fd, (struct sockaddr *)&A, sizeof(struct sockaddr_in) );
	if( c == -1 )
		err( "connect" );
	
	char buf[50];
	read( sock_fd, &buf, 50 );
	write( sock_fd, "good midnight", 14 );
	write( STDOUT_FILENO, &buf, strlen(buf) );


	if( close( sock_fd ) == -1 )
		err( "close" );

	return 0;
}
