/*
  $NiH: db-db.c,v 1.3 2006/04/16 00:12:56 dillo Exp $

  db-db.c -- access abstractions for Berkeley DB 1.x
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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



#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "dbl.h"



DB*
dbl_open(const char *name, int flags)
{
    HASHINFO hi;

    hi.bsize = 1024;
    hi.ffactor = 8;
    hi.nelem = 1500;
    hi.cachesize = 65536; /* taken from db's hash.h */
    hi.hash = NULL;
    hi.lorder = 0;

    return dbopen(name, (flags & DBL_WRITE) ? O_RDWR|O_CREAT : O_RDONLY,
		  0666, DB_HASH, &hi);
}



int
dbl_close(DB *db)
{
    (db->close)(db);
    return 0;
}



int
dbl_delete(DB *db, const DBT *key)
{
    return (db->del)(db, key, 0);
}




int
dbl_insert(DB *db, DBT *key, const DBT *value)
{
    return (db->put)(db, key, value, 0);
}



int
dbl_lookup(DB *db, DBT *key, DBT *value)
{
    return (db->get)(db, key, value, 0);
}



const char *
dbl_error(void)
{
    return strerror(errno);
}



int
dbl_foreach(DB *db, int (*f)(const DBT *, const DBT *, void *), void *ud)
{
    DBT key, value;
    int ret;

    /* test inside body to catch empty database */
    for (ret=db->seq(db, &key, &value, R_FIRST); ;
	 ret=db->seq(db, &key, &value, R_NEXT)) {
	if (ret != 0)
	    break;
	if ((ret=f(&key, &value, ud)) != 0)
	    break;
    }

    return ret < 0 ? -1 : 0;
}
