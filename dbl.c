/*
  $NiH: dbl.c,v 1.11 2002/06/06 09:26:53 dillo Exp $

  dbl.c -- generic low level data base routines
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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "dbl.h"
#include "xmalloc.h"



int
ddb_insert(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;
    uLong len;

    len = value->size*1.1+12;
    v.data = xmalloc(len+2);

    ((unsigned char *)v.data)[0] = (value->size >> 8) & 0xff;
    ((unsigned char *)v.data)[1] = value->size & 0xff;

    if (compress2(((unsigned char *)v.data)+2, &len, value->data, 
		  value->size, 9) != 0) {
	free(v.data);
	return -1;
    }
    v.size = len + 2;

    ret = ddb_insert_l(db, key, &v);

    free(v.data);

    return ret;
}



int
ddb_lookup(DB* db, DBT* key, DBT* value)
{
    DBT v;
    int ret;
    uLong len;

    ret = ddb_lookup_l(db, key, &v);

    if (ret != 0)
	return ret;

    value->size = ((((unsigned char *)v.data)[0] << 8)
		   | (((unsigned char *)v.data)[1]));
    value->data = xmalloc(value->size);

    len = value->size;
    if (uncompress(value->data, &len, ((unsigned char *)v.data)+2, 
		   v.size-2) != 0) {
	free(value->data);
	return -1;
    }
    value->size = len;

    return ret;
}



char *
ddb_name(char *prefix)
{
    char *s;

    if (prefix == NULL)
	return xstrdup(DDB_EXT);

    s = xmalloc(strlen(prefix)+strlen(DDB_EXT)+1);
    sprintf(s, "%s%s", prefix, DDB_EXT);

    return s;
}
