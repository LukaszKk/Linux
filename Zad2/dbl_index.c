#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <time.h>

int main( int argc, char* argv[] )
{
	if( argc != 8 )
	{
		printf( "Usage: %s -i <number_of_ranges> -b <range_number> -o <output file> <input file with data>", argv[0] );
		return -1;
	}
	int opt;
	int ranges;
	int range;
	while( (opt=getopt( argc, argv, "i:b:o:" )) != -1 )
	{
		switch( opt )
		{
		case 'i': ranges = atoi( optarg ); break;
		case 'b': range = atoi( optarg ); break;
		case 'o': break;
		default: printf( "Usage: %s -i <number_of_ranges> -b <range_number> -o <output file> <input file with data>", argv[0] ); return -1;
		}	
	}	
	
	double buf;
	int file = open( argv[7], O_RDONLY, S_IRWXU );
	int fileOut = open( argv[6], O_CREAT | O_WRONLY, S_IRWXU );
	if( ranges == 0 )
	{
		printf( "Wrong range\n" );
		return -1;
	}
	else if( ranges == 1 )
	{
		int i = 0;
		while( read( file, &buf, sizeof(double) ) > 0 )
		{
			char* buff = (char*)calloc( 50, 1 );
			sprintf( buff, "%d: %lg", i, buf );
			write( fileOut, buff, sizeof( buff ) );
			i++;
		}
	}
	else
	{
		double one_range = 1.7*pow(10,2*308/range);
		double range_max = DBL_MIN;
		double range_min = DBL_MIN;
		for( int i = 0; i < range; i++ )
			range_max += one_range;
		if( range_max < DBL_MIN + one_range )
			range_min = range_max - one_range;
		int i = 0;
		while( read( file, &buf, sizeof(double) ) > 0 )
		{
			if( buf > range_min && buf < range_max )
			{
				char* buff = (char*)calloc( 50, 1 );
				sprintf( buff, "%d:%lf", i, buf );
				write( fileOut, buff, sizeof( buff ) );
				write( fileOut, "\n", 1 );
			}
			i++;
		}
	}

	struct timespec tim;
	tim.tv_sec = 1;
	tim.tv_nsec = 0;
	struct stat sb;
	while( stat( ".gen-live", &sb ) != -1 )
	{
		nanosleep( &tim, NULL );
	}

	close( file );
	close( fileOut );

	return 0;
}
