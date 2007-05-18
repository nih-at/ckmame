#include <sqlite3.h>



char *sq3_get_string(sqlite3_stmt *, int);
int sq3_get_one_int(sqlite3 *, const char *, int *);
int sq3_set_string(sqlite3_stmt *, int, const char *);
