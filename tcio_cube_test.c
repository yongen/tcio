#include "tcio.h"

//begin global setting
int lib_option = 1;
int buffer_size = 1 << 8;
int num_buffer_pages  = 1<<19;
//end global setting


#define Z_LEN	(1<<8)
#define X_LEN	(1<<8)
#define Y_LEN	(1<<8)

#define NUM_PROC_PER_DIM	(4)

char fileName[] = "test_file";

int my_rank;
int num_procs;

double start, end;

double offsets[Z_LEN][Y_LEN];
double result[Z_LEN][Y_LEN][X_LEN];

tcio_fh handle;

int init_cols()
{
	int i, j, k;
	k = 0;
	int z_index = my_rank / ((NUM_PROC_PER_DIM) * (NUM_PROC_PER_DIM));
	int y_index = (my_rank % ((NUM_PROC_PER_DIM) * (NUM_PROC_PER_DIM)))
			/ (NUM_PROC_PER_DIM);
	int x_index = (my_rank % ((NUM_PROC_PER_DIM) * (NUM_PROC_PER_DIM)))
			% (NUM_PROC_PER_DIM);

	int base = z_index * ((NUM_PROC_PER_DIM) * (NUM_PROC_PER_DIM))*(X_LEN)*(Y_LEN)*(Z_LEN);

	for (i = 0; i < Z_LEN; i++)
	{
		int z_off = (X_LEN)* (NUM_PROC_PER_DIM)*(Y_LEN)*(NUM_PROC_PER_DIM)*i;
		for (j = 0; j < Y_LEN; j++)
		{
			int y_off = j*(X_LEN)* (NUM_PROC_PER_DIM);
			offsets[i][j] = base+z_off+y_off;
		}
	}
	return 0;
}

int output_matrix()
{
	//	start = MPI_Wtime();
	handle = tcio_fopen(fileName, MPI_MODE_CREATE | MPI_MODE_RDWR);
	MPI_Offset offset;
	int i, j, k;
	int type_size;
	MPI_Type_size(MPI_DOUBLE, &type_size);
	for (i = 0; i < Z_LEN; i++)
	{
		for (j = 0; j < Y_LEN; j++)
		{
			offset = offsets[i][j] * type_size;
			double value[X_LEN];
			for (k = 0; k < X_LEN; k++)
			{
				value[k] = offsets[i][j];
			}
#ifdef SEPERATE
			for(k=0;k<X_LEN;k++)
			{
				tcio_fwrite_at(handle, offset, &value[k], 1, MPI_DOUBLE);
				offset+=type_size;
			}
#else
			tcio_fwrite_at(handle, offset, value, X_LEN, MPI_DOUBLE);
#endif
		}
	}
	tcio_flush(handle);

	//	end = MPI_Wtime();
	//	log_info("%d, io staging write, %f\n", my_rank, end - start);

	//	start = MPI_Wtime();
	tcio_fclose(handle);
	//	end = MPI_Wtime();
	//	log_info("%d, write, %f\n", my_rank, end - start);

	return 0;
}

int read_matrix()
{
	//	start = MPI_Wtime();
	handle = tcio_fopen(fileName, MPI_MODE_RDWR);
	//	end = MPI_Wtime();
	//	log_info("%d, read, %f\n", my_rank, end - start);
	//	start = MPI_Wtime();
	MPI_Offset offset;
	int i, j, k;
	int type_size;
	MPI_Type_size(MPI_DOUBLE, &type_size);
	for (i = 0; i < Z_LEN; i++)
	{
		for (j = 0; j < Y_LEN; j++)
		{
			offset = offsets[i][j] * type_size;
			//	double value[LEN];

#ifdef SEPERATE
			for(k=0;k<X_LEN;k++)
			{
				tcio_fread_at(handle, offset, &result[i][j][k], 1, MPI_DOUBLE);
				offset+=type_size;
			}
#else
			tcio_fread_at(handle, offset, &result[i][j][0], X_LEN, MPI_DOUBLE);
#endif
		}
		//	result[i]=value[0];
		/*		if (MAX_LEN % 1024 == 0)
		 {
		 tcio_fetch(handle);
		 }*/
	}

	tcio_fetch(handle);
	//	end = MPI_Wtime();
	//	log_info("%d, io staging read, %f\n", my_rank, end - start);

	tcio_fclose(handle);

	return 0;
}

int print()
{
	int i, j;
	for (i = 0; i < Z_LEN; i++)
	{
		for(j=0;j<Y_LEN;j++)
		{
			printf("rank %d : %d, %f\n", my_rank, i, result[i][j][0]);
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

	//	printf("size: %d\n", sizeof(char) * buffer_size * num_buffer_pages);
	init_cols();

	MPI_Barrier(MPI_COMM_WORLD);

	start = MPI_Wtime();
	output_matrix();
	end = MPI_Wtime();
	log_info("%d, write, %f\n", my_rank, end - start);

	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	read_matrix();
	end = MPI_Wtime();
	log_info("%d, read, %f\n", my_rank, end - start);
	print();
	MPI_Finalize();
	return 0;
}
