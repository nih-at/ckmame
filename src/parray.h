#ifndef _HAD_PARRAY_H
#define _HAD_PARRAY_H

/*
  $NiH$

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
#define parray_bsearch(pa, key, cmp)	\
	(bsearch((key), (pa)->entry, parray_length(pa), sizeof(void *), (cmp)))

void parray_delete(parray_t *, int, void (*)(void *));
void parray_free(parray_t *, void (*)(void *));
parray_t *parray_new(void);
parray_t *parray_new_from_data(void **, int);
void parray_push(parray_t *, void *);
void parray_sort(parray_t *, int (*)(const void *, const void *));

#endif /* parray.h */
