#include <stddef.h>

#include "sq_util.h"
#include "xmalloc.h"



void *
sq3_get_blob(sqlite3_stmt *stmt, int col, size_t *sizep)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
	*sizep = 0;
	return NULL;
    }

    *sizep = sqlite3_column_bytes(stmt, col);
    return xmemdup(sqlite3_column_blob(stmt, col), *sizep);
}



int
sq3_get_int_default(sqlite3_stmt *stmt, int col, int def)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL)
	return def;
    return sqlite3_column_int(stmt, col);
}



int64_t
sq3_get_int64_default(sqlite3_stmt *stmt, int col, int64_t def)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL)
	return def;
    return sqlite3_column_int64(stmt, col);
}



int
sq3_get_one_int(sqlite3 *db, const char *query, int *valp)
{
    sqlite3_stmt *stmt;
    int ret;

    if ((ret=sqlite3_prepare_v2(db, query, -1, &stmt, NULL)) != SQLITE_OK)
	return ret;

    if ((ret=sqlite3_step(stmt)) != SQLITE_ROW)
	return ret;

    *valp = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return SQLITE_OK;
}



char *
sq3_get_string(sqlite3_stmt *stmt, int i)
{
    if (sqlite3_column_type(stmt, i) == SQLITE_NULL)
	return NULL;
    
    return xstrdup((const char *)sqlite3_column_text(stmt, i));
}



int
sq3_set_blob(sqlite3_stmt *stmt, int col, const void *p, size_t s)
{
    if (s == 0 || p == NULL)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_blob(stmt, col, p, s, SQLITE_STATIC);
}



int
sq3_set_int_default(sqlite3_stmt *stmt, int col, int val, int def)
{
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int(stmt, col, val);
}



int
sq3_set_int64_default(sqlite3_stmt *stmt, int col, int64_t val, int64_t def)
{
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int64(stmt, col, val);
}




int
sq3_set_string(sqlite3_stmt *stmt, int i, const char *s)
{
    int ret;
    if (s)
	ret = sqlite3_bind_text(stmt, i, s, -1, SQLITE_STATIC);
    else
	ret = sqlite3_bind_null(stmt, i);

    return ret == SQLITE_OK ? 0 : -1;
}
