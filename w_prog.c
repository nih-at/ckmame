/* write list of strings to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "w.h"
#include "types.h"
#include "xmalloc.h"



int
w_prog(DB *db, char *name, char *version)
{
    int err;
    DBT k, v;

    k.size = 5;
    k.data = xmalloc(k.size);
    strncpy(k.data, "/prog", k.size);

    v.data = NULL;
    v.size = 0;

    w__string(&v, name);
    w__string(&v, version);

    err = db_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
