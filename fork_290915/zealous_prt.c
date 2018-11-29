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

void SaHandler( int sig_num );
void printKilled();

struct Queue* queue;
volatile int count = 0;
volatile int stopped = 0;
volatile int flag = 0;
/*volatile*/ int pidg = 0;

int main( int argc, char* argv[] )
{
	if( InputCheck( argc, argv ) )
		return 1;
	float e = exp( 1 );
	pidg = strtol( argv[1], NULL, 10 );					//pid grupy
	count = strtol( argv[2], NULL, 10 );					//ilosc potomkow
	GetOpt( &e, argc, argv );						//wczytaj param	
	queue = allocMem();							//tworzenie kolejki
	
	
	//Children( &pidg, count );
	

	siginfo_t info;
	int i = count;
	while( i > 0 )
	{
		if( waitid( P_PGID, pidg, &info, WEXITED | WSTOPPED | WNOHANG ) == 0 )	//czekaj az wszystkie procesy zostana zakonczone lub wstrzymane
			i--;
	}
	
	struct sigaction sa;
	sa.sa_handler = &SaHandler;
	if( sigaction( SIGCHLD, &sa, NULL ) != 0 )				//zmiana SIGCHLD
		return -1;

	sigset_t set;								//inicjalizacja zbioru sygnalow
	struct timespec ts;
	ts.tv_sec = e/(M_PI*M_PI);
	ts.tv_nsec = 0;
	Children( &pidg, count );
	while( 1 )
	{
		if( killpg( SIGCONT, pidg ) != 0 )				//budzenie
			return -1;
		while( 1 )
		{
			if( sigwaitinfo( &set, &info ) != 0 )			//pasywne oczekiwanie
				return -1;
			if( flag == 1 )
			{
				printKilled();						//drukowanie nekrologow
				if( count <= 0 )					//nie ma zywych, koniec programu
				{
					freeQueue( &queue );
					return 0;
				}
				if( stopped <= count )					//nie ma aktywnych
				{
					flag = 0;
					if(nanosleep( &ts, 0 ) != 0)			//to spij
						return -1;
					flag = 0;
					break;
				}
				flag = 0;
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
	int f = 0;
	for( int i = 0; i < count; i++ )
	{
		if( (pid=fork()) == 0 )
		{
			if( f == 0 )
				setpgid( pid, pid );
			setpgid( pid, *pidg );
			exit( 1 );
		}
		f = 1;
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
	if( wsk->next != NULL )
		(*queue)->next = wsk->next;
	else
		(*queue)->next = NULL;
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


void SaHandler( int sig_num )
{
	siginfo_t info;
	if( flag == 0 )
	{
		while( waitid( P_PGID, pidg, &info, WEXITED | WSTOPPED | WNOHANG ) == 0 )
		{
			struct Queue* wsk = queue;
			while( wsk->next != NULL )
				wsk = wsk->next;
			switch( info.si_code )
			{
			case CLD_KILLED: count -= 1; info.si_code = CLD_EXITED; break;
			case CLD_STOPPED: stopped += 1; break;
			}
			push( &wsk, info );
		}
		flag = 1;
	}
	//if( stopped >= count )
	//	flag = 1;
}

void printKilled()
{
	struct Queue* wsk = queue;
	while( wsk->next != NULL )
	{
		if( wsk->info.si_code == CLD_EXITED )
		{
			printf( "pid: %d, status: %d, reason: %d\n", wsk->info.si_pid, wsk->info.si_status, wsk->info.si_code );
			pop( &wsk );
		}
		wsk = wsk->next;
	}
}
