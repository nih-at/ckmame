#ifndef HAD_DELETE_LIST_H
#define HAD_DELETE_LIST_H

/*
  delete_list.h -- list of files to delete
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "file_location.h"
#include "parray.h"


struct delete_list {
    parray_t *array;
    int mark;
};

typedef struct delete_list delete_list_t;


#define delete_list_add(dl, n, i) (parray_push((dl)->array, file_location_new((n), (i))))

#define delete_list_get(dl, i) ((file_location_t *)parray_get((dl)->array, (i)))

#define delete_list_length(dl) (parray_length((dl)->array))

#define delete_list_sort(dl) (parray_sort_unique(dl->array, file_location_cmp))

int delete_list_execute(delete_list_t *);
void delete_list_free(delete_list_t *);
void delete_list_mark(delete_list_t *);
delete_list_t *delete_list_new(void);
void delete_list_rollback(delete_list_t *);

#endif /* delete_list.h */
