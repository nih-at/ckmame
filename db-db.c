/*
  $NiH: db-db.c,v 1.13 2003/02/23 14:48:04 dillo Exp $

  db-db.c -- low level routines for Berkley db 
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "db-db.h"
#include "dbl.h"
#include "xmalloc.h"



DB*
ddb_open(char *name, int flags)
{
    DB* db;
    HASHINFO hi;
    char *s;

    if (flags & DDB_EXT)
	s = ddb_name(name);
    else
	s = name;

    hi.bsize = 1024;
    hi.ffactor = 8;
    hi.nelem = 1500;
    hi.cachesize = 65536; /* taken from db's hash.h */
    hi.hash = NULL;
    hi.lorder = 0;

    db = dbopen(s, (flags & DDB_WRITE) ? O_RDWR|O_CREAT : O_RDONLY,
		0666, DB_HASH, &hi);

    if (flags & DDB_EXT)
	free(s);

    return db;
}



int
ddb_close(DB* db)
{
    (db->close)(db);
    return 0;
}



int
ddb_insert_l(DB* db, DBT* key, DBT* value)
{
    return (db->put)(db, key, value, 0);
}



int
ddb_lookup_l(DB* db, DBT* key, DBT* value)
{
    return (db->get)(db, key, value, 0);
}



const char *
ddb_error(void)
{
    return "";
}
