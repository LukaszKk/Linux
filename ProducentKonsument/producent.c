#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX 1250000
#define MAX_EVENTS 10
#define MAXBUF 1024

void errExit( char * mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

void InputCheck( int argc, char* argv[], char** r, double* t, int* port, char* addr );

timer_t create_timer( double t );
void handler( int signal );
volatile int generate_data = 0;

struct Buffer
{
	char data[MAX];
	unsigned int head;
	unsigned int tail;
};
int isEmpty( unsigned int head, unsigned int tail );
int isFull( unsigned int head, unsigned int tail );
void writeBuf( struct Buffer* buffer, char data );
char readBuf( struct Buffer* buffer );

int makeSocket( int port, char addr[16] );
int setnonblocking(int fd);

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
	char* r;
	double t;
	int port;
	char addr[16] = { '\0' };
	InputCheck( argc, argv, &r, &t, &port, addr );

	struct Buffer magazine;
	magazine.head = 0;
	magazine.tail = 0;

	int sock_fd = makeSocket( port, addr );
	
	struct sockaddr_in B;
	socklen_t Blen = sizeof(struct sockaddr_in);
	
	int epoll_fd = epoll_create1(0);
	if( epoll_fd == -1 )
		errExit( "epoll create error" );
	struct epoll_event ev, events[MAX_EVENTS];
	int nfds;
	ev.events = EPOLLIN;
	ev.data.fd = sock_fd;
	if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1 )
		errExit( "epoll_ctl error" );

	struct sigaction sa;
	sa.sa_handler = handler;
	if( sigaction(SIGCHLD, &sa, NULL) == -1 )
		errExit( "Sigaction error" );
	
	timer_t timerid = create_timer( t );

	int i = 65;
	while( 1 )
	{
		if( !isFull(magazine.head, magazine.tail) && generate_data )
		{
			for( int j = 0; j < 640; j++ )
				writeBuf( &magazine, i );
			i++;
			generate_data = 0;
		}
	
		//
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if( nfds == -1 )
			errExit( "epoll_wait error" );
		for( int n = 0; n < nfds; n++ )
		{
			if( events[n].data.fd == sock_fd )
			{
				int new_fd = accept( sock_fd, (struct sockaddr*)&B, &Blen );
				if( new_fd == -1 )
					errExit( "accept error" );
				setnonblocking(new_fd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = new_fd;
				if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1 )
					errExit( "epoll_ctl_new error" );


			}
		}


		if( i == 122 ) i = 64;
		else if( i == 90 ) i = 96;
		//development
		if( i > 68 )
			break;
	}

	write( 1, magazine.data, strlen(magazine.data) );

	if( close( sock_fd ) == -1 )
		errExit( "close socket error" );

	/*
	if( close( new_fd ) == -1 )
		errExit( "close socket error" );
	*/

	if( timer_delete( timerid ) == -1 )
		errExit( "timer_delete error" );

	return 0;
}


//===================================================================
//===================================================================

void InputCheck( int argc, char* argv[], char** r, double* t, int* port, char* addr )
{
	if( argc != 6 )
	{
		printf( "Usage: %s -r <path> -t <val> [<addr>:]port\n", argv[0] );
		errExit( "Input Error" );
	}

	int opt;
	while( (opt=getopt(argc, argv, "r:t:")) != -1 )
	{
		switch( opt )
		{
		case 'r': *r = optarg; break;
		case 't': *t = strtof( optarg, NULL ); break;
		default: break;
		}
	}
	if( *t < 1 || *t > 8 )
		errExit( "Param T must be between 1 and 8" );
	*t = *t/96;
	
	int flag = 0, flag2 = 0;
	int mult = 10;
	*port = 0;
	for( int i = 0; i < strlen(argv[5]); i++ )
	{
		if( argv[5][i] == ':' )
		{
			flag = 1;
			continue;
		}
		if( !flag )
			addr[i] = argv[5][i];
		else
		{
			flag2 = 1;
			*port = *port*mult + (argv[5][i]-'0');
		}
	}
	if( !flag2 )
	{
		*port = strtol( addr, NULL, 10 );
		strcpy( addr, "localhost" );
	}
	if( strlen(addr) > 15 )
		errExit( "adress error" );
}

//===================================================================

timer_t create_timer( double t )
{
	struct sigevent sev;
	timer_t timerid;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGCHLD;
	if( timer_create(CLOCK_REALTIME, &sev, &timerid) == -1 )
		errExit( "timer_create error" );

	struct itimerspec its;
	its.it_value.tv_sec = t*100;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = t * 100;
	its.it_interval.tv_nsec = 0;

	if( timer_settime(timerid, 0, &its, NULL) == -1 )
		errExit( "timer_settime error" );

	return timerid;
}

void handler( int signal )
{
	generate_data = 1;
}

//===================================================================

int isEmpty( unsigned int head, unsigned int tail )
{
	return head == tail;
}

int isFull( unsigned int head, unsigned int tail )
{
	return (head+1) == tail;
}

void writeBuf( struct Buffer* buffer, char data )
{
	if( isFull( buffer->head, buffer->tail ) )
		return;
	buffer->data[buffer->head] = data;
	if( buffer->head + 1 > MAX )
		buffer->head = 0;
	else
		buffer->head += 1;
}

char readBuf( struct Buffer* buffer )
{
	if( isEmpty( buffer->head, buffer->tail ) )
			return '0';
	if( buffer->tail + 1 > MAX )
	{
		buffer->tail = 0;
		return buffer->data[buffer->tail];
	}
	buffer->tail += 1;
	return buffer->data[buffer->tail];
}

//===================================================================

int makeSocket( int port, char addr[16] )
{
	int sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sock_fd == -1 )
		errExit( "socket create error" );
	
	struct sockaddr_in A;
	A.sin_family = AF_INET;
	A.sin_port = htons( port );
	if( !strcmp( addr, "localhost" ) )
		strcpy( addr, "127.0.0.1" );
	inet_aton( addr, &A.sin_addr );
	int b = bind( sock_fd, (struct sockaddr*)&A, sizeof(struct sockaddr_in) );
	if( b == -1 )
		errExit( "bind error" );
	
	int l = listen( sock_fd, 50 );
	if( l == -1 )
		errExit( "listen error" );

	return sock_fd;
}

int setnonblocking(int fd)
{
    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

