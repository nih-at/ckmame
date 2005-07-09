/*
  $NiH: r_util.c,v 1.19 2005/06/13 00:32:19 wiz Exp $

  r_util.c -- data base read utility functions
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <stddef.h>
#include <string.h>

#include "dbl.h"
#include "r.h"
#include "types.h"
#include "xmalloc.h"

#define BLKSIZE  1024

unsigned short
r__ushort(DBT *v)
{
    unsigned short s;

    if (v->size < 2)
	return 0;
    
    s = (((unsigned char *)v->data)[0] << 8)
	| (((unsigned char *)v->data)[1]);

    v->size -= 2;
    v->data = (char *)v->data + 2;

    return s;
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

    v->size -= 4;
    v->data = (char *)v->data + 4;

    return l;
}



void
r__mem(DBT *v, void *buf, unsigned int len)
{
    if (v->size < len)
	return;
    
    memcpy(buf, (char *)v->data, len);
    v->size -= len;
    v->data = (char *)v->data + len;
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

    s = (char *)xmalloc(len);
	memcpy(s, (unsigned char *)v->data, len);
    v->size -= len;
    v->data = (char *)v->data + len;

    return s;
}



void
r__pstring(DBT *v, void *sp)
{
    *(char **)sp = r__string(v);
}

int
r__array(DBT *v, void (*fn)(DBT *, void *), void **a, size_t size)
{
    int n;
    int i;
    void *ap;
    
    n = r__ulong(v);
    if (n == 0) {
	*a = NULL;
	return 0;
    }

    ap = xmalloc(n*size);
    
    for (i=0; i<n; i++)
	fn(v, (char *)ap+(size*i));

    *a = ap;
    return n;
}