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
#include <errno.h>

#define MAX 1310720
#define MAX_EVENTS 64
#define MAXBUF 1024
#define MAX_SEND 114688

void errExit( char * mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

void InputCheck( int argc, char* argv[], char** r, double* t, int* port, char* addr );

timer_t create_timer( double t );
void saHandlerGen( int sig );
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
unsigned int sizeBuf( unsigned int head, unsigned int tail );

int generateData( struct Buffer* buffer, int* i );

int makeSocket( int port, char addr[16] );
int setnonblocking(int fd);
void saHandlerSock( int sig );
volatile int socket_OK = 0;
int createNewConnection( int fd, int epoll_fd, struct Buffer* magazine );
int processData( int fd );

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
	
	setnonblocking(sock_fd);

	int l = listen( sock_fd, 5 );
	if( l == -1 )
		errExit( "listen error" );
	
	//struct sockaddr_in B;
	//socklen_t Blen = sizeof(struct sockaddr_in);
	
	int epoll_fd = epoll_create1(0);
	if( epoll_fd == -1 )
		errExit( "epoll create error" );

	struct epoll_event ev, *events;
	ev.data.fd = sock_fd;
	ev.events = EPOLLIN | EPOLLET;
	if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1 )
		errExit( "epoll_ctl error" );
	
	events = calloc( MAX_EVENTS, sizeof(ev) );

	struct sigaction sa1;
	sa1.sa_handler = &saHandlerGen;
	if( sigaction(SIGCHLD, &sa1, NULL) == -1 )
		errExit( "Sigaction error" );
	
	struct sigaction sa2;
	sa2.sa_handler = &saHandlerSock;
	if( sigaction(SIGPIPE, &sa2, NULL) == -1 )
		errExit( "Sigaction error" );
	
	timer_t timerid = create_timer( t );

	int i = 65;
	while( 1 )
	{
		//generate data
		if( !isFull(magazine.head, magazine.tail) && generate_data )
		{
			generate_data = 0;

			//int count =
			generateData( &magazine, &i );
			
			//development
			write( 1, "@", 1 );
		}
		//development
		//if( i > 68 )
		//	break;
	
		//wait on descriptor to connection
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		/*if( nfds == -1 )
		{
			if( errno != EINTR )
				errExit( "epoll_wait error" );
			else
				continue;
		}*/

		int new_fd;
		for( int n = 0; n < nfds; n++ )
		{
			if( events[n].events & EPOLLHUP )
			{
				//development
				write( 1, "\nsocket closed\n", 16 );
			}
			else if( (events[n].events & EPOLLERR) || !(events[n].events & EPOLLIN) )
			{
				printf( "\nerrno: %d\n", errno );
			       	fflush(stdout);	
				if( close(events[n].data.fd) == -1 )
					errExit( "close events error" );
			}
			else if( events[n].data.fd == sock_fd )
			{
				new_fd = createNewConnection( sock_fd, epoll_fd, &magazine );
				//int new_fd = accept( sock_fd, (struct sockaddr*)&B, &Blen );
				//
				//setnonblocking(new_fd);
				//ev.events = EPOLLIN | EPOLLET;
				//ev.data.fd = new_fd;
				//if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1 )
				//	errExit( "epoll_ctl_new error" );
				
				//socket_OK = 1;

				//read 4 bytes
				//while( 1 )
				//{
					//client closed connection
					//if( !socket_OK )
					//{
						//development
					//	write( 1, "sock closed", 12 );
					//	break;
					//}
					
					/*int readed = recv(new_fd, buf, 4, MSG_DONTWAIT);
					if( readed == -1  )
					{
						if( errno == EINTR )
						{
							free( buf );
							continue;
						}
						//development
						fprintf( stdout, "\n%d\n", errno );
						fflush( stdout );
						write( 1, "read bad count\n", 15 );
						break;
					}
					free( buf );

					//development
					write( new_fd, "helloWW\n", 9 );
					*/

					//send data
					/*if( sizeBuf(magazine.head, magazine.tail) >= MAX_SEND )
					{
						for( int j = 0; j < MAX_SEND; j++ )
							buf_send[j] = readBuf( &magazine );
						write( new_fd, buf_send, MAX_SEND );
					}*/
				//}
			}
			else if( processData( events[n].data.fd ) == 4 ) 		//sending
			{
				char* buf_send = malloc( MAX_SEND );
	
				//TODO 50 -> MAX_SEND
				if( sizeBuf(magazine.head, magazine.tail) >= 50 )
				{
					//TODO 50 -> MAX_SEND
					for( int j = 0; j < 50; j++ )
						buf_send[j] = readBuf( &magazine );
					
					send( events[n].data.fd, buf_send, MAX_SEND, 0 );
				}
			
				free( buf_send );
			}
		}
	}

	//development
	//write( 1, magazine.data, strlen(magazine.data) );

	free( events );

	if( close( sock_fd ) == -1 )
		errExit( "close socket error" );
	
	if( timer_delete( timerid ) == -1 )
		errExit( "timer_delete error" );

	return 0;
}


