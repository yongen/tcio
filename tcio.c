#include "tcio.h"
#include "tcio_distributed.h"
#include "def.h"

tcio_fh tcio_fopen(char * filename, int mode)
{
	log_debug("tcio_fopen\n");
	tcio_fh handle = malloc(sizeof(struct tcio_file));
	handle->data = malloc(buffer_size);
	memset(handle->data, 0, buffer_size);
	handle->bfptr = 0;
	handle->bfsize = buffer_size;
	handle->bfstart = 0;
	handle->bfend = -1;
	handle->offset = 0.0f;

	handle->alpha = alpha;
	handle->last_es = buffer_size;
	handle->last_rs = buffer_size;
	handle->rs_counter = 0;

	handle->dist_buffer = tcio_dist_fopen(filename, mode);
	handle->last_op = OPERATION_OPEN;
	return handle;
}

int tcio_fwrite(tcio_fh handle, void * buf, int count,
		MPI_Datatype type)
{
	int size, part, remain;
	int type_size;
	log_debug("tcio_fwrite\n");
	MPI_Type_size(type, &type_size);
	size = count * type_size;
	if (OPERATION_READ == handle->last_op)
	{
		handle->bfstart = handle->bfptr;
	}
	if (handle->bfptr + size <= handle->bfsize)
	{
		memcpy(handle->data + handle->bfptr, (char *)buf, size);
		handle->bfptr += size;
	}
	else
	{
		part = handle->bfsize - handle->bfptr;
		remain = handle->bfptr + size - handle->bfsize;
		memcpy(handle->data + handle->bfptr, (char *)buf, part);
		handle->bfptr += part;
		//move data from local buffer to distributed buffer
		tcio_dist_fwrite(handle);

		while (remain > handle->bfsize)
		{
			memcpy(handle->data, (char *) buf + part, handle->bfsize);
			//move data to distributed buffer
			handle->bfptr += handle->bfsize;
			tcio_dist_fwrite(handle);

			remain -= handle->bfsize;
			part += handle->bfsize;
		}
		memcpy(handle->data, (char *) buf + part, remain);
		handle->bfptr += remain;
	}
	handle->last_op = OPERATION_WRITE;
	log_debug("tcio_fwrite end\n");
	return 0;
}

int tcio_fseek(tcio_fh handle, MPI_Offset offset, int whence)
{
	//	log_info("rank %d: tcio_fseek\n",handle->dist_buffer->rank);
	if (whence == MPI_SEEK_CUR)
	{
		if (0 == offset)
		{
			return 0;
		}
		else
		{
			if (OPERATION_WRITE == handle->last_op)
			{
				tcio_dist_fwrite(handle);
			}

			if (offset > 0)
			{
				int ptr = offset % buffer_size;
				handle->bfstart = ptr;
				handle->bfptr = ptr;
				handle->offset += offset - ptr;
				if (offset != ptr)//seek out of current local buffer
				{
					handle->bfend = -1;
				}
			}
			else
			{
				int ptr = offset % buffer_size;
				handle->bfstart = buffer_size + ptr;
				handle->bfptr = buffer_size + ptr;
				handle->offset += offset - ptr - buffer_size;
				if ((offset - ptr - buffer_size) != 0)//seek out of current local buffer
				{
					handle->bfend = -1;
				}
			}
		}
		handle->last_rs = handle->rs_counter;
		handle->rs_counter = 0;
	}
	else if (MPI_SEEK_SET == whence)
	{
		if ((handle->offset + handle->bfptr) == offset)
		{
			return 0;
		}

		if (OPERATION_WRITE == handle->last_op)
		{
			tcio_dist_fwrite(handle);
		}
		int ptr = offset % buffer_size;
		handle->bfstart = ptr;
		handle->bfptr = ptr;
		if (handle->offset != (offset - ptr))
		{
			handle->bfend = -1;
			handle->offset = offset - ptr;
		}

		handle->last_rs = handle->rs_counter;
		handle->rs_counter = 0;
	}

	handle->last_op = OPERATION_SEEK;
	log_debug("rank %d: tcio_fseek end\n", handle->dist_buffer->rank);
	return 0;
}

int tcio_fwrite_at(tcio_fh handle, MPI_Offset offset, void * buf,
                int count, MPI_Datatype type)
{
	tcio_fseek(handle, offset, MPI_SEEK_SET);
        tcio_fwrite(handle, buf, count, type);
	return 0;
}

int tcio_flush(tcio_fh handle)
{
	log_debug("tcio_flush\n");
	tcio_dist_fwrite(handle);
	handle->bfend = -1;
	return 0;
}

int tcio_fread(tcio_fh handle, void *buf, int count,
		MPI_Datatype type)
{
	int size, remain, avail;
	int type_size;
	log_debug("%d: tcio_fread\n", handle->dist_buffer->rank);
	MPI_Type_size(type, &type_size);
	size = count * type_size;

	if (OPERATION_WRITE == handle->last_op)
	{
		tcio_flush(handle);
	}
	//first load data or seek ptr to
	if (handle->bfend == -1)
	{
	//	printf("%d: preload \n", handle->dist_buffer->rank);
		tcio_dist_fread(handle);
	}

	/** read from buffer */
	char * curbuf;
	curbuf = (char *) buf;
	remain = size;
	handle->rs_counter += size;
	//	printf("e\n");

	while (remain > 0 && handle->bfend > 0 && handle->bfptr + remain
			> handle->bfend)
	{
	//	printf("%d: %d, %d, %d \n", handle->dist_buffer->rank, handle->bfptr,
	//			remain, handle->bfend);
		avail = handle->bfend - handle->bfptr;
		if (avail > 0)
		{
			memcpy(curbuf, (char *) handle->data + handle->bfptr, avail);
			curbuf += avail;
			remain -= avail;
			handle->bfptr += avail;
		}

		if (handle->bfptr == handle->bfsize)
		{
			handle->bfptr = 0;
			handle->offset += handle->bfsize;
		}
		//	printf("miss 2 \n");
		/* refill buffer */
		tcio_dist_fread(handle);
	}

	if (handle->bfend == 0)
	{
		/* ran out of data, eof */
		printf("eof\n");
	}
	else
	{
		memcpy(curbuf, handle->data + handle->bfptr, remain);
		handle->bfptr += remain;
		if (handle->bfptr == handle->bfsize)
		{
			handle->bfptr = 0;
			handle->offset += handle->bfsize;

			handle->bfend = -1;
			//	tcio_dist_fread(handle);
		}
	}

	handle->last_op = OPERATION_READ;
	log_debug("tcio_fread end\n");
	return 0;
}

int tcio_fread_at(tcio_fh handle, MPI_Offset offset, void *buf,
                int count, MPI_Datatype type)
{
		tcio_fseek(handle, offset, MPI_SEEK_SET);
		tcio_fread(handle, buf, count, type);
		return 0;
}

int tcio_fclose(tcio_fh handle)
{
	log_debug("rank %d:tcio_fclose\n", handle->dist_buffer->rank);
	if (OPERATION_WRITE == handle->last_op)
	{
		tcio_flush(handle);
	}
	if (handle->bfsize > 0)
	{
		free(handle->data);
	}
	tcio_dist_close(handle);
	//	MPI_File_close(&handle->fh);

	free(handle);
	return 0;
}
