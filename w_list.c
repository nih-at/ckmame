/* write list of strings to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "w.h"
#include "types.h"
#include "xmalloc.h"



int
w_list(DB *db, char *key, char **list, int n)
{
    int err;
    DBT k, v;

    k.size = strlen(key);
    k.data = xmalloc(k.size);
    strncpy(k.data, key, k.size);

    v.data = NULL;
    v.size = 0;

    w__array(&v, w__pstring, list, sizeof(char *), n);

    err = ddb_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
