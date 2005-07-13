/*
  $NiH$

  array_delete.c -- delete element from array of arbitrary types
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

#include "array.h"



void
array_delete(array_t *a, int index, void (*fn)(void *))
{
    if (index < 0 || index >= array_length(a))
	return;

    if (fn)
	fn(array_get(a, index));
    
    memmove(a->data + index*a->elem_size,
	    a->data + (index+1)*a->elem_size,
	    a->elem_size * (array_length(a)-index-1));
    a->nentry--;
}
