#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


void errExit( char* mes );

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
	char data[127] = "data";
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
	int fd = open( data, O_RDONLY );
	if( fd == -1 )
		errExit( "open_err" );
	
	//create pipes
	struct Pipes pipefd[sizeof(alpha)];
	for( int i = 0; i < sizeof(alpha)-1; i++ )
	{
		//create pipe
		if( pipe(pipefd[i].pipe) == -1 )
			errExit( "Pipe_err" );
	}

	//foreach char from X
	pid_t pid;
	int i = 0;
	for( char* c = alpha; *c != '\0' && i < sizeof(alpha)-1; c++, i++ )
	{
		//create child
		pid = fork();
		if( pid == 0 )
		{
			//connect pipe read to stdin
			if( dup2( pipefd[i].pipe[0], STDIN_FILENO ) == -1 )
				errExit( "Pipe_to_stdin_err" );
			
			//connect pipe write to stdout
			//if( dup2( pipefd[i].pipe[1], STDOUT_FILENO ) == -1 )
			//	errExit( "Pipe_to_stdin_err" );	
			//write( STDOUT_FILENO, "2", 1 );
		
			//read pipe
			char bufr[10];
			while( read( pipefd[i].pipe[0], &bufr, sizeof(bufr) ) > 1 )
			{
				//run tr
				if( execl( "tr", "tr", c, (char*)NULL ) == -1 )
					errExit( "tr_err" );
				
				//new line for every child
				if( write( STDOUT_FILENO, "\n", 1 ) == -1 )
					errExit( "write_child_err" );
			}	
		
			//after all new line
			if( write( STDOUT_FILENO, "\n", 1 ) == -1 )
				errExit( "write_child_err" );
			
			//exit child
			exit( EXIT_SUCCESS );
		}
		else if( pid == -1 )
			errExit( "fork_err" );
	}
	
	//read from file data block
	char bufw[10];
	while( read( fd, &bufw, sizeof(bufw) ) > 0 )
	{
		//send data block to pipes
		for( int i = 0; i < sizeof(alpha)-1; i++ )
		{
			if( write( pipefd[i].pipe[1], &bufw, sizeof(bufw) ) == -1 )
				errExit( "write_err" );
		}
	}

	//close data
	if( close(fd) == -1 )
		errExit( "close_err" );
	
	//wait for all children
	while( waitid( P_ALL, 0, NULL, WEXITED ) == 0 );
		//errExit( "waitid_err" );
	
	//print blank line
	printf( "\n" );
	return 0;
}


void errExit( char* mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}
