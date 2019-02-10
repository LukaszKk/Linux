#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define MAX 1310720
#define MAX_EVENTS 64
//#define MAX_SEND 114688

//DEVELOPMENT
#define MAX_SEND 4096

#define MAX_DESCRIPTORS 10000

void errExit( char * mes )
{
    perror( mes );
    exit( EXIT_FAILURE );
}

void InputCheck( int argc, char* argv[], char** r, double* t, int* port, char* addr );

timer_t createTimer( double t, int sig );
timer_t createReportTimer( int t, int sig );
void saHandlerGen( int sig );
void saHandlerRep( int sig );
volatile int generate_data = 0;
volatile int report_data = 0;

struct Buffer
{
    char data[MAX];
    unsigned int head;
    unsigned int tail;
    unsigned int size;
};
int isEmpty( unsigned int head, unsigned int tail );
int isFull( unsigned int head, unsigned int tail );
int writeBuf( struct Buffer* buffer, char data );
void readBuf( struct Buffer* buffer, char* buf );

int generateData( struct Buffer* buffer, int* i, unsigned int head, unsigned int tail );

int createSocket( int port, char* addr );
int setNonBlocking( int fd );
int createEpoll( int sock_fd, struct epoll_event* ev, struct epoll_event** events );

struct Descriptors
{
    int fd;			            //descriptor
    unsigned int sendCount;	    //statements count
    char addr[14];		        //address
};
volatile int descIndex = 0;
int findDesc( struct Descriptors* d, int fd, int maxIndex );
void updateDesc( struct Descriptors* d, int fd );

void createNewConnection( int fd, int outfile, int epoll_fd, struct Descriptors* desc );
void statementsCheck( int fd, struct Descriptors* desc );
void sendData( struct Buffer* magazine, long int* flow, struct Descriptors* desc );
void closeConnection( int fd, int outfile, struct Descriptors* desc );

void closeConnectionReport( int outfile );
void newConnectionReport( int outfile, char* new_addr );
void fiveSecReport( int outfile, struct Buffer magazine, long int flow );

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
    magazine.size = 0;

    //create socket
    int sock_fd = createSocket( port, addr );

    //create epoll
    struct epoll_event ev, *events;
    int epoll_fd = createEpoll( sock_fd, &ev, &events );

    struct Descriptors* desc = calloc( MAX_DESCRIPTORS, sizeof(struct Descriptors) );       //saves info about consumer
    int i = 65;         //[A-Z][a-z]
    long int flow = 0;

    //open file to save report in
    int outfile = open( r, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR );
    if( outfile == -1 )
        errExit( "open file error" );

    //create timers and set them
    timer_t timerId = createTimer( t, SIGCHLD );
    timer_t timerReportId = createReportTimer( 5, SIGALRM );

    while( 1 )
    {
        //DEVELOPMENT
        if( i == 'z' )
            break;

        //generate data
        flow += generateData( &magazine, &i, magazine.head, magazine.tail );

        //DEVELOPMENT
        printf( "%d, ", magazine.size );
        fflush( stdout );

        //epoll_wait
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for( int n = 0; n < nfds; n++ )
        {
            if( (events[n].events & EPOLLHUP) || (events[n].events & EPOLLRDHUP) || (events[n].events & EPOLLERR) || !(events[n].events & EPOLLIN) )
            {
                //client closed connection
                closeConnection( events[n].data.fd, outfile, desc );

                //DEVELOPMENT
                printf( "\nclient closed\n" );
                fflush( stdout );
            }
            else if( events[n].data.fd == sock_fd )
            {
                //new connection
                createNewConnection( sock_fd, outfile, epoll_fd, desc );
            }
            else
            {
                //check if there are some statements
                statementsCheck( events[n].data.fd, desc );
            }
        }

        //sending
        sendData( &magazine, &flow, desc );

        if( report_data )
        {
            report_data = 0;
            fiveSecReport( outfile, magazine, flow );
            flow = 0;
        }
    }

    free( events );
    free( desc );

    if( close( outfile ) == -1 )
        errExit( "close file error" );

    if( close( sock_fd ) == -1 )
        errExit( "close socket error" );

    if( timer_delete( timerId ) == -1 )
        errExit( "timer_delete error" );

    if( timer_delete( timerReportId ) == -1 )
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

    //addr and port
    int flag = 0, flag2 = 0;
    int mult = 10;
    *port = 0;
    for( int i = 0; i < strlen(argv[argc-1]); i++ )
    {
        if( argv[argc-1][i] == ':' )
        {
            flag = 1;
            continue;
        }
        if( !flag )
            addr[i] = argv[argc-1][i];
        else
        {
            flag2 = 1;
            *port = *port*mult + (argv[argc-1][i]-'0');
        }
    }
    if( !flag2 )
    {
        *port = (int) strtol( addr, NULL, 10 );
        strcpy( addr, "localhost" );
    }
    if( strlen(addr) > 15 )
        errExit( "adress error" );
}

