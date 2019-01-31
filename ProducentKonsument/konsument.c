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

#include <openssl/md5.h>


#define MAX_READ 114688


void errExit( char* mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}

int InputCheck( int argc, char* argv[], int* cnt, double* r, int* port, char* addr );
int randomizeL( unsigned int L1, unsigned int L2 );
double randomizeD( double D1, double D2 );

int connectSock( int port, char* addr );

timer_t create_timer( double t );
void saHandlerTim( int sig );
volatile int timer_pick = 1;

char* createMd5sum( char data[MAX_READ] );

typedef struct
{
	long int delay1;	
	long int delay2;
	char* md5sum;
} Report;

//===================================================================
//===================================================================

int main( int argc, char* argv[] )
{
	int cnt;
	double r;
	int port;
	char addr[16] = { '\0' };
	
	int flagS = InputCheck( argc, argv, &cnt, &r, &port, addr );
	
	struct sigaction sa1;
	sa1.sa_handler = &saHandlerTim;
	if( sigaction(SIGCHLD, &sa1, NULL) == -1 )
		errExit( "Sigaction error" );
	
	char* buf = malloc( MAX_READ );
	Report* report = malloc( cnt*sizeof(Report) );
	int repindex = 0;
	struct timespec tstart1, tend1, tstart2, tend2;
	
	int sock_fd = connectSock( port, addr );
	/*
	int flagsock;
	if( (flagsock = fcntl(sock_fd, F_GETFL, 0)) == -1 )
		errExit( "fcntl error" );
	if( fcntl( sock_fd, F_SETFL, flagsock | O_NONBLOCK ) == -1 )
		errExit( "fcntl error" );
	*/

	struct timespec req, rem;
	timer_t timerid;
	if( !flagS )
	{
		timer_pick = 0;
		timerid = create_timer( r );
	}
	else
	{	
		req.tv_sec = (time_t)r;
		req.tv_nsec = 0;
	}

	int tmpcnt = cnt;
	int recvcnt = 0;

	while( recvcnt < cnt )
	{
		//development
		//if( write( 1, "0", 1 ) == -1 )
		//	errExit( "dev write error" );

		if( (tmpcnt > 0) && (timer_pick == 1) && (write(sock_fd, "aaaa", 4) > 0) )
		{
			--tmpcnt;
			
			//development
			if( write( 1, "%", 1 ) == -1 )
				errExit( "dev write error" );
			
			if( !flagS )
				timer_pick = 0;
			
			if( clock_gettime(CLOCK_REALTIME, &tstart1) == -1 )
				errExit( "clock_gettime error" );	
		}
		
		//development
		//if( write( 1, "1", 1 ) == -1 )
		//	errExit( "dev write error" );

		if( clock_gettime(CLOCK_REALTIME, &tstart2) == -1 )
			errExit( "clock_gettime error" );
		
		//TODO...	
		if( recv( sock_fd, buf, MAX_READ, MSG_DONTWAIT ) > 0 )
		{
			++recvcnt;
			
			//development
			if( write( 1, "#", 1 ) == -1 )
				errExit( "dev write error" );
			
			if( clock_gettime(CLOCK_REALTIME, &tend1) == -1 )
				errExit( "clock_gettime error" );
					
			report[repindex].md5sum = createMd5sum(buf);			
		}
		else
		{
			//development
			//if( write( 1, "2", 1 ) == -1 )
			//	errExit( "dev write error" );

			continue;
		}

		if( clock_gettime(CLOCK_REALTIME, &tend2) == -1 )
			errExit( "clock_gettime error" );
		
		report[repindex].delay1 = (tend1.tv_sec-tstart1.tv_sec)*1000000000 + (tend1.tv_nsec-tstart1.tv_nsec);
		report[repindex].delay2 = (tend2.tv_sec-tstart2.tv_sec)*1000000000 + (tend2.tv_nsec-tstart2.tv_nsec);
		repindex++;
		
		if( flagS )
		{
			while( nanosleep( &req, &rem ) == -1 )
			{
				req.tv_sec = rem.tv_sec;
				req.tv_nsec = rem.tv_nsec;
			}
		}
	}

	//report
	if( clock_gettime(CLOCK_REALTIME, &tend1) == -1 )
		errExit( "clock_gettime error" );
	if( clock_gettime(CLOCK_MONOTONIC, &tend2) == -1 )
		errExit( "clock_gettime error" );
	
	fprintf( stderr, "\n1. Wall time: %ld[s], %ld[ns] \t Monotonic: %ld[s], %ld[ns]\n", 
			tend1.tv_sec, tend1.tv_nsec, tend2.tv_sec, tend2.tv_nsec );
	fflush(stdout);
	fprintf( stderr, "2. Pid: %d \t address: %s\n", getpid(), addr );
	fflush(stdout);
	for( int i = 0 ; i < repindex; i++ )
		fprintf( stderr, "3.\n\ta) %ld[ns]\n\tb) %ld[ns]\n\tc) md5: %s\n", report[i].delay1, report[i].delay2, report[i].md5sum );
	fflush(stdout);
	
	if( !flagS )
		if( timer_delete(timerid) == -1 )
			errExit( "timer delete error" );

	if( close(sock_fd) == -1 )
		errExit( "close error" );
	
	//development
	write( 1, "\nsocket closed\n", 16 );

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
	char tmps[50] = { '\0' };
	char tmpr[50] = { '\0' };
	char* tmpcnt;
	while( (opt=getopt(argc, argv, "#:r:s:")) != -1 )
	{
		switch( opt )
		{
		case 's': strcpy(tmps, optarg); break;
		case 'r': strcpy(tmpr, optarg); break;
		case '#': tmpcnt = optarg; break;
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
			*port = *port*mult + (argv[argc-1][i]-'0');
		}
	}
	if( !flag2 )
	{
		*port = strtol( addr, NULL, 10 );
		strcpy( addr, "localhost" );
	}
	if( strlen(addr) > 15 )
		errExit( "adress error" );
	
	//cnt
	flag = 0;
	unsigned int L1 = 0, L2 = 0;
	for( int i = 0; i < strlen(tmpcnt); i++ )
	{
		if( tmpcnt[i] == ':' )
		{
			flag = 1;
			continue;
		}
		if( !flag )
			L1 = L1*mult + (tmpcnt[i]-'0');
		else
			L2 = L2*mult + (tmpcnt[i]-'0');
	}
	
	if( L2 <= L1 )
		*cnt = L1;
	else
		*cnt = randomizeL( L1, L2 );
	
	//r/s
	int flags = 0;
	if( !strcmp( tmpr, "" ) )
	{
		flags = 1;
		strcpy(tmpr, tmps);
	}

	flag = 0;
	flag2 = 0;
	double D1 = 0, D2 = 0;
	for( int i = 0; i < strlen(tmpr); i++ )
	{
		if( tmpr[i] == ':' )
		{
			flag = 1;
			flag2 = 0;
			mult = 10;
			continue;
		}
		if( tmpr[i] == '.' )
		{
			flag2 = 1;
			mult = 0.1;
			continue;
		}

		if( !flag )
		{
			if( !flag2 )
				D1 = D1*mult + (tmpr[i]-'0');
			else
			{
				D1 = D1 + (tmpr[i]-'0')*mult;
				mult *= 0.1;
			}
		}
		else
		{
			if( !flag2 )
				D2 = D2*mult + (tmpr[i]-'0');
			else
			{
				D2 = D2 + (tmpr[i]-'0')*mult;
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
	srand(time(NULL));
	return rand()%L2 + L1;
}

double randomizeD( double D1, double D2 )
{
	srand(time(NULL));
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

timer_t create_timer( double t )
{
	struct sigevent sev;
	timer_t timerid;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGCHLD;
	if( timer_create(CLOCK_REALTIME, &sev, &timerid) == -1 )
		errExit( "timer_create error" );

	struct itimerspec its;
	its.it_value.tv_sec = t;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = t;
	its.it_interval.tv_nsec = 0;

	if( timer_settime(timerid, 0, &its, NULL) == -1 )
		errExit( "timer_settime error" );

	return timerid;
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

