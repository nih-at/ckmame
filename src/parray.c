/*
  $NiH: parray.c,v 1.1 2005/07/07 22:00:20 dillo Exp $

  parray.c -- create / free array of pointers
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include "parray.h"
#include "xmalloc.h"



void
parray_free(parray_t *pa, void (*fn)(void *))
{
    int i;
    
    if (pa == NULL)
	return;

    if (fn) {
	for (i=0; i<parray_length(pa); i++)
	    fn(parray_get(pa, i));
    }

    free(pa->entry);
    free(pa);
}



parray_t *
parray_new_sized(int n)
{
    parray_t *pa;

    if (n<0)
	n = 0;

    pa = xmalloc(sizeof(*pa));

    if (n == 0)
	pa->entry = 0;
    else
	pa->entry = xmalloc(n*sizeof(pa->entry[0]));
    pa->nentry = 0;
    pa->alloc_len = n;

    return pa;
}
