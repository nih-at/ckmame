/*
  $NiH: delete_list.c,v 1.10 2006/05/05 10:38:51 dillo Exp $

  delete_list.c -- list of files to delete
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

static int my_zip_close(struct zip *, const char *);



void
delete_list_free(delete_list_t *dl)
{
    if (dl == NULL)
	return;

    parray_free(dl->array, file_location_free);
    free(dl);
}



int
delete_list_execute(delete_list_t *dl)
{
    int i;
    const char *name;
    const file_location_t *fbh;
    struct zip *z;
    int ret, deleted;

    delete_list_sort(dl);

    name = NULL;
    z = NULL;
    deleted = 0;
    ret = 0;
    for (i=0; i<delete_list_length(dl); i++) {
	fbh = delete_list_get(dl, i);

	if (name == NULL || strcmp(file_location_name(fbh), name) != 0) {
	    if (z && deleted == zip_get_num_files(z))
		remove_empty_archive(name);

	    if (my_zip_close(z, name) == -1)
		ret = -1;

	    name = file_location_name(fbh);
	    if ((z=my_zip_open(name, 0)) == NULL)
		ret = -1;
	    deleted = 0;
	}
	if (z) {
	    if (fix_options & FIX_PRINT)
		printf("%s: delete used file `%s'\n",
		       name, zip_get_name(z, file_location_index(fbh), 0));
	    /* XXX: check for error */
	    zip_delete(z, file_location_index(fbh));
	    deleted++;
	}
    }

    if (z && deleted == zip_get_num_files(z))
	remove_empty_archive(name);

    if (my_zip_close(z, name) == -1)
	ret = -1;
    
    return ret;
}



void
delete_list_mark(delete_list_t *dl)
{
    dl->mark = parray_length(dl->array);
}



delete_list_t *
delete_list_new(void)
{
    delete_list_t *dl;

    dl = xmalloc(sizeof(*dl));
    dl->array = parray_new();
    dl->mark = 0;

    return dl;
}



void
delete_list_rollback(delete_list_t *dl)
{
    parray_set_length(dl->array, dl->mark, NULL, file_location_free);
}



static int
my_zip_close(struct zip *z, const char *name)
{
    if (z) {
	if (zip_close(z) < 0) {
	    seterrinfo(NULL, name);
	    myerror(ERRZIP, "cannot delete files: %s", zip_strerror(z));
	    zip_unchange_all(z);
	    zip_close(z);
	    return -1;
	}
    }

    return 0;
}
