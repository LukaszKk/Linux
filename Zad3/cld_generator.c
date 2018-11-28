#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <wait.h>
#include <string.h>

void GetOpt( int argc, char* argv[], char** Pname, char** Cname )
{
	int opt;
	while( (opt=getopt(argc, argv, ":p:c:")) != -1 )
	{
		switch(opt)
		{
		case 'p': *Pname = optarg; break;
		case 'c': *Cname = optarg; break;
		default: break;
		}
	}
}

char* floatToString( float P )
{
	char* buf = (char*)calloc( 30, sizeof(char) );
	sprintf( buf, "%f", P );
	return buf;
}

char* pidToString( pid_t P )
{
	char* buf = (char*)calloc( 30, sizeof(char) );
	sprintf( buf, "%d", P );
	return buf;
}

char* intToString( int P )
{
	char* buf = (char*)calloc( 30, sizeof(char) );
	sprintf( buf, "%d", P );
	return buf;
}

int CreateChild( int argc, char* argv[], char* Cname, pid_t* pidg, float* smallest )
{
	srand(time(NULL));
	float P;
	int flag_first = 0;
	char* wsk;
	pid_t pid;
	int count = 0;											//ilosc potomkow
	for( int i = 0; i < argc; i++ )
	{
		P = strtof( argv[i], &wsk );
		if( fpclassify( P ) == FP_NORMAL )							//czy prawidlowa liczba float
		{
			if( flag_first == 0 )
				*smallest = P;
			else if( P < *smallest )
				*smallest = P;								//zapisywanie najmniejszego float
			if( (pid=fork()) == 0 )								//tworzenie potomka
			{	
				if( flag_first == 0 )
				{
					if( setpgid( pid, 0 ) != 0 )					//utworz grupe
						return 0;
				}
				else
				{
					if( setpgid( pid, *pidg ) != 0 )				//dolacz do grupy
						return 0;
				}
				if( Cname != NULL )							//jesli byl podany param -c
				{
					char* buf = floatToString(P);
					execl( Cname, buf, NULL );					//to uruchom program
					exit( 1 );
				}
				else
				{
					struct timespec ts;
					ts.tv_sec = 0;
					ts.tv_nsec = P * 10000000;	
					if( nanosleep( &ts, 0 ) != 0 )					//jesli nie to spij
						return 0;
					int ret = rand() % 127;						//i losuj
					exit( ret );
				}
			}
			if( flag_first == 0 )
				*pidg = getpgid(pid);
			count++;
			flag_first = 1;
		}
	}
	return count;
}

void LookThrough( char* PName, pid_t pidg, int child_count, float smallest )
{
	if( PName != NULL )												//jesli byl podany param -p
	{
		char* buf1 = pidToString( pidg );
		char* buf2 = intToString( child_count );
		char* buf3 = floatToString( smallest );
		execl( PName, buf1, buf2, "-t", buf3, NULL );								//to wykonywanie programu
	}
	else														//jesli nie
	{
		siginfo_t info;
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = smallest/2 * 10000000;
		while( child_count > 0 )										//odczytaj wszystkich status
		{
			if( waitid( P_PGID, pidg, &info, WEXITED | WSTOPPED | WCONTINUED | WNOHANG ) == 0 )		//czytaj status
			{
				child_count--;
				printf( "PID: %d, Status: %d, Death: %s\n", info.si_pid, info.si_status, strsignal(info.si_code) );//wypisz informacje
				if( nanosleep( &ts, 0 ) != 0 )								//czekaj
					return;
			}
			else
			{
				char buf[13] = "Waitid error\n";
				if( write( STDERR_FILENO, &buf, sizeof(buf) ) == -1 )					//wypisz blad
					return;
			}
		}
	}
}	


int main( int argc, char* argv[] )
{
	char* Pname = NULL;
	char* Cname = NULL;
	GetOpt( argc, argv, &Pname, &Cname );
	pid_t pidg = 12111;
	float smallest;
	int child_count = CreateChild( argc, argv, Cname, &pidg, &smallest );
	LookThrough( Pname, pidg, child_count, smallest );

	exit( 0 );
	return 0;
}