//===================================================================

timer_t createTimer( double t, int sig )
{
    struct sigaction sa1;
    sa1.sa_handler = saHandlerGen;
    sigemptyset (&sa1.sa_mask);
    sa1.sa_flags = 0;
    if( sigaction(SIGCHLD, &sa1, NULL) == -1 )
        errExit( "Sigaction error" );

    struct sigevent sev;
    timer_t timerId;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = sig;
    if( timer_create(CLOCK_REALTIME, &sev, &timerId) == -1 )
        errExit( "timer_create error" );

    long int tmp = (long int) (t * 60 / 96 * 1000000000);
    long int sec = 0;
    long int nSec = tmp;
    if( tmp > 1000000000 )
    {
        sec = tmp / 1000000000;
        nSec = tmp - 1000000000;
        while( nSec >= 1000000000 )
            nSec = nSec - 1000000000;
    }

    struct itimerspec its;
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nSec;
    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = nSec;

    if( timer_settime(timerId, 0, &its, NULL) == -1 )
        errExit( "timer_settime error" );

    return timerId;
}

timer_t createReportTimer( int t, int sig )
{
    struct sigaction sa2;
    sa2.sa_handler = saHandlerRep;
    sigemptyset (&sa2.sa_mask);
    sa2.sa_flags = 0;
    if( sigaction(SIGALRM, &sa2, NULL) == -1 )
        errExit( "Sigaction error" );

    struct sigevent sev;
    timer_t timerId;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = sig;
    if( timer_create(CLOCK_REALTIME, &sev, &timerId) == -1 )
        errExit( "timerReport_create error" );

    struct itimerspec its;
    its.it_value.tv_sec = t;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = t;
    its.it_interval.tv_nsec = 0;

    if( timer_settime(timerId, 0, &its, NULL) == -1 )
        errExit( "timerReport_settime error" );

    return timerId;
}
void saHandlerGen( int sig )
{
    generate_data = 1;
}

