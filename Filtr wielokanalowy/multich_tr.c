#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <poll.h>


void errExit( char* mes )
{
	write( STDERR_FILENO, mes, strlen(mes) );
	write( STDERR_FILENO, "\n", 1 );
	exit( EXIT_FAILURE );
}

typedef struct Pipes
{
	int pipe[2];
	int pipe2[2];
} Pipe;

int main( int argc, char* argv[] )
{
	//input check
	if( argc != 3 && argc != 5 )
	{
		printf( "Usage: %s -X <alphanumeric> [-f <data>]\n", argv[0] );
		return -1;
	}

	//param getting
	int opt;
	char alpha[11];
	char data[127];
	while( (opt=getopt(argc, argv, "X:f:")) != -1 )
	{
		switch( opt )
		{
		case 'X': 
			if( strlen(optarg) > 10 ) 
				printf( "To long string, only 10 taken\n" ); 
			strncpy(alpha, optarg, sizeof(alpha)-1);
		       	alpha[10] = '\0';	
			break;
		
		case 'f': 
			strncpy(data, optarg, sizeof(data)); 
			data[126] = '\0';
			break;
		}
	}
	
	//alphanumeric checking
	char* alc = alpha;
	for( int i = 0; i < strlen(alpha) && *alc != '\0'; i++, alc++ )
		if( (*alc < 'a' || *alc >'z') && (*alc < 'A' || *alc > 'Z') && (*alc < '0' || *alc > '9') )
			errExit( "X param isn't alphanumeric" );
	
	//open data
	int fd = STDIN_FILENO;
	if( argc == 5 )
		fd = open( data, O_RDONLY | O_NONBLOCK );
	if( fd == -1 )
		errExit( "open_err" );
	
	//create pipes
	Pipe pipefd[strlen(alpha)];

	//foreach char from X
	pid_t pcid;
	int i = 0;
	for( char* c = alpha; *c != '\0' && i < strlen(alpha); c++, i++ )
	{
		//create pipe
		if( pipe(pipefd[i].pipe) == -1 )
			errExit( "Pipe_err" );
		
		//create pipe2
		if( pipe(pipefd[i].pipe2) == -1 )
			errExit( "Pipe_err" );
		
		//create child
		pcid = fork();
		if( pcid == -1 )
			errExit( "fork_err" );
		else if( pcid == 0 )
		{
			//connect pipe write to stdout
			if( dup2( pipefd[i].pipe2[1], STDOUT_FILENO ) == -1 )
				errExit( "Pipe_to_stdout_err" );	
			
			if( close( pipefd[i].pipe2[1] ) == -1 )
				errExit( "close_err" );
		
			//close read
			if( close( pipefd[i].pipe2[0] ) == -1 )
				errExit( "close_err" );		
			
			//connect pipe read to stdin
			if( dup2( pipefd[i].pipe[0], STDIN_FILENO ) == -1 )
				errExit( "Pipe_to_stdin_err" );
				
			//close write
			if( close( pipefd[i].pipe[1] ) == -1 )
				errExit( "close_err" );
			
			//read pipe
			char bufr;
			while( 1 )
			{
				struct pollfd fds;
				fds.fd = pipefd[i].pipe[0];
				fds.events = POLLIN;
				fds.revents = 0;
				
				//wait for some data
				if( poll( &fds, 1, 3*60*1000 ) == -1 )
					errExit( "poll_err" );

				if( fds.revents & POLLIN )
				{
					if( read( fds.fd, &bufr, 1 ) == -1 )
						errExit( "read_err" );
	
					//run tr
					if( execl( "tr", "tr", c, (char*)NULL ) == -1 )
						errExit( "tr_err" );
				}
			
				if( fds.revents & POLLHUP )
					break;
			}
			
			//exit child
			exit( EXIT_SUCCESS );
		}
		else
		{
			//close read
			if( close( pipefd[i].pipe[0] ) == -1 )
				errExit( "close_err" );
		
			//close write
			if( close( pipefd[i].pipe2[1] ) == -1 )
				errExit( "close_err" );
		}
	}

	//position in col create
	int position_x[strlen(alpha)];
	for( int j = 0; j < strlen(alpha); j++ )
	{
		position_x[j] = 0;
	}
	
	//nanosleep struct
	struct timespec req, rem;
	
	//clear screen
	printf( "\033[2J" );
	fflush( stdout );

	//read from file data block
	while( 1 )
	{
		//read data
		char bufw[10] = { "" };
		if( read( fd, &bufw, 10 ) > 0 )
		{
			//send data block to pipes
			for( int j = 0; j < strlen(alpha); j++ )
			{	
				if( write( pipefd[j].pipe[1], bufw, strlen(bufw) ) == -1 )
					errExit( "write_err" );
			}
		}

		//delay
		req.tv_sec = 0;
		req.tv_nsec = 1000000000/4*3;
		while( nanosleep( &req, &rem ) == -1 )
		{
			req.tv_sec = rem.tv_sec;
			req.tv_nsec = rem.tv_nsec;
		}

		//read from pipes
		char c;
		for( int j = 0; j < strlen(alpha); j++ )
		{
			struct pollfd fds;
			fds.fd = pipefd[j].pipe2[0];
			fds.events = POLLIN;
			fds.revents = 0;
			
			//nonblocking read
			if( poll( &fds, 1, 0 ) == -1 )
				errExit( "poll_err" );
		
			if( fds.revents & POLLIN )
			{			
				if( read( pipefd[j].pipe2[0], &c, 1 ) > 0 )
				{
					//if more then 78 charactes then erase line
					if( ++position_x[j] > 78 )
					{
						position_x[j] = 0;
						printf( "\033[2K" );
						fflush( stdout );
					}
					printf( "\033[%d;1H", (j+1)*2 );
					printf( "\033[@" );
					printf( "\033[%d;1H", (j+1)*2 );
					fflush( stdout );
					
					//print char
					if( write( STDOUT_FILENO, &c, 1 ) == -1 )
						errExit( "write_err" );
				}
			}
		
			if( fds.revents & POLLHUP )
				continue;
		}
	}

	//close data
	if( argc == 5 )
		if( close(fd) == -1 )
			errExit( "close_err" );
	
	return 0;
}
