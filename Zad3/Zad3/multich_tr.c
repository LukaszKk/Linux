#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


void errExit( char* mes );

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
	char data[51];
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
			data[50] = '\0';
			break;
		}
	}
	
	//alphanumeric checking
	char* alc = alpha;
	for( int i = 0; i < strlen(alpha) && *alc != '\0'; i++, alc++ )
		if( (*alc < 'a' || *alc >'z') && (*alc < 'A' || *alc > 'Z') && (*alc < '0' || *alc > '9') )
			errExit( "X param isn't alphanumeric" );
	
	//foreach char from X
	pid_t pid;
	for( char* c = alpha; *c != '\0'; c++ )
	{
		//create child
		pid = fork();
		if( pid == -1 )
			errExit( "fork_err" );
		if( pid == 0 )
		{
			//run tr
			if( execl( "tr", "tr", c, (char*)NULL ) == -1 )
				errExit( "tr_err" );

			//exit child
			exit( EXIT_SUCCESS );
		}
	}
	
	if( waitid( P_ALL, 0, NULL, WEXITED ) == -1 )
		errExit( "waitid_err" );
	

	printf( "\n" );
	return 0;
}


void errExit( char* mes )
{
	perror( mes );
	exit( EXIT_FAILURE );
}
