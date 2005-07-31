#ifndef HAD_DELETE_LIST_H
#define HAD_DELETE_LIST_H

/*
  $NiH$

  delete_list.h -- list of files to delete
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



#include "file_by_hash.h"
#include "parray.h"



struct delete_list {
    parray_t *array;
    int mark;
};

typedef struct delete_list delete_list_t;



#define delete_list_add(dl, n, i)	\
	(parray_push((dl)->array, file_by_hash_new((n), (i))))
			
#define delete_list_get(dl, i)	\
	((file_by_hash_t *)parray_get((dl)->array, (i)))

#define delete_list_length(dl)	(parray_length((dl)->array))
#define delete_list_mark(dl)	((dl)->mark = parray_length((dl)->array))

#define delete_list_rollback(dl)			\
	(parray_set_length((dl)->array, (dl)->mark,	\
			   NULL, file_by_hash_free))

void delete_list_free(delete_list_t *);
int delete_list_execute(delete_list_t *);
delete_list_t *delete_list_new(void);

#endif /* delete_list.h */
