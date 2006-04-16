/*
  $NiH: dbh.c,v 1.1 2006/04/15 22:52:58 dillo Exp $

  dbh.c -- compressed on-disk mame.db data base
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "dbh.h"
#include "r.h"
#include "xmalloc.h"



#define DBH_ENOERR	0
#define DBH_EOLD	1	/* old db (no /ckmame entry) */
#define DBH_EVERSION	2	/* version mismatch */
#define DBH_EMAX	3

#define DBH_LEN_SIZE	4	/* size of len field */

static int dbh_errno;



int
dbh_check_version(DB *db, int flags)
{
    DBT v;
    int version;
    void *data;

    if (dbh_lookup(db, DBH_KEY_DB_VERSION, &v) != 0) {
	if (!(flags & DBL_WRITE)) {
	    /* reading database, version not found -> old */
	    dbh_errno = DBH_EOLD;
	    return -1;
	}
	else {
	    if (dbh_lookup(db, "/list", &v) == 0) {
		/* writing database, version not found, but list found
		   -> old */
		free(v.data);
		dbh_errno = DBH_EOLD;
		return -1;
	    }
	    else {
		/* writing database, version and list not found ->
		   creating database, ok */
		dbh_errno = DBH_ENOERR;
		return 0;
	    }
	}
    }

    /* compare version numbers */

    data = v.data;
    version = r__ushort(&v);
    free(data);

    if (version != DBH_FORMAT_VERSION) {
	dbh_errno = DBH_EVERSION;
	return -1;
    }
    
    dbh_errno = DBH_ENOERR;
    return 0;
}



int
dbh_close(DB *db)
{
    return dbl_close(db);
}



const char *
dbh_error(void)
{
    static const char *str[] = {
	"No error",
	"Old (incompatible) database",
	"Database format version mismatch",
	"Unknown error"
    };

    if (dbh_errno == DBH_ENOERR)
	return dbl_error();

    return str[dbh_errno<0||dbh_errno>DBH_EMAX ? DBH_EMAX : dbh_errno];
}



int
dbh_insert(DB *db, const char *key, const DBT *value)
{
    DBT k, v;
    int ret;
    uLong len;

    dbl_init_string_key(&k, key);

    len = value->size*1.1+12;
    v.data = xmalloc(len+DBH_LEN_SIZE);
    
    ((unsigned char *)v.data)[0] = (value->size >> 24) & 0xff;
    ((unsigned char *)v.data)[1] = (value->size >> 16) & 0xff;
    ((unsigned char *)v.data)[2] = (value->size >> 8) & 0xff;
    ((unsigned char *)v.data)[3] = value->size & 0xff;

    if (compress2(((unsigned char *)v.data)+DBH_LEN_SIZE, &len, value->data, 
		  value->size, 9) != 0) {
#if 0
	free(k.data);
#endif
	free(v.data);
	return -1;
    }
    v.size = len + DBH_LEN_SIZE;
    
    ret = dbl_insert(db, &k, &v);

#if 0
    free(k.data);
#endif

    return ret;
}



int
dbh_lookup(DB *db, const char *key, DBT *value)
{
    DBT k, v;
    int ret;
    uLong len;

    dbl_init_string_key(&k, key);

    ret = dbl_lookup(db, &k, &v);

    if (ret != 0)
	return ret;

    value->size = ((((unsigned char *)v.data)[0] << 24)
		   | (((unsigned char *)v.data)[1] << 16)
		   | (((unsigned char *)v.data)[2] << 8)
		   | (((unsigned char *)v.data)[3]));
    value->data = xmalloc(value->size);
    
    len = value->size;
    if ((ret=uncompress(value->data, &len,
			((unsigned char *)v.data)+DBH_LEN_SIZE, 
			v.size-DBH_LEN_SIZE)) != 0) {
	free(value->data);
	return -1;
    }
    value->size = len;
    
    return 0;
}



DB *
dbh_open(const char *name, int flags)
{
    DB *db;

    if ((db=dbl_open(name, flags)) == NULL)
	return NULL;

    if (dbh_check_version(db, flags) != 0) {
	dbl_close(db);
	return NULL;
    }

    return db;
}
