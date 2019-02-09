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
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <openssl/md5.h>

//#define MAX_READ 114688

//DEVELOPMENT
#define MAX_READ 5000


void errExit( char* mes )
{
    perror( mes );
    exit( EXIT_FAILURE );
}

int InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr );
int randomizeL( unsigned int L1, unsigned int L2 );
double randomizeD( double D1, double D2 );

int connectSock( int port, char* addr );

timer_t createTimer( long int sec, long int nSec );
void saHandlerTim( int sig );
volatile int timer_pick = 1;

char* createMd5sum( char data[MAX_READ] );

typedef struct
{
    long int delay1;
    long int delay2;
    char* md5sum;
} Report;

void endingReport( struct timespec tEnd_1, struct timespec tEnd_2, int repIndex, char* addr, Report* report );

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
    int count;
    double r;
    int port;
    char addr[16] = { '\0' };

    int flagS = InputCheck( argc, argv, &count, &r, &port, addr );

    struct sigaction sa1;
    sa1.sa_handler = &saHandlerTim;
    if( sigaction(SIGCHLD, &sa1, NULL) == -1 )
        errExit( "Sigaction error" );

    char* buf = malloc( MAX_READ );
    Report* report = malloc( count*sizeof(Report) );
    int repIndex = 0;
    struct timespec tStart_1, tEnd_1, tStart_2, tEnd_2;

    int sock_fd = connectSock( port, addr );

    struct timespec req, rem;
    timer_t timerId;

    long int sec = (long int) r;
    long int nSec = 0;
    while( r >= 1 )
        r -= 1;
    nSec = (long int)(r * 1000000000);

    if( !flagS )
    {
        //timer_pick = 0;
        //timerId = createTimer( sec, nSec );

        req.tv_sec = sec;
        req.tv_nsec = nSec;
    }
    else
    {
        req.tv_sec = sec;
        req.tv_nsec = nSec;
    }

    int tmpCount = count;
    int recvCount = 0;

    int dataInSocketCount;
    while( recvCount < count )
    {
        if( (tmpCount > 0) && (timer_pick == 1) && (send(sock_fd, "aaaa", 4, 0) == 4) )
        {
            dataInSocketCount = 0;
            ioctl( sock_fd, FIONREAD, &dataInSocketCount );
            printf( "\ndataInSocketCount: %d\n", dataInSocketCount );

            --tmpCount;

            //if( !flagS )
              //  timer_pick = 0;

            if( clock_gettime(CLOCK_REALTIME, &tStart_1) == -1 )
                errExit( "clock_gettime error" );

            //DEVELOPMENT
            if( write( 1, "%", 1 ) == -1 )
                errExit( "write error" );
        }

        if( clock_gettime(CLOCK_REALTIME, &tStart_2) == -1 )
            errExit( "clock_gettime error" );

        //if( flagS )
        //{
           /* while( nanosleep( &req, &rem ) == -1 )
            {
                req.tv_sec = rem.tv_sec;
                req.tv_nsec = rem.tv_nsec;
            }*/
        //}

        dataInSocketCount = 0;
        ioctl( sock_fd, FIONREAD, &dataInSocketCount );
        //printf( "\ndataInSocketCount: %d\n", dataInSocketCount );

        if( dataInSocketCount >= MAX_READ )
        {
            if( recv( sock_fd, buf, MAX_READ, 0 ) != -1 ) //>= MAX_READ )
            {
                ++recvCount;

                if( clock_gettime( CLOCK_REALTIME, &tEnd_1 ) == -1 )
                    errExit( "clock_gettime error" );

                report[repIndex].md5sum = createMd5sum( buf );

                //DEVELOPMENT
                if( write( 1, "#", 1 ) == -1 )
                    errExit( "write error" );
            }
        }
        else
            continue;

        if( clock_gettime(CLOCK_REALTIME, &tEnd_2) == -1 )
            errExit( "clock_gettime error" );

        report[repIndex].delay1 = (tEnd_1.tv_sec-tStart_1.tv_sec)*1000000000 + (tEnd_1.tv_nsec-tStart_1.tv_nsec);
        report[repIndex].delay2 = (tEnd_2.tv_sec-tStart_2.tv_sec)*1000000000 + (tEnd_2.tv_nsec-tStart_2.tv_nsec);
        repIndex++;
    }

    if( close(sock_fd) == -1 )
        errExit( "close error" );

    if( clock_gettime(CLOCK_REALTIME, &tEnd_1) == -1 )
        errExit( "clock_gettime error" );
    if( clock_gettime(CLOCK_MONOTONIC, &tEnd_2) == -1 )
        errExit( "clock_gettime error" );

    endingReport( tEnd_1, tEnd_2, repIndex, addr, report );

    //if( !flagS )
        //if( timer_delete(timerId) == -1 )
      //      errExit( "timer delete error" );

    free( report );
    free( buf );

    return 0;
}

//===================================================================
//===================================================================

int InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr )
{
    if( argc < 4 || argc > 8 )
    {
        printf( "Usage: %s -# <cnt> -r <dly> | -s <dly> [<addr>:]port\n", argv[0] );
        errExit( "Input Error" );
    }

    int opt;
    char tmpS[50] = { '\0' };
    char tmpR[50] = { '\0' };
    char* tmpCount;
    while( (opt=getopt(argc, argv, "#:r:s:")) != -1 )
    {
        switch( opt )
        {
            case 's': strcpy(tmpS, optarg); break;
            case 'r': strcpy(tmpR, optarg); break;
            case '#': tmpCount = optarg; break;
            default: break;
        }
    }

    //port and addr
    int flag = 0, flag2 = 0;
    double mult = 10;
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
            *port = *port * ((int)mult) + (argv[argc-1][i] - '0');
        }
    }
    if( !flag2 )
    {
        *port = (int) strtol( addr, NULL, 10 );
        strcpy( addr, "localhost" );
    }
    if( strlen(addr) > 15 )
        errExit( "adress error" );

    //cnt
    flag = 0;
    unsigned int L1 = 0, L2 = 0;
    for( int i = 0; i < strlen(tmpCount); i++ )
    {
        if( tmpCount[i] == ':' )
        {
            flag = 1;
            continue;
        }
        if( !flag )
            L1 = L1*((int)mult) + (tmpCount[i]-'0');
        else
            L2 = L2*((int)mult) + (tmpCount[i]-'0');
    }

    if( L2 <= L1 )
        *cnt = L1;
    else
        *cnt = randomizeL( L1, L2 );

    //r/s
    int flags = 0;
    if( !strcmp( tmpR, "" ) )
    {
        flags = 1;
        strcpy(tmpR, tmpS);
    }

    flag = 0;
    flag2 = 0;
    double D1 = 0, D2 = 0;
    for( int i = 0; i < strlen(tmpR); i++ )
    {
        if( tmpR[i] == ':' )
        {
            flag = 1;
            flag2 = 0;
            mult = 10;
            continue;
        }
        if( tmpR[i] == '.' )
        {
            flag2 = 1;
            mult = 0.1;
            continue;
        }

        if( !flag )
        {
            if( !flag2 )
                D1 = D1*mult + (tmpR[i]-'0');
            else
            {
                D1 = D1 + (tmpR[i]-'0')*mult;
                mult *= 0.1;
            }
        }
        else
        {
            if( !flag2 )
                D2 = D2*mult + (tmpR[i]-'0');
            else
            {
                D2 = D2 + (tmpR[i]-'0')*mult;
                mult *= 0.1;
            }
        }
    }

    if( D2 <= D1 )
        *r = D1;
    else
        *r = randomizeD( D1, D2 );

    return flags;
}

int randomizeL( unsigned int L1, unsigned int L2 )
{
    srand((unsigned int) time(NULL));
    return rand()%L2 + L1;
}

double randomizeD( double D1, double D2 )
{
    srand((unsigned int) time(NULL));
    double range = D2 - D1;
    double div = RAND_MAX / range;
    return D1 + (double)(rand()/div);
}

//===================================================================

int connectSock( int port, char* addr )
{
    int sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_fd == -1 )
        errExit( "socket error" );

    struct sockaddr_in A;
    A.sin_family = AF_INET;
    A.sin_port = htons(port);
    if( !strcmp(addr, "localhost") )
        strcpy(addr, "127.0.0.1");
    inet_aton( addr, &A.sin_addr );
    bzero( &(A.sin_zero), 8 );

    int c = connect( sock_fd, (struct sockaddr*)&A, sizeof(struct sockaddr_in) );
    if( c == -1 )
        errExit( "connect error" );

    return sock_fd;
}

//===================================================================

timer_t createTimer( long int sec, long int nSec )
{
    struct sigevent sev;
    timer_t timerId;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGCHLD;
    if( timer_create(CLOCK_REALTIME, &sev, &timerId) == -1 )
        errExit( "timer_create error" );

    struct itimerspec its;
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nSec;
    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = nSec;

    if( timer_settime(timerId, 0, &its, NULL) == -1 )
        errExit( "timer_settime error" );

    return timerId;
}


void saHandlerTim( int sig )
{
    timer_pick = 1;
}

//===================================================================

char* createMd5sum( char data[MAX_READ] )
{
    unsigned char* out = malloc(sizeof(unsigned char)*16);
    memset( out, 0, sizeof(*out) );
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, MAX_READ);
    MD5_Final(out, &c);

    char* md5 = malloc(33);
    for(int i = 0; i < 16; ++i)
        sprintf(&md5[i*2], "%02x", (unsigned int)out[i]);

    return md5;
}

//===================================================================

void endingReport( struct timespec tEnd_1, struct timespec tEnd_2, int repIndex, char* addr, Report* report )
{
    fprintf( stderr, "\n1. Wall time: %ld[s], %ld[ns] \t Monotonic: %ld[s], %ld[ns]\n",
             tEnd_1.tv_sec, tEnd_1.tv_nsec, tEnd_2.tv_sec, tEnd_2.tv_nsec );
    fflush(stderr);
    fprintf( stderr, "2. Pid: %d \t address: %s\n", getpid(), addr );
    fflush(stderr);
    for( int i = 0 ; i < repIndex; i++ )
        fprintf( stderr, "3. Blok %d:\n\ta) %ld[ns]\n\tb) %ld[ns]\n\tc) md5: %s\n", i+1, report[i].delay1, report[i].delay2, report[i].md5sum );
    fflush(stderr);
}