/*
  $NiH: w_util.c,v 1.4 2006/04/15 22:52:58 dillo Exp $

  w_util.c -- data base write utility functions
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



#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "types.h"
#include "xmalloc.h"
#include "w.h"

#define BLKSIZE  1024

void
w__grow(DBT *v, int n)
{
    unsigned int size;
    
    size = (v->size + BLKSIZE-1) / BLKSIZE;
    size *= BLKSIZE;

    if (v->size+n >= size) {
	v->data = xrealloc(v->data, size+BLKSIZE);
    }
}



void
w__ushort(DBT *v, unsigned short s)
{
    w__grow(v, 2);
    ((unsigned char *)v->data)[v->size++] = (s>> 8) & 0xff;
    ((unsigned char *)v->data)[v->size++] = s & 0xff;
}



void
w__ulong(DBT *v, unsigned long l)
{
    w__grow(v, 4);
    ((unsigned char *)v->data)[v->size++] = (l>> 24) & 0xff;
    ((unsigned char *)v->data)[v->size++] = (l>> 16) & 0xff;
    ((unsigned char *)v->data)[v->size++] = (l>> 8) & 0xff;
    ((unsigned char *)v->data)[v->size++] = l & 0xff;
}



void
w__mem(DBT *v, const void *buf, unsigned int len)
{
    w__grow(v, len);
    memcpy(((unsigned char *)v->data)+v->size, buf, len);
    v->size += len;
}



void
w__string(DBT *v, const char *s)
{
    int len;

    if (s == NULL)
	len = 0;
    else
	len = strlen(s)+1;

    w__ushort(v, len);
    w__grow(v, len);
    if (s)
	memcpy(((unsigned char *)v->data)+v->size, s, len);
    v->size += len;
}



void
w__parray(DBT *v, void (*fn)(DBT *, const void *), const parray_t *pa)
{
    int i;
    
    if (pa == NULL) {
	w__ulong(v, 0);
	return;
    }

    w__ulong(v, parray_length(pa));
    for (i=0; i<parray_length(pa); i++)
	fn(v, parray_get(pa, i));
}



void
w__pstring(DBT *v, const void *sp)
{
    w__string(v, *(char **)sp);
}



void
w__array(DBT *v, void (*fn)(DBT *, const void *), const array_t *a)
{
    int i;
    
    w__ulong(v, array_length(a));

    for (i=0; i<array_length(a); i++)
	fn(v, array_get(a, i));
}



int
w_version(DB *db)
{
    DBT v;
    int err;

    v.data = NULL;
    v.size = 0;

    w__ushort(&v, DBH_FORMAT_VERSION);
    err = dbh_insert(db, DBH_KEY_DB_VERSION, &v);
    free(v.data);

    return err;
}
