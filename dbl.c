#include <stdlib.h>
#include <zlib.h>

#include "dbl.h"
#include "xmalloc.h"



int
db_insert(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;
    uLong len;

    v.size = value->size*1.1+12;
    v.data = xmalloc(v.size+2);

    ((unsigned char *)v.data)[0] = (value->size >> 8) & 0xff;
    ((unsigned char *)v.data)[1] = value->size & 0xff;

    if (compress2(v.data+2, &len, value->data, value->size, 9) != 0) {
	free(v.data);
	return -1;
    }
    v.size = len + 2;

    ret = db_insert_l(db, key, &v);

    free(v.data);

    return ret;
}



int
db_lookup(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;
    uLong len;

    ret = db_lookup_l(db, key, &v);

    if (ret != 0)
	return ret;

    value->size = ((((unsigned char *)v.data)[0] << 8)
		   | (((unsigned char *)v.data)[1]));
    value->data = xmalloc(value->size);

    len = value->size;
    if (uncompress(value->data, &len, v.data+2, v.size-2) != 0) {
	free(v.data);
	return -1;
    }
    v.size = len;

    free(v.data);

    return ret;
}
