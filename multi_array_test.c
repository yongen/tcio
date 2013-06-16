#include "tcio.h"
#include "config.h"

char file_name[] = "test_file";
char cfg_name[] = "test.cfg";

int num_array;
char * type_array;
int len_array;
int io_size;

int method;

char ** arrays;

int my_rank;
int num_procs;
int mode = MPI_MODE_CREATE | MPI_MODE_RDWR;

double start, end;

MPI_File handle;
tcio_fh auto_handle;

/*
 #define NUM_ARR (1<<1)
 #define len_arr (1<<3)
 #define BUF_SIZE ((NUM_ARR)*(len_arr))

 double arrays[NUM_ARR][len_arr];
 */

int buffer_size = 1 << 20;
int num_buffer_pages = 1;
float alpha = 0.2f;

int num_array;
int len_array;
int method;
int io_size;
char * type_array;

char * buffer;
int buf_bottom, buf_top;

int type_size(int array_id)
{
	int type_size=0;
	if (type_array[array_id]=='c')
	{
		type_size = 1;
	}
	else if (type_array[array_id]=='s')
	{
		type_size = 2;
	}
	else if (type_array[array_id]== 'i')
	{
		type_size = 4;
	}
	else if (type_array[array_id]== 'f')
	{
		type_size = 4;
	}
	else if (type_array[array_id]== 'd')
	{
		type_size = 8;
	}
	return type_size;
}

int init_buf()
{
	buf_bottom = 0;
	buf_top = 0;
	int i;
	int len=0;

	for (i = 0; i < num_array; i++)
	{
		int tsize = type_size(i);
		len += len_array * tsize;
	}
	buffer = malloc(len);
	return 0;
}

int free_buf()
{
	free(buffer);
	buf_bottom = 0;
	buf_top = 0;
	return 0;
}

int fill_buf(int array_id, int element_id, int len)
{
	int tsize = type_size(array_id);
	int length = len * tsize;
	int j = tsize * element_id;
	memcpy(buffer + buf_top, &arrays[array_id][j], length);
	buf_top += length;
	return 0;
}

int fill_array(int array_id, int element_id, int len)
{
	int tsize = type_size(array_id);
	int length = len * tsize;
	int j = tsize * element_id;
	memcpy(&arrays[array_id][j], buffer + buf_bottom, length);
	buf_bottom += length;
	return 0;
}

int init_array()
{
	int i, j;
	arrays = malloc(sizeof(char *) * num_array);
	for (i = 0; i < num_array; i++)
	{
		if (type_array[i]== 'c')
		{
			arrays[i] = malloc(sizeof(char) * len_array);
			for (j = 0; j < len_array; j++)
			{
				arrays[i][j] = 'a';
			}
		}
		else if (type_array[i]== 's')
		{
			arrays[i] = malloc(sizeof(short) * len_array);
			short * tmp = (short *) arrays[i];
			for (j = 0; j < len_array; j++)
			{
				tmp[j] = 2;
			}
		}
		else if (type_array[i]== 'i')
		{
			arrays[i] = malloc(sizeof(int) * len_array);
			int * tmp = (int *) arrays[i];
			for (j = 0; j < len_array; j++)
			{
				tmp[j] = 4;
			}
		}
		else if (type_array[i]== 'f')
		{
			arrays[i] = malloc(sizeof(float) * len_array);
			float * tmp = (float *) arrays[i];
			for (j = 0; j < len_array; j++)
			{
				tmp[j] = 8.0f;
			}
		}
		else if (type_array[i]== 'd')
		{
			arrays[i] = malloc(sizeof(double) * len_array);
			double * tmp = (double *) arrays[i];
			for (j = 0; j < len_array; j++)
			{
				tmp[j] = 16.0f;
			}
		}

	}

	return 0;
}

int zero_array()
{
	int i;
	for (i = 0; i < num_array; i++)
	{
		int tsize = type_size(i);
		int len = len_array*tsize;
		memset(arrays[i], '0', len);
	}
	return 0;
}

int free_array()
{
	int i;
	for (i = 0; i < num_array; i++)
	{
		free(arrays[i]);
	}
	free(arrays);

	return 0;
}

int block_size()
{
	int len = 0, i;
	for (i = 0; i < num_array; i++)
	{
		int type_size=0;
		if (type_array[i]== 'c')
		{
			type_size = 1;
		}
		else if (type_array[i]== 's')
		{
			type_size = 2;
		}
		else if (type_array[i]=='i')
		{
			type_size = 4;
		}
		else if (type_array[i]== 'f')
		{
			type_size = 4;
		}
		else if (type_array[i]== 'd')
		{
			type_size = 8;
		}
		len += type_size * io_size;
	}
	return len;
}

int output_data_col()
{
	start = MPI_Wtime();
	//combine data in user's buffer
	init_buf();
	int i, j;
	for (i = 0; i < len_array; i += io_size)
	{
		for (j = 0; j < num_array; j++)
		{
			fill_buf(j, i, io_size);
		}
	}
	//open file
	//	log_info("%d, open file\n", my_rank);
	MPI_File_open(MPI_COMM_WORLD, file_name, mode, MPI_INFO_NULL, &handle);

	//set out file view
	////
	int len = block_size();
	MPI_Datatype eType;
	MPI_Type_contiguous(len, MPI_BYTE, &eType);
	MPI_Type_commit(&eType);

	MPI_Datatype fileType;
	MPI_Type_vector(len_array / io_size, 1, num_procs, eType, &fileType);
	MPI_Type_commit(&fileType);

	MPI_Offset disp = my_rank * len;
	//	log_info("%d, set file view\n", my_rank);
	MPI_File_set_view(handle, disp, eType, fileType, "native", MPI_INFO_NULL);

	MPI_Status status;
	//	log_info("%d, write\n", my_rank);

	MPI_File_write_all(handle, buffer,  len_array / io_size * len, MPI_BYTE,
			&status);
	MPI_File_close(&handle);
	free_buf();
	end = MPI_Wtime();
	log_info("%d, write, %f\n", my_rank, end - start);
	//	log_info("%d, output_data_col end\n", my_rank);
	return 0;
}

