/*
  $NiH: array_grow.c,v 1.3 2006/05/05 10:38:51 dillo Exp $

  array_grow.c -- grow array of arbitrary types by one element
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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

#include "array.h"
#include "xmalloc.h"



void *
array_grow(array_t *a, void (*fn)(void *))
{
    void *ne;

    if (a->nentry >= a->alloc_len) {
	if (a->alloc_len == 0)
	    a->alloc_len = 1;
	else
	    a->alloc_len *= 2;
	a->data = xrealloc(a->data, a->elem_size*a->alloc_len);
    }
    a->nentry++;

    ne = array_get(a, array_length(a)-1);

    if (fn)
	fn(ne);

    return ne;
}
