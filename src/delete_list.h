#ifndef HAD_DELETE_LIST_H
#define HAD_DELETE_LIST_H

/*
  $NiH: delete_list.h,v 1.4 2006/05/05 10:38:51 dillo Exp $

  delete_list.h -- list of files to delete
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



#include "file_location.h"
#include "parray.h"



struct delete_list {
    parray_t *array;
    int mark;
};

typedef struct delete_list delete_list_t;



#define delete_list_add(dl, n, i)	\
	(parray_push((dl)->array, file_location_new((n), (i))))
			
#define delete_list_get(dl, i)	\
	((file_location_t *)parray_get((dl)->array, (i)))

#define delete_list_length(dl)	(parray_length((dl)->array))

#define delete_list_sort(dl)					\
	(parray_sort_unique(dl->array, file_location_cmp))

int delete_list_execute(delete_list_t *);
void delete_list_free(delete_list_t *);
void delete_list_mark(delete_list_t *);
delete_list_t *delete_list_new(void);
void delete_list_rollback(delete_list_t *);

#endif /* delete_list.h */
