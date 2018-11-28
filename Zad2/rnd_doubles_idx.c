#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>

int main( int argc, char* argv[] )
{
	if( argc != 4 && argc != 6 )
	{
		printf( "Usage: %s -b <number> [-i <number>] <path>\n", argv[0] );
		return -1;
	}
	//getting args
	int opt;
	int block_num;
	int range = 32;
	while( (opt=getopt( argc, argv, "b:i:" )) != -1 )
	{
		switch( opt )
		{
		case 'b': block_num = atoi( optarg ); break;  //number of blocks of numbers
		case 'i': range = atoi( optarg ); break;      //number of ranges
		default: printf( "Usage: %s -b <number> [-i <number>] <path>\n", argv[0] ); return -1;
		}
	}
	//getting proper file size
	long long int fsize = (1<<14)*block_num*5;
	while( (1<<16)*block_num < fsize )
		block_num++;
	//casting int to char*
	char* buf = (char*)calloc( 50, 1 );
	sprintf( buf, "%d", block_num );
	//creating new argv to child process
	long long int i = 3;
	if( argc == 6 )
		i = 5;
	char* argv2[] = { "b.out", "-d", buf, "-o", argv[i], NULL };
	//creating fork
	pid_t pid = fork();
	//executing child process
	if( pid == 0 )
		execv( "/home/ukasz/Linux/Zad2/b.out", argv2 );
	else
		wait( NULL );
	//freeing memory - unnecessary buffer
	free( buf );
	//cutting file to proper size
	/*double bufD;
	int file2 = open( "file2", O_CREAT | O_RDWR, S_IRWXU | S_IRWXO | S_IRWXG );	//file with data
	int j = 0;
	while( j++ < fsize )
	{
		read( file, &bufD, sizeof(double) );
		write( file2, &bufD, sizeof(double) );
	}*/
	int file = open( argv[i], O_RDWR, S_IRWXU | S_IRWXO | S_IRWXG );
	truncate( argv[i], fsize * sizeof(double) );
	//program works until .gen-live exists and only if it's not existing at the beginning
	struct stat sb;
	if( stat( ".gen-live", &sb ) == 0 )
	{
		printf( "File .gen-live exists\n" );
		return -1;
	}
	//checking if file exists
	int livefile = open( ".gen-live", O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO ) ;
	struct timespec tim;
	tim.tv_sec = 1;
	tim.tv_nsec = 0;
	//clearing contents of output file
	/*close( file );
	remove( argv[i] );
	file = open( argv[i], O_CREAT | O_RDWR, S_IRWXU | S_IRWXO | S_IRWXG );*/
	//casting int to char*
	buf = (char*)calloc( 50, 1 );
	sprintf( buf, "%d", range );
	//creating child processes
	for( int k = 0; k < range; k++ )
	{
		char* buf2 = (char*)calloc( 5, 1 );
		sprintf( buf2, "%d", k );
		char* argv3[] = { "c.out", "-i", buf, "-b", buf2, "-o", "file2", argv[i], NULL };
		if( fork() == 0 )
		{
			execv( "/home/ukasz/Linux/Zad2/c.out", argv3 );
			exit( 1 );
		}
	}
	//loop check file existance
	while( stat( ".gen-live", &sb ) != -1 )
	{
		nanosleep( &tim, NULL );
	}

	//closing and deleting
	close( file );
	close( livefile );
	//remove( argv[i] );
	return 0;
}
