#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "db-db.h"
#include "xmalloc.h"



DB*
db_open(char *name, int extp, int writep)
{
    DB* db;
    char *s;

    if (extp) {
	s = (char *)xmalloc(strlen(name)+4);
	sprintf(s, "%s.db", name);
    }
    else
	s = name;

    db = dbopen(s, writep ? O_RDWR|O_CREAT : O_RDONLY, 0666, DB_HASH, NULL);

    if (extp)
	free(s);

    return db;
}



int
db_close(DB* db)
{
    (db->close)(db);
    return 0;
}



int
db_insert_l(DB* db, DBT* key, DBT* value)
{
    return (db->put)(db, key, value, 0);
}



int
db_lookup_l(DB* db, DBT* key, DBT* value)
{
    return (db->get)(db, key, value, 0);
}