void saHandlerRep( int sig )
{
    report_data = 1;
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

int writeBuf( struct Buffer* buffer, char data )
{
    if( (buffer->size + 640) > MAX )
        return -1;

    buffer->size += 640;

    if( (buffer->head + 640) <= MAX )
    {
        char buf[640];
        memset( buf, data, sizeof(buf) );
        memcpy( &buffer->data[buffer->head], buf, sizeof(buf) );
        buffer->head += 640;

        return 0;
    }

    unsigned int n = 640;
    n -= MAX - buffer->head;

    char buf[MAX - buffer->head];
    memset( buf, data, sizeof(buf) );
    memcpy( &buffer->data[buffer->head], buf, sizeof(buf) );

    buffer->head = n;

    char buf2[n];
    memset( buf2, data, sizeof(buf2) );
    memcpy( &buffer->data[0], buf2, sizeof(buf2) );

    return 0;
}

void readBuf( struct Buffer* buffer, char* buf )
{
    buffer->size -= MAX_SEND;

    if( (buffer->tail + MAX_SEND) <= MAX )
    {
        memcpy( buf, &buffer->data[buffer->tail], MAX_SEND );
        buffer->tail += MAX_SEND;
        return;
    }

    unsigned int n = MAX_SEND;
    n -= MAX - buffer->tail;
    memcpy( &buffer->data[buffer->tail], buf, MAX - buffer->tail );
    buffer->tail = n;
    memcpy( &buffer->data[0], &buf[MAX - buffer->tail], n );
}

//===================================================================

int generateData( struct Buffer* buffer, int* i, unsigned int head, unsigned int tail )
{
    if( isFull(head, tail) || !generate_data )
        return 0;

    generate_data = 0;

    if( writeBuf( buffer, (char)*i ) == -1 )
        return 0;

    (*i)++;
    if( *i == 122 ) *i = 64;
    else if( *i == 90 ) *i = 96;

    return 640;
}

//===================================================================

int createSocket( int port, char* addr )
{
    int sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_fd == -1 )
        errExit( "socket create error" );

    int opt = 1;
    if( setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
        errExit( "setsockopt error" );

    struct sockaddr_in A;
    A.sin_family = AF_INET;
    A.sin_port = htons((uint16_t) port );
    if( !strcmp( addr, "localhost" ) )
        strcpy( addr, "127.0.0.1" );
    inet_aton( addr, &A.sin_addr );
    bzero( &(A.sin_zero), 8 );

    int b = bind( sock_fd, (struct sockaddr*)&A, sizeof(struct sockaddr_in) );
    if( b == -1 )
        errExit( "bind error" );

    setNonBlocking( sock_fd );

    int l = listen( sock_fd, 5 );
    if( l == -1 )
        errExit( "listen error" );

    return sock_fd;
}

int setNonBlocking( int fd )
{
    int flags;
    if( (flags = fcntl(fd, F_GETFL, 0)) == -1 )
        errExit( "fcntl error" );
    if( fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 )
        errExit( "fcntl error" );

    return 0;
}

int createEpoll( int sock_fd, struct epoll_event* ev, struct epoll_event** events )
{
    int epoll_fd = epoll_create1(0);
    if( epoll_fd == -1 )
        errExit( "epoll create error" );

    ev->data.fd = sock_fd;
    ev->events = EPOLLIN | EPOLLET;
    if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, ev) == -1 )
        errExit( "epoll_ctl error" );
    *events = calloc( MAX_EVENTS, sizeof(*ev) );

    return epoll_fd;
}

//===================================================================

int findDesc( struct Descriptors* d, int fd, int maxIndex )
{
    for( int i = 0; i < maxIndex; i++ )
    {
        if( d[i].fd == fd )
            return i;
    }

    return -1;
}

void updateDesc( struct Descriptors* d, int fd )
{
    struct Descriptors tmp[descIndex];
    for( int i = 0; i < descIndex; i++ )
    {
        tmp[i] = d[i];
    }
    memset( d, 0, descIndex*sizeof(struct Descriptors) );

    int n = findDesc( tmp, fd, descIndex );
    if( n == -1 )
        errExit( "updateDesc: findDesc error" );

    int j = 0;
    for( int i = 0; i < descIndex; i++ )
    {
        if( i == n )
            continue;
        d[j] = tmp[i];
        ++j;
    }

    --descIndex;
}

//===================================================================

void createNewConnection( int fd, int outfile, int epoll_fd, struct Descriptors* desc )
{
    struct epoll_event event;
    struct sockaddr_in B;
    socklen_t Blen = sizeof(B);
    int new_fd = accept(fd, (struct sockaddr*)&B, &Blen);
    if( new_fd == -1 )
        errExit( "accept" );

    strcpy( desc[descIndex].addr, inet_ntoa( B.sin_addr ) );

    setNonBlocking( new_fd );

    event.data.fd = new_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event) == -1 )
        errExit( "epoll_ctl" );

    newConnectionReport( outfile, desc[descIndex].addr );

    desc[descIndex].fd = new_fd;
    ++descIndex;
}

void statementsCheck( int fd, struct Descriptors* desc )
{
    char buf[4];
    int i = findDesc( desc, fd, descIndex );
    if( i == -1 )
        errExit( "statementsCheck: findDesc error" );

    while( recv( fd, buf, 4, 0 ) == 4 )
    {
        //save statements count
        desc[i].sendCount += 1;
    }
}

