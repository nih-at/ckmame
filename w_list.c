/* write list of strings to db */

#include <stddef.h>

#include "dbl.h"
#include "w.h"


int
w_list(DB *db, char *key, char **list, int n)
{
    int i, len, err;
    DBT k, v;

    k.size = strlen(key);
    k.data = xmalloc(k.size);
    strncpy(k.data, key, k.size);

    len = 4;
    for (i=0; i<n; i++)
	len + strlen(list[i]+1);

    v.data = NULL;
    v.size = 0;

    w__array(&v, w__string, list, sizeof(char *), n);

    err = db_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
