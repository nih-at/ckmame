#include <stddef.h>
#include <fcntl.h>
#include <db-db.h>

#include <zlib.h>



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
db_insert(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;

    v.size = value->size*1.1+12;
    v.data = xmalloc(v.size+2);

    ((unsigned char *)v.data)[0] = (value->size >> 8) & 0xff;
    ((unsigned char *)v.data)[1] = value->size & 0xff;

    if (compress2(v.data+2, &v.size, value->data, value->size, 9) != 0) {
	free(v.data);
	return -1;
    }

    v.size += 2;

    ret = (db->put)(db, key, &v, 0);

    free(v.data);

    return ret;
}



int
db_lookup(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;

    ret = (db->get)(db, key, &v, 0);

    value->size = ((((unsigned char *)v.data)[0] << 8)
		   | (((unsigned char *)v.data)[1]));
    value->data = xmalloc(value->size);

    if (uncompress(value->data, &value->size, v.data+2, v.size-2) != 0) {
	free(v.data);
	return -1;
    }
    
    free(v.data);

    return ret;
}
