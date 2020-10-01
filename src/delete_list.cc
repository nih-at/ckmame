/*
  delete_list.c -- list of files to delete
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


#include <stdlib.h>
#include <zip.h>

#include "delete_list.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "xmalloc.h"


void
delete_list_free(delete_list_t *dl) {
    if (dl == NULL)
	return;

    parray_free(dl->array, file_location_free);
    free(dl);
}


int
delete_list_execute(delete_list_t *dl) {
    int i;
    const char *name;
    const file_location_t *fbh;
    archive_t *a;
    int ret;

    delete_list_sort(dl);

    name = NULL;
    a = NULL;
    ret = 0;
    for (i = 0; i < delete_list_length(dl); i++) {
	fbh = delete_list_get(dl, i);

	if (name == NULL || strcmp(file_location_name(fbh), name) != 0) {
	    if (a) {
		if (archive_commit(a) < 0) {
		    archive_rollback(a);
		    ret = -1;
		}

		if (archive_is_empty(a))
		    remove_empty_archive(name);

		archive_free(a);
	    }

	    name = file_location_name(fbh);
	    /* TODO: don't hardcode location */
	    if ((a = archive_new(name, TYPE_ROM, FILE_NOWHERE, 0)) == NULL)
		ret = -1;
	}
	if (a) {
	    if (fix_options & FIX_PRINT)
		printf("%s: delete used file '%s'\n", name, file_name(archive_file(a, file_location_index(fbh))));
	    /* TODO: check for error */
	    archive_file_delete(a, file_location_index(fbh));
	}
    }

    if (a) {
	if (archive_commit(a) < 0) {
	    archive_rollback(a);
	    ret = -1;
	}

	if (archive_is_empty(a))
	    remove_empty_archive(name);

	archive_free(a);
    }

    return ret;
}


void
delete_list_mark(delete_list_t *dl) {
    dl->mark = parray_length(dl->array);
}


delete_list_t *
delete_list_new(void) {
    delete_list_t *dl;

    dl = static_cast<delete_list_t *>(xmalloc(sizeof(*dl)));
    dl->array = parray_new();
    dl->mark = 0;

    return dl;
}


void
delete_list_rollback(delete_list_t *dl) {
    parray_set_length(dl->array, dl->mark, NULL, file_location_free);
}


void
delete_list_used(archive_t *a, int index) {
    delete_list_t *list = NULL;
    
    switch (archive_where(a)) {
    case FILE_NEEDED:
	list = needed_delete_list;
	break;
	
    case FILE_SUPERFLUOUS:
	list = superfluous_delete_list;
	break;

    case FILE_EXTRA:
	if (fix_options & FIX_DELETE_EXTRA) {
	    list = extra_delete_list;
	}
	break;
            
    default:
        break;
    }

    if (list) {
	delete_list_add(list, archive_name(a), index);
    }
}
