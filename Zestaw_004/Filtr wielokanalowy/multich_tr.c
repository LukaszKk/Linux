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

struct Pipes
{
	int pipe[2];
};

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
		fd = open( data, O_RDONLY );
	if( fd == -1 )
		errExit( "open_err" );
	
	//create pipes
	struct Pipes pipefd[strlen(alpha)];

	//foreach char from X
	pid_t pcid;
	int i = 0;
	for( char* c = alpha; *c != '\0' && i < strlen(alpha); c++, i++ )
	{
		//create pipe
		if( pipe(pipefd[i].pipe) == -1 )
			errExit( "Pipe_err" );
		
		write( STDOUT_FILENO, "2", 1 );
		
		//create child
		pcid = fork();
		if( pcid == -1 )
			errExit( "fork_err" );
		else if( pcid == 0 )
		{
			write( STDOUT_FILENO, "2", 1 );
			
			//close write
			if( close( pipefd[i].pipe[1] ) == -1 )
				errExit( "close_err" );


			write( STDOUT_FILENO, "2", 1 );


			//connect pipe read to stdin
			if( dup2( pipefd[i].pipe[0], STDIN_FILENO ) == -1 )
				errExit( "Pipe_to_stdin_err" );

			//connect pipe write to stdout
			//if( dup2( pipefd[i].pipe[1], STDOUT_FILENO ) == -1 )
			//	errExit( "Pipe_to_stdin_err" );	
			//write( STDOUT_FILENO, "2", 1 );
	

			write( STDOUT_FILENO, "2", 1 );
		
				
			//wait for some data
			struct pollfd fds;
			fds.fd = pipefd[i].pipe[0];
			fds.events = POLLIN;
			fds.revents = 0;
			if( poll( &fds, 1, -1 ) == -1 )
				errExit( "poll_err" );


			write( STDOUT_FILENO, "2", 1 );
			

			//read pipe
			char bufr[10];
			//while( read( pipefd[i].pipe[0], &bufr, sizeof(bufr) ) > 0 )
			while( 1 )
			{
			if( fds.revents == POLLIN )
			{
				if( read( fds.fd, &bufr, sizeof(bufr) ) == -1 )
					errExit( "read_err" );
				

				write( STDOUT_FILENO, "2", 1 );
				

				//run tr
				if( execl( "tr", "tr", c, (char*)NULL ) == -1 )
					errExit( "tr_err" );
				
				//new line for every child
				if( write( STDOUT_FILENO, "\n", 1 ) == -1 )
					errExit( "write_child_err" );
			}
			}
		
			//after all new line
			if( write( STDOUT_FILENO, "\n", 1 ) == -1 )
				errExit( "write_child_err" );
			
			//exit child
			exit( EXIT_SUCCESS );
		}
		else
		{
			//close read
			if( close( pipefd[i].pipe[0] ) == -1 )
				errExit( "close_err" );
		}
	}

	//wait some time
	struct timespec req, rem;
	req.tv_sec = 1;
	req.tv_nsec = 0;
	while( nanosleep( &req, &rem ) == -1 )
	{
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}
	
	//read from file data block
	char bufw[10];
	while( read( fd, &bufw, sizeof(bufw) ) > 0 )
	{
		//send data block to pipes
		for( int i = 0; i < strlen(alpha); i++ )
		{
			
			write( STDOUT_FILENO, "2", 1 );
			

			if( write( pipefd[i].pipe[1], &bufw, sizeof(bufw) ) == -1 )
				errExit( "write_err" );
		}
	}

	//close data
	if( argc == 5 )
		if( close(fd) == -1 )
			errExit( "close_err" );
	
	//wait for all children
	//while( waitid( P_ALL, 0, NULL, WEXITED ) == 0 );
	//	errExit( "waitid_err" );
	
	while( 1 )
	{

	}

	return 0;
}
