/*
  db-gdbm.c -- low level routines for GNU gdbm
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

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
#include <gdbm.h>

#include "xmalloc.h"
#include "dbl.h"



DB*
ddb_open(char *name, int extp, int writep)
{
    GDBM_FILE db;
    char *s;

    if (extp) {
	s = (char *)xmalloc(strlen(name)+6);
	sprintf(s, "%s.gdbm", name);
    }
    else
	s = name;

    db = gdbm_open(s, 0, writep ? GDBM_WRCREAT : GDBM_READER, 0666, NULL);

    return (DB*)db;
}



int
ddb_close(DB* db)
{
    gdbm_close((GDBM_FILE)db);

    return 0;
}



int
ddb_insert_l(DB* db, DBT* key, DBT* value)
{
    return gdbm_store((GDBM_FILE)db, *(datum *)key, *(datum *)value,
		      GDBM_REPLACE);
}



int
ddb_lookup_l(DB* db, DBT* key, DBT* value)
{
    datum v;

    v = gdbm_fetch((GDBM_FILE)db, *(datum *)key);

    if (v.dptr == NULL)
	return 1;

    value->data = v.dptr;
    value->size = v.dsize;

    return 0;
}



const char *
ddb_error(void)
{
    return gdbm_strerror(gdbm_errno);
}
