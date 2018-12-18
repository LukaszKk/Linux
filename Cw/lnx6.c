#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

void errExit(char* str);
void sigAct( int, siginfo_t *, void * );

float num;
timer_t tid = 0;

int main( int argc, char* argv[] )
{
	if( argc == 1 )
	{
		printf( "Usage: %s <float> <float> ...\n", argv[0] );
		return -1;
	}
	if( argc > 5 )
		printf( "To much args\n" );
	
	struct sigevent sevp;
	sevp.sigev_notify = SIGEV_SIGNAL;
	sevp.sigev_signo = SIGRTMAX - 5;

	struct sigaction sa;
	sa.sa_sigaction = sigAct;
	sa.sa_flags = SA_SIGINFO;
	
	if( sigaction( SIGRTMAX - 5, &sa, NULL ) == -1 )
	       errExit("sigaction create");	

	struct timespec ts;
	ts.tv_sec = 10;
	ts.tv_nsec = 0;
	struct timespec tr;

	for( int i = 0; i < 4 && i < argc-1; i++ )
	{
		num = strtof(argv[i+1], NULL);	
		sevp.sigev_value.sival_int = i;
		if( timer_create( CLOCK_MONOTONIC, &sevp, &tid ) == -1 )
			errExit("timer_create");
		
		struct itimerspec its;
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		its.it_value.tv_sec = num;
		its.it_value.tv_nsec = 0;
	
		if( timer_settime( tid, 0, &its, NULL ) == -1 )
			errExit("timer_settime");
		
		while( nanosleep(&ts, &tr) == -1 )
		{
			ts.tv_sec = tr.tv_sec;
			ts.tv_nsec = tr.tv_nsec;
		}

		if( timer_delete(tid) == -1 )
			errExit("timer_delete");
	}


	printf( "\n" );
	return 0;
}

void errExit(char* str)
{
	perror( str );
	exit( EXIT_FAILURE );
}

void sigAct( int sig_num, siginfo_t * sinfo, void * p )
{
	printf( "Timer %d gave signal\n", sinfo->si_value.sival_int );
	
	/*struct itimerspec its;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = num;
	its.it_value.tv_nsec = 0;
	
	if( timer_settime( tid, 0, &its, NULL ) == -1 )
		errExit("timer_settime");
	*/
}
