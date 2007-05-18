#include <stddef.h>

#include "sq_util.h"
#include "xmalloc.h"



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
sq3_set_string(sqlite3_stmt *stmt, int i, const char *s)
{
    int ret;
    if (s)
	ret = sqlite3_bind_text(stmt, i, s, -1, SQLITE_STATIC);
    else
	ret = sqlite3_bind_null(stmt, i);

    return ret == SQLITE_OK ? 0 : -1;
}
