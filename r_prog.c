/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "dbl.h"
#include "r.h"
#include "xmalloc.h"



int
r_prog(DB *db, char **namep, char **versionp)
{
    int n;
    DBT k, v;
    void *data;

    k.size = 5;
    k.data = xmalloc(k.size);
    strncpy(k.data, "/prog", k.size);

    if (db_lookup(db, &k, &v) != 0) {
	free(k.data);
	return -1;
    }
    data = v.data;

    *name = r__string(&v);
    *version = r__string(&v);

    free(k.data);
    free(data);

    return n;
}
