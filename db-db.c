#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "db-db.h"
#include "dbl.h"
#include "xmalloc.h"



DB*
ddb_open(char *name, int extp, int writep)
{
    DB* db;
    HASHINFO hi;
    char *s;

    if (extp) {
	s = (char *)xmalloc(strlen(name)+4);
	sprintf(s, "%s.db", name);
    }
    else
	s = name;

    hi.bsize = 1024;
    hi.ffactor = 8;
    hi.nelem = 1500;
    hi.cachesize = 65536; /* taken from db's hash.h */
    hi.hash = NULL;
    hi.lorder = 0;

    db = dbopen(s, writep ? O_RDWR|O_CREAT : O_RDONLY, 0666, DB_HASH, &hi);

    if (extp)
	free(s);

    return db;
}



int
ddb_close(DB* db)
{
    (db->close)(db);
    return 0;
}



int
ddb_insert_l(DB* db, DBT* key, DBT* value)
{
    return (db->put)(db, key, value, 0);
}



int
ddb_lookup_l(DB* db, DBT* key, DBT* value)
{
    return (db->get)(db, key, value, 0);
}



const char *
ddb_error(void)
{
    return "";
}
