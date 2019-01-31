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
#define MAX_DESCRIPTORS 10000

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

struct Descriptors
{
	int fd;
	unsigned int sendcnt;	//statements count
};
volatile int descindex = 0;
int findDesc( struct Descriptors* d, int fd );
void updateDesc( struct Descriptors** d, int fd );

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
	char* r;
	double t;
	int port;
	char addr[16] = { '\0' };
	InputCheck( argc, argv, &r, &t, &port, addr );

	//repository
	struct Buffer magazine;
	magazine.head = 0;
	magazine.tail = 0;

	//create socket
	int sock_fd = makeSocket( port, addr );
	setnonblocking(sock_fd);

	int l = listen( sock_fd, 5 );
	if( l == -1 )
		errExit( "listen error" );
	
	//create epoll
	int epoll_fd = epoll_create1(0);
	if( epoll_fd == -1 )
		errExit( "epoll create error" );

	struct epoll_event ev, *events;
	ev.data.fd = sock_fd;
	ev.events = EPOLLIN | EPOLLET;
	if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) == -1 )
		errExit( "epoll_ctl error" );
	
	events = calloc( MAX_EVENTS, sizeof(ev) );

	//saves info about consumer
	struct Descriptors* desc = calloc( MAX_DESCRIPTORS, sizeof(struct Descriptors) );
	char* buf_send = malloc( MAX_SEND );
	//[A-Z][a-z]
	int i = 65;
	
	//signal for timer
	struct sigaction sa1;
	sa1.sa_handler = &saHandlerGen;
	if( sigaction(SIGCHLD, &sa1, NULL) == -1 )
		errExit( "Sigaction error" );
	
	struct sigaction sa2;
	sa2.sa_handler = &saHandlerSock;
	if( sigaction(SIGPIPE, &sa2, NULL) == -1 )
		errExit( "Sigaction error" );
	
	//create timer and set
	timer_t timerid = create_timer( t );

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
	
		//epoll_wait
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

		for( int n = 0; n < nfds; n++ )
		{
			if( events[n].events & EPOLLHUP )
			{
				//development
				printf( "\ndesc: %d", descindex );
				fflush( stdout );

				//client closed connection
				//remove info about client
				updateDesc( &desc, events[n].data.fd );
				
				//development
				write( 1, "\nsocket closed\n", 16 );
				//development
				printf( "\ndesc: %d", descindex );
				fflush( stdout );
			}
			else if( (events[n].events & EPOLLERR) || !(events[n].events & EPOLLIN) )
			{
				//development
				printf( "\nerrno: %d\n", errno );
			       	fflush(stdout);
				
				//errors - close connection
				updateDesc( &desc, events[n].data.fd );
				if( close(events[n].data.fd) == -1 )
					errExit( "close events error" );
			}
			else if( events[n].data.fd == sock_fd )
			{
				//new connection
				desc[descindex].fd = createNewConnection( sock_fd, epoll_fd, &magazine );
				++descindex;
			}
			else
			{
				//check if there are some statements
				if( processData( events[n].data.fd ) == 4 )
				{
					int indx = findDesc( desc, events[n].data.fd );
					if( indx == -1 )
						continue;
					//save statements count
					desc[indx].sendcnt += 1;
				}
			}
		}

		//sending
		//send to every which send statement
		for( int k = 0; k < descindex; k++ )
		{
			//TODO 50 -> MAX_SEND
			if( sizeBuf(magazine.head, magazine.tail) >= 50 )
			{
				//m statements - send m times
				for( int m = desc[k].sendcnt; m > 0; m--, desc[k].sendcnt-- )	
				{
					//TODO 50 -> MAX_SEND
					if( sizeBuf(magazine.head, magazine.tail) >= 50 )
					{
						//TODO 50 -> MAX_SEND
						for( int j = 0; j < 50; j++ )
							buf_send[j] = readBuf( &magazine );
			
						send( desc[k].fd, buf_send, MAX_SEND, 0 );
					}
					else
						break;
				}
			}
			else
				break;
		}
	}

	free( buf_send );
	free( events );
	free( desc );

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
	int new_fd = accept(fd, (struct sockaddr*)&B, &Blen);
	if( new_fd == -1 )
		errExit( "accept" );
	
	//socket_OK = 1;
	
	setnonblocking( new_fd );

	event.data.fd = new_fd;
	event.events = EPOLLIN | EPOLLET;
	if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1 )
		errExit( "epoll_ctl" );

	return new_fd;
}

int processData( int fd )
{
	char buf[5];
	////TODO check
	ssize_t count = read(fd, buf, sizeof(buf) - 1);
	//int cnt;

	//while( (count = read(fd, buf, sizeof(buf) - 1)) > 0 )
	//	cnt = count;

	return count;
}

//===================================================================

int findDesc( struct Descriptors* d, int fd )
{
	for( int i = 0; i < descindex; i++ )
	{
		if( fd == d[i].fd )
			return i;
	}

	return -1;
}

void updateDesc( struct Descriptors** d, int fd )
{
	struct Descriptors tmp[MAX_DESCRIPTORS];
	for( int i = 0; i < descindex; i++ )
	{
		tmp[i] = *d[i];
	}
	memset( *d, 0, MAX_DESCRIPTORS*sizeof(struct Descriptors) );
	
	int n = findDesc( *d, fd );
	int j = 0;
	for( int i = 0; i < descindex; i++ )
	{
		if( i == n )
			continue;
		*d[j] = tmp[i];
		++j;
	}

	--descindex;
}
