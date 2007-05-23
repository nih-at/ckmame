#include <sqlite3.h>

#include "myinttypes.h"



void *sq3_get_blob(sqlite3_stmt *, int, size_t *);
int sq3_get_int_default(sqlite3_stmt *, int, int);
int64_t sq3_get_int64_default(sqlite3_stmt *, int, int64_t);
int sq3_get_one_int(sqlite3 *, const char *, int *);
char *sq3_get_string(sqlite3_stmt *, int);
int sq3_set_blob(sqlite3_stmt *, int, const void *, size_t);
int sq3_set_int_default(sqlite3_stmt *, int, int, int);
int sq3_set_int64_default(sqlite3_stmt *, int, int64_t, int64_t);
int sq3_set_string(sqlite3_stmt *, int, const char *);

