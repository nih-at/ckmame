#ifndef _HAD_DBL_H
#define _HAD_DBL_H

#include "dbl-int.h"

DB* db_open(char *name, int extp, int writep);
int db_close(DB* db);
int db_insert(DB* db, DBT* key, DBT* value);	/* compressing versions */
int db_lookup(DB* db, DBT* key, DBT* value);

int db_insert_l(DB* db, DBT* key, DBT* value);	/* non-compressing versions */
int db_lookup_l(DB* db, DBT* key, DBT* value);

const char *db_error(void);
char *db_name(char *prefix);

#endif
