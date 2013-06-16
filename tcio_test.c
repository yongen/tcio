#include "tcio.h"

//begin global setting
int lib_option = 0;
int buffer_size = 1 << 23;
int num_buffer_pages = 1;
//end global setting


#define MAX_LEN	 (1<<10)
#define LEN	(1<<10)

char fileName[] = "test_file";

int my_rank;
int num_procs;

double start, end;

double cols[MAX_LEN];
double result[MAX_LEN][LEN];

tcio_fh handle;

int init_cols()
{
	int i, k;
	k = 0;
	for (i = 0; i < MAX_LEN; i++)
	{
		cols[i] = my_rank + i * num_procs;
	}
	return 0;
}

int output_matrix()
{
	start = MPI_Wtime();
	handle = tcio_fopen(fileName, MPI_MODE_CREATE | MPI_MODE_RDWR);
	MPI_Offset offset;
	int i, j;
	int type_size;
	MPI_Type_size(MPI_DOUBLE, &type_size);
	for (i = 0; i < MAX_LEN; i++)
	{
		offset = cols[i] * type_size * LEN;
		double value[LEN];
		for (j = 0; j < LEN; j++)
		{
			value[j] = cols[i];
		}
#ifdef SEPERATE
		for(j=0;j<LEN;j++)
		{
			tcio_fwrite_at(handle, offset, &value[j], 1, MPI_DOUBLE);
			offset+=type_size;
		}
#else
		tcio_fwrite_at(handle, offset, value, LEN, MPI_DOUBLE);
#endif
	}
	tcio_flush(handle);

	end = MPI_Wtime();
	log_info("%d, io staging write, %f\n", my_rank, end - start);

	start = MPI_Wtime();
	tcio_fclose(handle);
	end = MPI_Wtime();
	log_info("%d, write, %f\n", my_rank, end - start);

	return 0;
}

int read_matrix()
{
	start = MPI_Wtime();
	handle = tcio_fopen(fileName, MPI_MODE_RDWR);
	end = MPI_Wtime();
	log_info("%d, read, %f\n", my_rank, end - start);
	start = MPI_Wtime();
	MPI_Offset offset;
	int i, j;
	int type_size;
	MPI_Type_size(MPI_DOUBLE, &type_size);
	for (i = 0; i < MAX_LEN; i++)
	{
		offset = cols[i] * type_size * LEN;
		//	double value[LEN];

#ifdef SEPERATE
		for(j=0;j<LEN;j++)
		{
			tcio_fread_at(handle, offset, &result[i][j], 1, MPI_DOUBLE);
			offset+=type_size;
		}
#else
		tcio_fread_at(handle, offset, &result[i][0], LEN, MPI_DOUBLE);
#endif

		//	result[i]=value[0];
		/*		if (MAX_LEN % 1024 == 0)
		 {
		 tcio_fetch(handle);
		 }*/
	}

	tcio_fetch(handle);
	end = MPI_Wtime();
	log_info("%d, io staging read, %f\n", my_rank, end - start);

	tcio_fclose(handle);

	return 0;
}

int print()
{
	int i;
	for (i = 0; i < MAX_LEN; i++)
	{
		printf("rank %d : %d, %f\n", my_rank, i, result[i][0]);
	}
	return 0;
}

int main(argc, argv)
	int argc;char * argv[];
{
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	//	printf("size: %d\n", sizeof(char) * buffer_size * num_buffer_pages);
	init_cols();

	MPI_Barrier(MPI_COMM_WORLD);

//	start = MPI_Wtime();
	output_matrix();
//	end = MPI_Wtime();
//	log_info("%d, write, %f\n", my_rank, end - start);

	MPI_Barrier(MPI_COMM_WORLD);
//	start = MPI_Wtime();
	read_matrix();
//	end = MPI_Wtime();
//	log_info("%d, read, %f\n", my_rank, end - start);
//	print();
	MPI_Finalize();
	return 0;
}
