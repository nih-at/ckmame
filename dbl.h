#ifndef _HAD_DBL_H
#define _HAD_DBL_H

#include "dbl-int.h"

DB* ddb_open(char *name, int extp, int writep);
int ddb_close(DB* db);
int ddb_insert(DB* db, DBT* key, DBT* value);	/* compressing versions */
int ddb_lookup(DB* db, DBT* key, DBT* value);

int ddb_insert_l(DB* db, DBT* key, DBT* value);	/* non-compressing versions */
int ddb_lookup_l(DB* db, DBT* key, DBT* value);

const char *ddb_error(void);
char *ddb_name(char *prefix);

#endif
