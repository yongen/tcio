#ifndef TCIO_FILE_H
#define TCIO_FILE_H

#include "tcio_distributed.h"
#include "def.h"

int tcio_file_fwrite(tcio_distributed_fh dist_handle);

int tcio_file_fread(tcio_distributed_fh dist_handle);

#endif
