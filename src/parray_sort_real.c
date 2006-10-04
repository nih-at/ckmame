/*
  $NiH: parray_sort_real.c,v 1.2 2005/12/24 11:28:45 dillo Exp $

  parray_sort.c -- sort array of pointers
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include "parray.h"
#include "util.h"



void
parray_sort_real(parray_t *pa, int omit_duplicates, int lo, int hi,
		 int (*cmp)(const void *, const void *))
{
    int n, shrink;
    void **data;

    if (lo < 0)
	lo = 0;
    if (hi == -1 || hi > parray_length(pa))
	hi = parray_length(pa);
    if (hi < lo)
	hi = lo;

    if (hi-lo < 2)
	return;

    data = pa->entry+lo;
    n = psort(data, hi-lo, omit_duplicates, cmp);
    shrink = ((hi-lo) - n);

    if (shrink > 0 && hi < parray_length(pa))
	memmove(data+n, data+n+shrink,
		(parray_length(pa)-hi) * sizeof(pa->entry[0]));

    parray_length(pa) -= shrink;
}
