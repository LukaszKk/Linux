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
	int b = bind( sock_fd, (struct sockaddr *)&A, sizeof(struct sockaddr_in) );
	if( b == -1 )
		err( "bind" );
	int l = listen( sock_fd, 50 );
	if( l == -1 )
		err( "listen" );
	struct sockaddr_in B;
	socklen_t Blen = sizeof(struct sockaddr_in);
	int new_fd = accept( sock_fd, (struct sockaddr *)&B, &Blen );
	if( new_fd == -1 )
		err( "accept" );
	
	if( close( sock_fd ) == -1 )
		err( "close" );
	
	char buf[50];
	write( new_fd, "good morning", 13 );
	read( new_fd, &buf, sizeof(buf) );
	write( STDOUT_FILENO, &buf, strlen(buf) );

	if( close( new_fd ) == -1 )
		err( "close" );

	return 0;
}
