#ifndef _HAD_ARRAY_H
#define _HAD_ARRAY_H

/*
  $NiH$

  array.h -- array of arbitrary types
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



struct array {
    char *data;
    int elem_size;
    int nentry;
    int alloc_len;
};

typedef struct array array_t;



#define array_get(a, i)		((void *)((a)->data+(a)->elem_size*(i)))
#define array_length(a)		((a)->nentry)
#define array_new(s)		(array_new_sized((s), 0))

void array_delete(array_t *, int, void (*)(/* void * */));
void array_free(array_t *, void (*)(/* void * */));
void array_grow(array_t *, void (*)(/* void * */));
array_t *array_new_sized(int, int);
array_t *array_new_length(int, int, void (*)(/* void * */));
void array_push(array_t *, void *);

#endif /* array.h */
