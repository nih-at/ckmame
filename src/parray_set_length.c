/*
  $NiH$

  parray_set_length.c -- set length of parary
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
#include  "xmalloc.h"




void
parray_set_length(parray_t *pa, int len, void *(*fn_alloc)(void),
		  void (*fn_free)(/* void * */))
{
    int i;
    
    if (len == parray_length(pa))
	return;
    else if (len < parray_length(pa)) {
	for (i=len; i<parray_length(pa); i++)
	    fn_free(parray_get(pa, i));
    }
    else {
	if (len >= pa->alloc_len) {
	    pa->alloc_len = len;
	    pa->entry = xrealloc(pa->entry,
				 sizeof(pa->entry[0])*pa->alloc_len);
	    
	    for (i=parray_length(pa); i<len; i++)
		parray_get(pa, i) = fn_alloc();
	}
    }

    parray_length(pa) = len;
}
