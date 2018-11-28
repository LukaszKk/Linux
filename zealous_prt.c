#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>


int InputCheck( int argc, char* argv[] );
void GetOpt( float* e, int argc, char* argv[] );

void Children( int* pidg, int count );

struct Queue									//globalna kolejka
{
	siginfo_t info;
	struct Queue* next;
};


void printQueue( struct Queue* queue );

struct Queue* allocMem();							//alokacja pamieci na elemencie
void push( struct Queue** queue, siginfo_t info );				//dodaj na kolejnym elemencie
void pop( struct Queue** queue );						//usun kolejny element
void freeQueue( struct Queue** queue );

void SaHandler( int sig_num, siginfo_t* info, void * v );
void printKilled( siginfo_t info );

struct Queue* queue;
volatile int count = 0;
volatile int stopped = 0;
volatile int flag = 0;

int main( int argc, char* argv[] )
{
	if( InputCheck( argc, argv ) )
		return 1;
	float e = exp( 1 );
	int pidg = strtol( argv[1], NULL, 10 );					//pid grupy
	count = strtol( argv[2], NULL, 10 );					//ilosc potomkow
	GetOpt( &e, argc, argv );						//wczytaj param
	
	/*
	siginfo_t tmp;
	struct Queue* queue = allocMem();
	struct Queue* wsk = queue;
	tmp.si_pid = 12;
	push( &wsk, tmp );
	tmp.si_pid = 13;
	wsk = wsk->next;
	push( &wsk, tmp );
	tmp.si_pid = 14;
	wsk = wsk->next;
	push( &wsk, tmp );
	printQueue( queue );
	pop( &wsk );
	printQueue( queue );
	pop( &queue );
	printQueue( queue );

	return 0;
	*/


	queue = allocMem();							//tworzenie kolejki
	
	
	/*struct sigaction sa;
	sa.sa_sigaction = &SaHandler;
	sa.sa_flags = SA_SIGINFO;
	sigaction( SIGCHLD, &sa, NULL );*/
	Children( &pidg, count );						//stworz potomkow, funkcja opcjonalna
	

	siginfo_t info;
	int i = count;
	while( i > 0 )
	{
		if( waitid( P_PGID, pidg, &info, WEXITED | WSTOPPED | WNOHANG ) == 0 )	//czekaj az wszystkie procesy zostana zakonczone lub wstrzymane
			i--;
	}
	
	struct sigaction sa;
	sa.sa_sigaction = &SaHandler;
	sa.sa_flags = SA_SIGINFO;
	if( sigaction( SIGCHLD, &sa, NULL ) != 0 )				//zmiana SIGCHLD
		return -1;

	sigset_t set;								//inicjalizacja zbioru sygnalow
	if( sigemptyset( &set ) != 0 )
		return -1;
	if( sigaddset( &set, SIGCONT ) != 0 )
		return -1;
	if( sigaddset( &set, SIGTERM ) != 0 )
		return -1;
	if( sigaddset( &set, SIGSTOP ) != 0 )
		return -1;
	
	struct timespec ts;
	ts.tv_sec = e/(M_PI*M_PI);
	ts.tv_nsec = 0;
	while( 1 )
	{
		//if( killpg( SIGCONT, pidg ) != 0 )				//budzenie
			//return -1;
		Children( &pidg, count );						//stworz potomkow, funkcja opcjonalna
		while( 1 )
		{
			//if( sigwaitinfo( &set, &info ) == -1 )			//pasywne oczekiwanie
			//	return -1;
			if( waitid( P_PGID, pidg, &info, WEXITED | WSTOPPED | WNOHANG ) == 0 )
			{
				printf( "I am here\n" );
				if( info.si_status == CLD_EXITED )		//drukowanie nekrologu
				{
					printKilled( info );
				}	
			}
			if( count == 0 )					//nie ma zywych, koniec programu
			{
				freeQueue( &queue );
				return 0;
			}
			if( flag == 1 )						//nie ma aktywnych
			{
				flag = 0;
				if(nanosleep( &ts, 0 ) != 0)			//to spij
					return -1;
				break;
			}
		}
	}

	freeQueue( &queue );
	return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------

int InputCheck( int argc, char* argv[] )
{
	if( argc >= 3 && argc <= 5 )
		return 0;
	printf( "Usage: %s <pid_grp> <number of children> [-t [<float>]]\n", argv[0] );
	return 1;
}

void GetOpt( float* e, int argc, char* argv[] )
{
	int opt;
	while( (opt=getopt( argc, argv, ":t:" )) != -1 )
	{
		if( opt == 't' )
			*e = strtof( optarg, 0 );
	}
}

void Children( int* pidg, int count )
{
	pid_t pid;
	for( int i = 0; i < count; i++ )
	{
		if( (pid=fork()) == 0 )
		{
			setpgid( pid, *pidg );
			exit( 0 );
		}
		*pidg = getpgid(pid);
	}
}



void printQueue( struct Queue* queue )
{
	struct Queue* wsk = queue->next;
	while( wsk->next != NULL )
	{
		printf( "%d\t", wsk->info.si_pid );
		wsk = wsk->next;
	}
	printf( "%d\n", wsk->info.si_pid );
}

struct Queue* allocMem()							//alokacja pamieci na elemencie
{
	struct Queue* wsk = (struct Queue*)malloc( sizeof(struct Queue) );
	if( !wsk )
	{
		printf( "Memory issue: resizing queue\n" );
	}
	memset( wsk, 0, sizeof(struct Queue) );
	return wsk;
}

void push( struct Queue** queue, siginfo_t info )				//dodaj na kolejnym elemencie
{
	struct Queue* wsk = allocMem();
	wsk->info = info;
	wsk->next = NULL;
	(*queue)->next = wsk;
}

void pop( struct Queue** queue )						//usun kolejny element
{
	if( (*queue)->next == NULL )
		return;
	struct Queue* wsk = (*queue)->next;
	free( (*queue)->next );
	(*queue)->next = wsk->next;
}

void freeQueue( struct Queue** queue )
{
	struct Queue* wsk = *queue;
	while( (*queue)->next != NULL )
	{
		wsk = wsk->next;
		free( *queue );
		*queue = wsk;
	}
	free( *queue );
}


void SaHandler( int sig_num, siginfo_t* info, void * v )
{
	struct Queue* wsk = queue;
	while( wsk->next != NULL )
		wsk = wsk->next;
	push( &wsk, *info );
	switch( sig_num )
	{
		case SIGTERM: count -= 1; info->si_code = CLD_EXITED; break;
		case SIGSTOP: stopped += 1; break;
	}
	if( stopped == count )
		flag = 1;
}

void printKilled( siginfo_t info )
{
	struct Queue* wsk = queue;
	while( (info.si_pid != wsk->info.si_pid)  && (wsk->next != NULL) )
		wsk = wsk->next;
	printf( "pid: %d, status: %d, reason: %d\n", wsk->info.si_pid, wsk->info.si_status, wsk->info.si_code );
	pop( &wsk );
}
