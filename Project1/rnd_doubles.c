#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <linux/random.h>

#define MAX 1<<21 //bufor size
#define FMAX 1<<16 //file max size with outfile_size = 0

int main( int argc, char* argv[] )
{
	if( argc != 5 )
	{
		printf( "ARGC: Usage %s -d <output file size> -o <path to output file>\n", argv[0] );
		return -1;
	}
	int opt;
	int outfile_size = 0;
	char outfile_path[20];
	while( (opt = getopt( argc, argv, "d:o:")) != -1 )
		switch( opt )
		{
		case 'd': outfile_size = atoi( optarg ); break;
		case 'o': strcpy( outfile_path, optarg ); break;
		default: {
			  	printf( "Usage %s -d <output file size> -o <path to output file>\n", argv[0] );
				return -1;
			}
		}
	int rand = open( "/dev/urandom", O_RDONLY );
	int outfile = open( outfile_path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXO | S_IRWXG);
	double* buf = (double*)calloc( MAX, sizeof(double) );
	long unsigned i = 0;
	long unsigned maxSize = ((long unsigned)(FMAX)*outfile_size);
	long unsigned bufSize = (long unsigned)(MAX);
	if( (i+(double)(MAX)) > maxSize )
		bufSize = maxSize - i;
	while( i < maxSize )
	{
		read( rand, buf, bufSize );
		for( long unsigned j = 0; j < sizeof(buf); j++ )
		{
			if( fpclassify( buf[j] ) == FP_NORMAL )
				write( outfile, &buf[j], sizeof(buf[j]) );
			else
			{
				read( rand, &buf[j], sizeof(buf[j]));
				j--;
			}
		}
		i += sizeof(buf);
		if( (i < maxSize) && ((i+(long unsigned)(MAX)) > maxSize) )
			bufSize = maxSize - i;
	}

	free( buf );
	close( rand );
	close( outfile );

	return 0;
}
