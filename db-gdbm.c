#include <stdio.h>
#include <gdbm.h>

#include "xmalloc.h"
#include "dbl.h"



DB*
db_open(char *name, int extp, int writep)
{
    GDBM_FILE db;
    char *s;

    if (extp) {
	s = (char *)xmalloc(strlen(name)+6);
	sprintf(s, "%s.gdbm", name);
    }
    else
	s = name;

    db = gdbm_open(s, 0, writep ? GDBM_WRCREAT : GDBM_READER, 0666, NULL);

    return (DB*)db;
}



int
db_close(DB* db)
{
    gdbm_close((GDBM_FILE)db);

    return 0;
}



int
db_insert_l(DB* db, DBT* key, DBT* value)
{
    return gdbm_store((GDBM_FILE)db, *(datum *)key, *(datum *)value,
		      GDBM_REPLACE);
}



int
db_lookup_l(DB* db, DBT* key, DBT* value)
{
    datum v;

    v = gdbm_fetch((GDBM_FILE)db, *(datum *)key);

    if (v.dptr == NULL)
	return 1;

    value->data = v.dptr;
    value->size = v.dsize;

    return 0;
}



const char *
db_error(void)
{
    return gdbm_strerror(gdbm_errno);
}
