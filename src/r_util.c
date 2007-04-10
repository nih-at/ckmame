/*
  $NiH: r_util.c,v 1.4 2006/10/04 17:36:44 dillo Exp $

  r_util.c -- data base read utility functions
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <stddef.h>
#include <string.h>

#include "dbl.h"
#include "r.h"
#include "types.h"
#include "xmalloc.h"

#define BLKSIZE  1024

#define ADVANCE(v, n)	((v)->size -= (n), (v)->data = (char *)((v)->data) + (n))



array_t *
r__array(DBT *v, void (*fn)(DBT *, void *), size_t size)
{
    int n;
    int i;
    array_t *a;
    
    n = r__ulong(v);

    
    a = array_new_sized(size, n);
    
    for (i=0; i<n; i++) {
	array_grow(a, NULL);
	fn(v, array_get(a, i));
    }

    return a;
}



void
r__mem(DBT *v, void *buf, unsigned int len)
{
    if (v->size < len)
	return;
    
    memcpy(buf, (char *)v->data, len);
    ADVANCE(v, len);
}



parray_t *
r__parray(DBT *v, void *(*fn)(DBT *))
{
    parray_t *pa;
    int i, n;

    n = r__ulong(v);

    pa = parray_new_sized(n);

    for (i=0; i<n; i++)
	parray_push(pa, fn(v));

    return pa;
}



void
r__pstring(DBT *v, void *sp)
{
    *(char **)sp = r__string(v);
}



char *
r__string(DBT *v)
{
    char *s;
    unsigned int len;

    len = r__ushort(v);
    if (len == 0)
	return NULL;
    
    if (v->size < len)
	return NULL;

    s =xmalloc(len);
    memcpy(s, v->data, len);
    ADVANCE(v, len);

    return s;
}



uint64_t
r__uint64(DBT *v)
{
    uint64_t u;
    uint8_t *d;

    if (v->size < 8)
	return 0;

    d = v->data;

    u = ((((uint64_t)d[0])<<56) | (((uint64_t)d[1])<<48)
	 | (((uint64_t)d[2])<<40) | (((uint64_t)d[3])<<32)
	 | (((uint64_t)d[4])<<24) | (((uint64_t)d[5])<<16)
	 | (((uint64_t)d[6])<<8) | (d[7])) & 0xffffffffffffffffLL;

    ADVANCE(v, 8);

    return u;
}



uint8_t
r__uint8(DBT *v)
{
    uint8_t u;
    uint8_t *d;

    if (v->size < 1)
	return 0;

    d = v->data;

    u = d[0];

    ADVANCE(v, 1);

    return u;
}



unsigned long
r__ulong(DBT *v)
{
    unsigned long l;

    if (v->size < 4)
	return 0;
    
    l = ((((unsigned char *)v->data)[0] << 24)
	 | (((unsigned char *)v->data)[1] << 16)
	 | (((unsigned char *)v->data)[2] << 8)
	 | (((unsigned char *)v->data)[3])) & 0xffffffff;

    ADVANCE(v, 4);

    return l;
}



unsigned short
r__ushort(DBT *v)
{
    unsigned short s;

    if (v->size < 2)
	return 0;
    
    s = (((unsigned char *)v->data)[0] << 8)
	| (((unsigned char *)v->data)[1]);

    ADVANCE(v, 2);

    return s;
}
