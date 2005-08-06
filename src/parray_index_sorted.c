/*
  $NiH: parray_index_sorted.c,v 1.1 2005/07/13 17:42:20 dillo Exp $

  parray_index.c -- find index of element
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

#include "parray.h"



/* return index of ELEM in sorted (by CMP) parray PA, -1 if not found */

int
parray_index_sorted(const parray_t *pa, const void *elem,
		    int (*cmp)(const void *, const void *))
{
    int hi, lo, mid, c;

    lo = 0;
    hi = parray_length(pa)-1;

    while (lo <= hi) {
	mid = lo + (hi-lo)/2;
	c = cmp(elem, parray_get(pa, mid));
	if (c == 0)
	    return mid;
	else if (c < 0)
	    hi = mid-1;
	else
	    lo = mid+1;
    }

    return -1;	
}
