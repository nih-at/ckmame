#ifndef _HAD_PARRAY_H
#define _HAD_PARRAY_H

/*
  $NiH: parray.h,v 1.2 2005/07/13 17:42:20 dillo Exp $

  parray.h -- array of pointers
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



struct parray {
    void **entry;
    int nentry;
    int alloc_len;
};

typedef struct parray parray_t;



#define parray_get(a, i)	((a)->entry[i])
#define parray_length(a)	((a)->nentry)
#define parray_sort(a, cmp)	(parray_sort_real((a), 0, -1, -1, (cmp)))
#define parray_sort_part(a, lo, hi, cmp) \
	(parray_sort_real((a), 0, (lo), (hi), (cmp)))
#define parray_sort_unique(a, cmp) (parray_sort_real((a), 1, -1, -1, (cmp)))
#define parray_new()		(parray_new_sized(0))


void parray_delete(parray_t *, int, void (*)(/* void * */));
void parray_free(parray_t *, void (*)(/* void * */));
int parray_index_sorted(const parray_t *, const void *,
			int (*)(/* const void *, const void * */));
parray_t *parray_new_sized(int);
parray_t *parray_new_from_data(void **, int);
void parray_push(parray_t *, void *);
void parray_set_length(parray_t *, int, void *(*)(void),
		       void (*)(/* void * */));
void parray_sort_real(parray_t *, int, int, int,
		      int (*)(/* const void *, const void * */));

#endif /* parray.h */
