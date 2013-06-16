#ifndef DEF_H
#define DEF_H

#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"


#define OPERATION_OPEN		0
#define	OPERATION_WRITE		1
#define OPERATION_READ		2
#define OPERATION_SEEK		3

#define MAX(a,b)	((a>b)?(a):(b))
#define MIN(a,b)	((a<b)?(a):(b))
#define MIN3(a,b,c)	((MIN(a,b)<c)?(MIN(a,b)):(c))
#define MINTHREE(a,b,c)	(((a)<(b)?(a):(b))<(c)?((a)<(b)?(a):(b)):(c))


extern int buffer_size;
extern int num_buffer_pages;
extern float alpha;

extern int method;

extern int num_array;
extern char * type_array;
extern int  len_array;
extern int  io_size;

#endif
