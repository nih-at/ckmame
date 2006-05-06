/*
  $NiH: array.c,v 1.1 2005/07/13 17:42:19 dillo Exp $

  array.c -- create / free array of arbitrary types
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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
#include <string.h>

#include "array.h"
#include "xmalloc.h"



void
array_free(array_t *a, void (*fn)(void *))
{
    int i;
    
    if (a == NULL)
	return;

    if (fn) {
	for (i=0; i<array_length(a); i++)
	    fn(array_get(a, i));
    }

    free(a->data);
    free(a);
}



void *
array_get(const array_t *a, int i)
{
    return (void *)(a->data + a->elem_size*i);
}



array_t *
array_new_sized(int size, int n)
{
    array_t *a;

    if (n<0)
	n = 0;

    a = xmalloc(sizeof(*a));

    if (n == 0)
	a->data = 0;
    else
	a->data = xmalloc(n*size);
    a->elem_size = size;
    a->nentry = 0;
    a->alloc_len = n;

    return a;
}



void
array_set(array_t *a, int i, const void *d)
{
    memcpy(array_get(a, i), d, a->elem_size);
}
