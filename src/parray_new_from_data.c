/*
  $NiH$

  parray_new_from_data.c -- create array of pointers from existing data
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



parray_t *
parray_new_from_data(void **data, int n)
{
    parray_t *pa;

    pa = xmalloc(sizeof(*pa));

    pa->entry = data;
    pa->nentry = n;
    pa->alloc_len = n;

    return pa;
}
