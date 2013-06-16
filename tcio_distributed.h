#ifndef TCIO_DIST_H
#define TCIO_DIST_H

#include "tcio.h"
#include "tcio.h"
#include <math.h>


typedef struct tcio_distributed_file
{
	//file
	MPI_File fh;

	//buffer size of second level
	int bfsize;
	//second level buffer
	int bfpages;
//	int * id;
	char *data;

	MPI_Offset max_offset;

	int num_procs;
	int rank;
	MPI_Win win;
	int modified;
}* tcio_distributed_fh;

tcio_distributed_fh tcio_dist_fopen(char * filename, int mode);

int tcio_dist_fwrite(tcio_fh handle);

int tcio_dist_flush(tcio_fh handle);

int tcio_dist_fread(tcio_fh handle);

int tcio_dist_close(tcio_fh handle);

#endif
