/*
  $NiH$

  parray_push.c -- append element to end of array of pointers
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

#include <string.h>

#include "parray.h"



void
parray_delete(parray_t *pa, int index, void (*fn)(void *))
{
    if (index < 0 || index >= parray_length(pa))
	return;

    if (fn)
	fn(parray_get(pa, index));
    
    memmove(pa->entry+index, pa->entry+index+1,
	    sizeof(pa->entry[0]) * parray_length(pa)-index-1);
    pa->nentry--;
}
