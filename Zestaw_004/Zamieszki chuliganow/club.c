#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>


void errExit( char * s )
{
	write( STDERR_FILENO, s, strlen(s) );
	write( STDERR_FILENO, "\n", 1 );
	exit( EXIT_FAILURE );
}

void HandlerUsr( int, siginfo_t*, void* );
void HandlerChld( int, siginfo_t*, void* );
volatile int signal_pid = 0;
volatile int signal_count = 0;

int main( int argc, char* argv[] )
{
	//input check
	if( argc == 1 )
	{
		printf( "Usage: %s [N:F:T] [N:F:T] ... (integer:float:string)\n", argv[0] );
		errExit( "Input_err" );
	}
	
	//create new group
	pid_t pid = getpid();
	if( setpgid( pid, pid ) == -1 )
		errExit( "setpgid_err" );

	//read params
	int allChld = 0;
	int N = 0;
	float F = 0;
	char * T;
	char * split;
	for( int i = 1; i < argc; i++ )
	{
		split = strtok( argv[i], ":" );
		if( split == NULL ) errExit( "param_err, run without param to see usage" );
		N = strtol( split, NULL, 10 );
		allChld += N;
		split = strtok( NULL, ":" );
		if( split == NULL ) errExit( "param_err, run without param to see usage" );
		F = strtof( split, NULL );
		T = strtok( NULL, ":" );
		if( T == NULL ) errExit( "param_err, run without param to see usage" );
		char buf[50];
		sprintf( buf, "%f", F );

		pid_t pcid;
		//create children
		for( int j = 0; j < N; j++ )
		{
			pcid = fork();
			if( pcid == -1 )
				errExit( "fork_err" );
			if( pcid == 0 )
			{
				//add to group
				if( setpgid( pcid, pid ) == -1 )
					errExit( "setpgid_err" );
				
				//run hools
				if( execl( "hools", "hools", "-d", buf, T, (char*)NULL ) == -1 )
					errExit( "exec_err" );
				
				//exit child
				exit( EXIT_SUCCESS );
			}
		}
	}
	
	//signals
	struct sigaction actUsr1, actUsr2, actChld;
	actUsr2.sa_handler = SIG_IGN;
	actUsr1.sa_sigaction = &HandlerUsr;
	actUsr1.sa_flags = SA_SIGINFO;
	actChld.sa_sigaction = &HandlerChld;
	actChld.sa_flags = SA_SIGINFO;
	
	if( sigaction( SIGUSR1, &actUsr1, NULL ) == -1 )
		errExit( "sigaction_err" );

	if( sigaction( SIGUSR2, &actUsr2, NULL ) == -1 )
		errExit( "sigaction_err" );

	if( sigaction( SIGCHLD, &actChld, NULL ) == -1 )
		errExit( "sigaction_err" );
	
	
	//give some time to do everything
	struct timespec req, rem;
	req.tv_sec = 1;
	req.tv_nsec = 0;
	while( nanosleep( &req, &rem ) == -1 )
	{
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}


	//send signal
	if( kill( -pid, SIGUSR2 ) == -1 )
		errExit( "killpg_err" );

	while( 1 )
	{
		if( signal_count == allChld )
			break;
	}

	return 0;
}


void HandlerUsr( int signum, siginfo_t * siginfo, void * p )
{
	signal_pid = siginfo->si_pid;
	char buf[50] = { "" };
	char buf2[50] = { "" };
	sprintf( buf2, "%d", siginfo->si_pid );
	strcat( buf, "Process: " );
	strcat( buf, buf2 );
	strcat( buf, " gave signal SIGUSR\n" );
	if( write( STDOUT_FILENO, buf, strlen(buf) ) == -1 )
		errExit( "write_err" );
}

void HandlerChld( int signum, siginfo_t * siginfo, void* p )
{
	signal_count++;
	char buf[50] = { "" };
	sprintf( buf, "%d", siginfo->si_pid );
	strcat( buf, ": killed\n" );
	if( write( STDOUT_FILENO, buf, strlen(buf) ) == -1 )
		errExit( "write_err" );
	
	if( signal_pid == siginfo->si_pid )
	{
		char buf2[50] = { "" };
		sprintf( buf2, "%d", siginfo->si_pid );
		strcat( buf2, ": Medal Darwina\n" );
		if( write( STDOUT_FILENO, buf2, strlen(buf2) ) == -1 )
			errExit( "write_err" );
	}
}