void sendData( struct Buffer* magazine, long int* flow, struct Descriptors* desc )
{
    char bufSend[MAX_SEND];
    //send to every which send statement
    for( int k = 0; k < descIndex; k++ )
    {
        if( magazine->size < MAX_SEND )
            return;

        //m statements - send m times
        for( int m = desc[k].sendCount; m > 0; m-- )
        {
            if( magazine->size < MAX_SEND )
                return;

            readBuf( magazine, bufSend );

            int sendedCount = 0;
            while( sendedCount < MAX_SEND )
                sendedCount += send( desc[k].fd, &bufSend[sendedCount], 1024, 0 );

            *flow -= MAX_SEND;

            desc[k].sendCount -= 1;
        }
    }
}

void closeConnection( int fd, int outfile, struct Descriptors* desc )
{
    if( shutdown( fd, SHUT_RDWR ) == -1 )
        errExit( "shutdown events error" );
    if( close(fd) == -1 )
        errExit( "close events error" );

    //report
    closeConnectionReport( outfile );

    int k = findDesc( desc, fd, descIndex );
    if( k == -1 )
        errExit( "findDesc error" );

    if( write( outfile, desc[k].addr, strlen( desc[k].addr )) == -1 )
        errExit( "write to file error" );
    if( write( outfile, "\n", 1 ) == -1 )
        errExit( "write to file error" );

    //remove info about consumer
    updateDesc( desc, fd );
}

//===================================================================

void closeConnectionReport( int outfile )
{
    struct timespec t1, t2;
    char buf[50] = { '\0' };
    if( clock_gettime( CLOCK_REALTIME, &t1 ) == -1 )
        errExit( "clock_gettime error" );
    if( clock_gettime( CLOCK_MONOTONIC, &t2 ) == -1 )
        errExit( "clock_gettime error" );

    if( write( outfile, "Connection closed: \t", strlen("Connection closed: \t") ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "WallTime: %ld \t ", t1.tv_sec * 1000000000 + t1.tv_nsec );
    if( write( outfile, buf, strlen( buf )) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "Monotonic: %ld \t Address: ", t2.tv_sec * 1000000000 + t2.tv_nsec );
    if( write( outfile, buf, strlen( buf )) == -1 )
        errExit( "write to file error" );
}

void newConnectionReport( int outfile, char* new_addr )
{
    struct timespec t1, t2;
    char buf[50] = { '\0' };
    if( clock_gettime(CLOCK_REALTIME, &t1) == -1 )
        errExit( "clock_gettime error" );
    if( clock_gettime(CLOCK_MONOTONIC, &t2) == -1 )
        errExit( "clock_gettime error" );

    if( write( outfile, "New connection: \t", strlen("New connection: \t") ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "WallTime: %ld \t ", t1.tv_sec*1000000000+t1.tv_nsec );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "Monotonic: %ld \t Address: ", t2.tv_sec*1000000000+t2.tv_nsec );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );

    if( write( outfile, new_addr, strlen(new_addr) ) == -1 )
        errExit( "write to file error" );
    if( write( outfile, "\n", 1 ) == -1 )
        errExit( "write to file error" );
}

void fiveSecReport( int outfile, struct Buffer magazine, long int flow )
{
    struct timespec t1, t2;
    char buf[50] = { '\0' };
    if( clock_gettime(CLOCK_REALTIME, &t1) == -1 )
        errExit( "clock_gettime error" );
    if( clock_gettime(CLOCK_MONOTONIC, &t2) == -1 )
        errExit( "clock_gettime error" );

    if( write( outfile, "Periodic report: \t", strlen("Periodic report: \t") ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "WallTime: %ld \t ", t1.tv_sec*1000000000+t1.tv_nsec );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "Monotonic: %ld \t ", t2.tv_sec*1000000000+t2.tv_nsec );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );

    sprintf( buf, "Connected: %d \t ", descIndex );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );

    sprintf( buf, "Size: %d\t", magazine.size );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "Size2: %f%%\t", (double)(magazine.size)*100/MAX );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );
    sprintf( buf, "Flow: %ld\n", flow );
    if( write( outfile, buf, strlen(buf) ) == -1 )
        errExit( "write to file error" );
}