int read_data_col()
{
	//	log_info("%d, read_data_col start\n", my_rank);
	start = MPI_Wtime();
	init_buf();
	//open file
	//	log_info("open file \n");
	MPI_File_open(MPI_COMM_WORLD, file_name, mode, MPI_INFO_NULL, &handle);

	//set out file view
	int len = block_size();
	MPI_Datatype eType;
	MPI_Type_contiguous(len, MPI_BYTE, &eType);
	MPI_Type_commit(&eType);

	MPI_Datatype fileType;
	MPI_Type_vector(len_array / io_size, 1, num_procs, eType, &fileType);
	MPI_Type_commit(&fileType);

	MPI_Offset disp = my_rank * len;
	//	log_info("set view\n");
	MPI_File_set_view(handle, disp, eType, fileType, "native", MPI_INFO_NULL);

	MPI_Status status;
	//	log_info("read file\n");
	MPI_File_read_all(handle, buffer,  len_array /io_size* len, MPI_BYTE,
			&status);

	//	log_info("fill array\n");
	int i, j;
	for (i = 0; i < len_array; i += io_size)
	{
		for (j = 0; j < num_array; j++)
		{
			fill_array(j, i, io_size);
		}
	}
	MPI_File_close(&handle);
	free_buf();
	end = MPI_Wtime();
	log_info("%d, read, %f\n", my_rank, end - start);
	//	log_info("%d, read_data_col end \n", my_rank);
	return 0;
}

int output_data_auto()
{
	int i, j;
	start = MPI_Wtime();
	auto_handle = tcio_fopen(file_name, mode);
	int bsize = block_size();
	MPI_Offset offset = 0;
	for (i = 0; i < len_array; i += io_size)
	{
		offset = my_rank * bsize + i/io_size * bsize * num_procs;
		for (j = 0; j < num_array; j++)
		{
			int len = io_size * type_size(j);
			int tmp = i * type_size(j);
			tcio_fwrite_at(auto_handle, offset, &arrays[j][tmp], len,
					MPI_BYTE);
			offset += len;
			//		log_info("%d, write, %f, offset %d\n", my_rank, arrays[j][i],
			//				offset);
		}
	}

	tcio_fclose(auto_handle);
	end = MPI_Wtime();
	log_info("%d, write, %f\n", my_rank, end - start);
	return 0;
}

int read_data_auto()
{
	int i, j;

	zero_array();
	start = MPI_Wtime();
	auto_handle = tcio_fopen(file_name, mode);

	int bsize = block_size();
	MPI_Offset offset = 0;
	for (i = 0; i < len_array; i += io_size)
	{
		offset = my_rank * bsize + i/io_size * bsize * num_procs;
		for (j = 0; j < num_array; j++)
		{
			int len = io_size * type_size(j);
			int tmp = i * type_size(j);
			tcio_fread_at(auto_handle, offset, &arrays[j][tmp], len,
					MPI_BYTE);
			offset += len;
			//		log_info("%d,read, %f, offset %d\n", my_rank, arrays[j][i], offset);
		}
	}

	tcio_fclose(auto_handle);
	end = MPI_Wtime();
	log_info("%d, read, %f\n", my_rank, end - start);
	return 0;
}

int print_array()
{
	int i;
	for (i = 0; i < num_array; i++)
	{
		if (type_array[i]== 'c')
		{
			printf("rank %d : %d, %c \n", my_rank, i, arrays[i][0]);
		}
		if (type_array[i]== 's')
		{
			printf("rank %d : %d, %d \n", my_rank, i, (short) arrays[i][0]);
		}
		if (type_array[i]== 'i')
		{
			printf("rank %d : %d, %d \n", my_rank, i, (int) arrays[i][0]);
		}
		if (type_array[i]== 'f')
		{
			float * tmp = (float *)arrays[i];
			printf("rank %d : %d, %f \n", my_rank, i, tmp[0]);
		}
		if (type_array[i]== 'd')
		{
			double * tmp = (double *)arrays[i];
			printf("rank %d : %d, %lf %lf \n", my_rank, i, tmp[0],tmp[1]);
		}
	}
	return 0;
}

int main(argc, argv)
	int argc;char * argv[];
{
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	load_config(cfg_name);

	init_array();

	MPI_Barrier(MPI_COMM_WORLD);
	if (method == 0)
	{
		output_data_col();
	}
	else
	{
		output_data_auto();
	}
	MPI_Barrier(MPI_COMM_WORLD);

	zero_array();

	if (method == 0)
	{
		read_data_col();
	}
	else
	{
		read_data_auto();
	}
	MPI_Barrier(MPI_COMM_WORLD);
	print_array();

	free_array();

	MPI_Finalize();
	return 0;
}