//===================================================================
//===================================================================

void InputCheck( int argc, char* argv[], char** r, double* t, int* port, char* addr )
{
	if( argc < 4 || argc > 6 )
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
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = t*60/96*1000000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = t*60/96*1000000000;

	if( timer_settime(timerid, 0, &its, NULL) == -1 )
		errExit( "timer_settime error" );

	return timerid;
}

void saHandlerGen( int sig )
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

unsigned int sizeBuf( unsigned int head, unsigned int tail )
{
	return tail - head;
}

//===================================================================

int generateData( struct Buffer* buffer, int* i )
{
	int j = 0;
	for( ; j < 640; j++ )
		writeBuf( buffer, *i );
	(*i)++;
	if( *i == 122 ) *i = 64;
	else if( *i == 90 ) *i = 96;

	return j;
}

//===================================================================
int makeSocket( int port, char addr[16] )
{
	int sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sock_fd == -1 )
		errExit( "socket create error" );
	
	int opt = 1;
 	if( setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
        	errExit( "setsockopt error" );

	struct sockaddr_in A;
	A.sin_family = AF_INET;
	A.sin_port = htons( port );
	if( !strcmp( addr, "localhost" ) )
		strcpy( addr, "127.0.0.1" );
	inet_aton( addr, &A.sin_addr );
 	bzero( &(A.sin_zero), 8 );

	int b = bind( sock_fd, (struct sockaddr*)&A, sizeof(struct sockaddr_in) );
	if( b == -1 )
		errExit( "bind error" );

	return sock_fd;
}

int setnonblocking(int fd)
{
    int flags;
    if( (flags = fcntl(fd, F_GETFL, 0)) == -1 )
        errExit( "fcntl error" );
    if( fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 )
	    errExit( "fcntl error" );

    return 0;
}

void saHandlerSock( int sig )
{
	socket_OK = 0;
}


int createNewConnection( int fd, int epoll_fd, struct Buffer* magazine )
{
	struct epoll_event event;
	struct sockaddr B;
	socklen_t Blen = sizeof(B);
	int new_fd = -1;

	while( (new_fd = accept(fd, (struct sockaddr*)&B, &Blen) ) != -1 )
	{
		//socket_OK = 1;
		setnonblocking( new_fd );

		event.data.fd = new_fd;
		event.events = EPOLLIN | EPOLLET;
		if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1 )
			errExit( "epoll_ctl" );
		Blen = sizeof(B);
	}

	if( errno != EAGAIN && errno != EWOULDBLOCK)
		perror("accept");

	return new_fd;
}

int processData( int fd )
{
	char buf[5];
	//TODO check
	ssize_t count;// = read(fd, buf, sizeof(buf) - 1);
	int cnt;

	while( (count = read(fd, buf, sizeof(buf) - 1)) > 0 )
		cnt = count;

	return cnt;
}
