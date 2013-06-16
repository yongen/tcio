#include "tcio_distributed.h"

#include "tcio_file.h"
#include "def.h"

tcio_distributed_fh tcio_dist_fopen(char * filename, int mode)
{
	int status;
	log_debug("tcio_dist_fopen\n");
	tcio_distributed_fh dist_handle = malloc(
			sizeof(struct tcio_distributed_file));
	dist_handle->bfpages = num_buffer_pages;
	dist_handle->bfsize = buffer_size;
	dist_handle->max_offset = 0;
	dist_handle->modified = 0;
	MPI_Alloc_mem(sizeof(char) * dist_handle->bfsize * num_buffer_pages,
			MPI_INFO_NULL, &(dist_handle->data));
	MPI_Win_create(dist_handle->data, dist_handle->bfsize * num_buffer_pages,
			1, MPI_INFO_NULL, MPI_COMM_WORLD, &(dist_handle->win));
	//	MPI_Win_fence(0, dist_handle->win);

	MPI_Comm_size(MPI_COMM_WORLD, &dist_handle->num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &dist_handle->rank);

	status = MPI_File_open(MPI_COMM_WORLD, filename, mode, MPI_INFO_NULL,
			&dist_handle->fh);
	if (status != MPI_SUCCESS)
	{
		char error_string[MPI_MAX_ERROR_STRING];
		int length_of_error_string, error_class;
		MPI_Error_class(status, &error_class);
		MPI_Error_string(error_class, error_string, &length_of_error_string);
		fprintf(stderr, "%s\n", error_string);
		MPI_Error_string(status, error_string, &length_of_error_string);
		fprintf(stderr, "%s\n", error_string);
		MPI_Abort(MPI_COMM_WORLD, status);
	}

	//cache file data in memory
	tcio_file_fread(dist_handle);

	MPI_Win_fence(MPI_MODE_NOPRECEDE, dist_handle->win);

	return dist_handle;
}

int tcio_dist_fwrite(tcio_fh handle)
{
	tcio_distributed_fh dist_handle = handle->dist_buffer;
	log_debug("rank %d: tcio_dist_fwrite\n", dist_handle->rank);
	dist_handle->modified = 1;
	dist_handle->max_offset = MAX(dist_handle->max_offset, handle->offset
			+ handle->bfptr);
	int rank_id = (handle->offset / dist_handle->bfsize)
			% dist_handle->num_procs;
	int count = handle->bfptr - handle->bfstart;
	if (count == 0)
	{
		log_info("%d: %d, %d, %d, %d\n", dist_handle->rank, handle->offset,
				handle->bfptr, handle->bfstart, handle->last_op);
	}

	int block_id = (handle->offset / dist_handle->bfsize)
			/ dist_handle->num_procs;

	MPI_Win_lock(MPI_LOCK_SHARED, rank_id, 0, dist_handle->win);
	//	MPI_Win_fence(MPI_MODE_NOPRECEDE, dist_handle->win);
	MPI_Put(handle->data + handle->bfstart, count, MPI_BYTE, rank_id, block_id
			* dist_handle->bfsize + handle->bfstart, count, MPI_BYTE,
			dist_handle->win);
	//	MPI_Win_fence(0, dist_handle->win);
	MPI_Win_unlock(rank_id, dist_handle->win);
	//	MPI_Win_fence((MPI_MODE_NOSTORE|MPI_MODE_NOSUCCEED), dist_handle->win);

	handle->bfstart = handle->bfptr;

	if (handle->bfstart == dist_handle->bfsize)
	{
		handle->offset += handle->bfsize;
		handle->bfstart = 0;
		handle->bfptr = 0;
	}
	log_debug("rank %d: tcio_dist_fwrite end\n", dist_handle->rank);
	return 0;
}

int tcio_dist_flush(tcio_fh handle)
{
	tcio_distributed_fh dist_handle = handle->dist_buffer;
	log_debug("rank %d: tcio_dist_flush\n", dist_handle->rank);
	tcio_file_fwrite(dist_handle);
	//	MPI_Win_fence(0, dist_handle->win);
	dist_handle->modified = 0;
	return 0;
}

int tcio_dist_fread(tcio_fh handle)
{
	tcio_distributed_fh dist_handle = handle->dist_buffer;
	// a special function is required for load data and the max offset
	log_debug("rank %d: tcio_dist_fread\n", dist_handle->rank);
	int rank_id = (handle->offset / dist_handle->bfsize)
			% dist_handle->num_procs;
	int cur_es;
	if (0 != handle->last_rs)
	{
		int last_rs_p = log2(handle->last_rs);
		int last_es_p = log2(handle->last_es);
		int cur_es_p = handle->alpha * last_rs_p + (1.0f - handle->alpha)
				* last_es_p;
		cur_es = pow(2, cur_es_p);
	}
	else
	{
		cur_es = handle->last_es;
	}
	//	log_info("%d: cures-%d,lastes-%d,lastrs-%d\n", dist_handle->rank, cur_es,
	//			handle->last_es, handle->last_rs);
	handle->last_es = cur_es;

	//	log_info("%d: avail - %d, max- %d\n", dist_handle->rank,
	//			dist_handle->bfsize - handle->bfptr, dist_handle->max_offset
	//					- handle->offset);
	int count = MIN3(cur_es, dist_handle->bfsize - handle->bfptr,
			dist_handle->max_offset - handle->offset);
	if(count<=0)
	{
		printf("eof 2\n");
		return 0;
	}
	//	log_info("%d: count - %d\n", dist_handle->rank, count);

	int block_id = (handle->offset / dist_handle->bfsize)
			/ dist_handle->num_procs;
	//	log_info("%d: lock\n",dist_handle->rank);
	//MPI_LOCK_EXCLUSIVE  MPI_LOCK_SHARED
	MPI_Win_lock(MPI_LOCK_SHARED, rank_id, 0, dist_handle->win);
	//	MPI_Win_fence(MPI_MODE_NOPRECEDE, dist_handle->win);
	MPI_Get(handle->data + handle->bfptr, count, MPI_BYTE, rank_id, block_id
			* dist_handle->bfsize + handle->bfptr, count, MPI_BYTE,
			dist_handle->win);
	//	MPI_Win_fence(0, dist_handle->win);
	MPI_Win_unlock(rank_id, dist_handle->win);
	//	log_info("%d: unlock\n", dist_handle->rank);
	handle->bfend = handle->bfptr + count;
	/*	if (handle->bfend == dist_handle->bfsize)
	 {
	 handle->offset += handle->bfsize;
	 }
	 */
	//	printf("%d: end %d\n",dist_handle->rank, handle->bfend);
	log_debug("rank %d: tcio_dist_fread end\n", dist_handle->rank);
	return 0;
}

int tcio_dist_close(tcio_fh handle)
{
	tcio_distributed_fh dist_handle = handle->dist_buffer;
	log_debug("rank %d: tcio_dist_close\n", dist_handle->rank);
	if (dist_handle->modified)
	{
		tcio_dist_flush(handle);
	}
	//	MPI_Win_fence(0, dist_handle->win);

	MPI_Win_fence(MPI_MODE_NOSUCCEED, dist_handle->win);
	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Free_mem(dist_handle->data);
	dist_handle->data = NULL;
	MPI_Win_free(&dist_handle->win);
	MPI_File_close(&dist_handle->fh);

	free(dist_handle);
	return 0;
}
