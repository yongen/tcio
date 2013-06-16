#include "tcio_file.h"
#include "tcio_distributed.h"

#include "def.h"

int tcio_file_fwrite(tcio_distributed_fh dist_handle)
{
	MPI_Offset offset_tmp;
	//	tcio_distributed_fh dist_handle = handle->dist_buffer;
	log_debug("rank %d:tcio_file_fwrite\n", dist_handle->rank);
	MPI_Win_fence(0, dist_handle->win);
	MPI_Offset offset = dist_handle->rank * dist_handle->bfsize;
	int i = 0;
	//broadcast to find the max offset
	MPI_Allreduce(&dist_handle->max_offset, &offset_tmp, 1, MPI_LONG_LONG_INT,
			MPI_MAX, MPI_COMM_WORLD);
	dist_handle->max_offset = offset_tmp;
//	printf("rank:%d,%d \n", dist_handle->rank, offset_tmp);
	while (offset <= dist_handle->max_offset)
	{
		int count = MIN(dist_handle->max_offset - offset, dist_handle->bfsize);
		MPI_File_write_at(dist_handle->fh, offset, dist_handle->data + i
				* dist_handle->bfsize, count, MPI_BYTE, MPI_STATUS_IGNORE);
		i++;
//		printf("rank:%d,%d,%d\n", dist_handle->rank, offset, count);
		offset += dist_handle->bfsize * dist_handle->num_procs;
	}
	log_debug("rank %d:tcio_file_fwrite end\n", dist_handle->rank);
	return 0;
}

int tcio_file_fread(tcio_distributed_fh dist_handle)
{
	MPI_Status status;
	MPI_Offset offset_tmp;
	int count = 0;
	//	tcio_distributed_fh dist_handle = handle->dist_buffer;
	log_debug("rank %d: tcio_file_fread\n",dist_handle->rank);
	MPI_Offset offset = dist_handle->rank * dist_handle->bfsize;
//	MPI_File_seek(dist_handle->fh, offset, MPI_SEEK_CUR);
	MPI_File_read_at(dist_handle->fh,offset, dist_handle->data, dist_handle->bfsize,
			MPI_BYTE, &status);
	MPI_Get_count(&status, MPI_BYTE, &count);

	int i = 1;
	while (count == dist_handle->bfsize)
	{
//		MPI_File_seek(dist_handle->fh, dist_handle->bfsize
//						* dist_handle->num_procs, MPI_SEEK_CUR);
		offset+=dist_handle->bfsize * dist_handle->num_procs;
		MPI_File_read_at(dist_handle->fh, offset, dist_handle->data + i
				* dist_handle->bfsize, dist_handle->bfsize, MPI_BYTE, &status);
		MPI_Get_count(&status, MPI_BYTE, &count);
		i++;
	}
//	offset_tmp = (i - 1) * dist_handle->num_procs * dist_handle->bfsize + count;
	offset_tmp = offset + count;
	//broadcast to find the max offset
	MPI_Allreduce(&offset_tmp, &dist_handle->max_offset, 1, MPI_LONG_LONG_INT,
			MPI_MAX, MPI_COMM_WORLD);
	log_debug("rank %d: tcio_file_fread end\n",dist_handle->rank);
	return 0;
}
