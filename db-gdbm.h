#ifndef _HAD_DBL_INT_H
#define _HAD_DBL_INT_H
   
#include <gdbm.h>

typedef struct {
    char *data;
    int  size;
} DBT;

typedef void *DB;
#define DDB_EXT ".gdbm"

#endif
