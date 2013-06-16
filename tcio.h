#ifndef TCIO_H
#define TCIO_H

#include <mpi.h>

typedef struct tcio_file
{
	int last_op;
	//first level buffer
	int bfptr;
	char * data;
	int bfstart, bfend, bfsize;
	MPI_Offset offset;

	//prefetch size
	int last_es;
	float alpha;
	int last_rs;
	int rs_counter;

	//second level buffer
	struct tcio_distributed_file * dist_buffer;
} * tcio_fh;

tcio_fh tcio_fopen(char * filename, int mode);

int	tcio_fwrite(tcio_fh handle, void * buf, int count, MPI_Datatype type);

int tcio_fwrite_at(tcio_fh handle, MPI_Offset offset, void * buf,
		int count, MPI_Datatype type);

int tcio_fseek(tcio_fh handle, MPI_Offset offset, int whence);

int tcio_flush(tcio_fh handle);

int tcio_fread(tcio_fh handle,void *buf, int count, MPI_Datatype type);

int tcio_fread_at(tcio_fh handle, MPI_Offset offset, void *buf,
		int count, MPI_Datatype type);

int tcio_fclose(tcio_fh handle);

#endif
