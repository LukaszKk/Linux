#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>


int main( int argc, char* argv[] )
{
	if( argc != 2 && argc != 4 )
	{
		printf( "Usage: %s [-b <numer>] <infile path>\n", argv[0] );
		return -1;
	}
	//reading parameters
	int opt;
	int range = 64;
	while( (opt=getopt(argc, argv, "b:")) != -1 )
	{
		switch( opt )
		{
			case 'b': range = atoi( optarg ); break;
			default: printf( "Usage: ./%s [-b <numer>] <infile path>\n", argv[0] ); return -1;
		}
	}
	int i;
	if( argc == 2 )
		i = 1;
	else
		i = 3;
	int file = open( argv[i], O_RDONLY );
	if( file == -1 )
		return -1;

	//printing bar chart
	printf( "# cnt %d\n", i );
	printf( "# bins %d\n", range );
	
	if( range <= 0 )
		printf( "Wrong range\n" );
	else if( range == 1 )
	{
		struct stat sb;
		stat( argv[i], &sb );
		printf( "0: %ld\n", sb.st_size/sizeof(double) );
	}
	else
	{
		int* numInRange = (int*)calloc( range, sizeof(int) );
		int flag;
		double j;
		double buf;
		double one_range = 1.7*pow(10,2*308/range);
		while( read( file, &buf, sizeof(double) ) > 0 )
		{
			j = DBL_MIN + one_range;
			flag = 0;
			for( int k = 0; k < range - 1; j += one_range, k++ )
			{
				if( buf < j )
				{
					numInRange[k]++;
					flag = 1;
					break;
				}
			}
			if( flag == 0 )
				numInRange[range-1]++;
		}
		for( int k = 0; k < range; k++ )
			printf( "%d: %d\n", k, numInRange[k] );
		free( numInRange );
	}

	close( file );
	return 0;
}
