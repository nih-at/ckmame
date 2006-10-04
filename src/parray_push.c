/*
  $NiH: parray_push.c,v 1.1 2005/07/07 22:00:20 dillo Exp $

  parray_push.c -- append element to end of array of pointers
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

#include "parray.h"
#include "xmalloc.h"



void
parray_push(parray_t *pa, void *e)
{
    if (pa->nentry >= pa->alloc_len) {
	if (pa->alloc_len == 0)
	    pa->alloc_len = 1;
	else
	    pa->alloc_len *= 2;
	pa->entry = xrealloc(pa->entry, sizeof(pa->entry[0])*pa->alloc_len);
    }

    pa->entry[pa->nentry++] = e;
}
