/* should be in dbl-int.h */
   
#include <db.h>

/* should stay in dbl.h */

DB* db_open(char *name, int extp, int writep);
int db_close(DB* db);
int db_insert(DB* db, DBT* key, DBT* value);
int db_lookup(DB* db, DBT* key, DBT* value);
