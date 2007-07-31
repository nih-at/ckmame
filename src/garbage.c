/*
  garbage.c -- move files to garbage directory
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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

#include "error.h"
#include "funcs.h"
#include "garbage.h"
#include "xmalloc.h"



static int garbage_open(garbage_t *);



int garbage_add(garbage_t *g, int idx)
{
    const char *name;
    char *name2;
    int ret;

    if (garbage_open(g) < 0)
	return -1;

    name = file_name(archive_file(g->sa, idx));
    if (archive_file_index_by_name(g->da, name) != -1)
	name2 = my_zip_unique_name(archive_zip(g->da), name);	/* XXX */
    else
	name2 = NULL;

    /* XXX: used ZIP_FL_UNCHANGED, why is/was that needed? */
    ret = archive_file_copy(g->sa, idx, g->da, name2 ? name2 : name);

    free(name2);
    return ret;
}



int
garbage_close(garbage_t *g)
{
    archive_t *da;

    if (g == NULL)
	return 0;

    da = g->da;

    free(g);

    if (da == NULL)
	return 0;

    /* XXX: move this to archive_commit/archive_free? */
    if (archive_num_files(da) > 0) {
	if (ensure_dir(archive_name(da), 1) < 0) {
	    archive_rollback(da);
	    archive_free(da);
	    return -1;
	}
    }

    if (archive_free(da) < 0)
	return -1;

    return 0;
}



garbage_t *garbage_new(archive_t *a)
{
    garbage_t *g;

    g = (garbage_t *)xmalloc(sizeof(*g));

    g->sa = a;
    g->da = NULL;
    g->opened = false;

    return g;
}



static int
garbage_open(garbage_t *g)
{
    char *name;

    if (!g->opened) {
	g->opened = true;
	name = make_garbage_name(archive_name(g->sa), 0);
	g->da = archive_new(name, TYPE_ROM, ARCHIVE_FL_CREATE);
	free(name);
	if (archive_ensure_zip(g->da) < 0) {
	    archive_free(g->da);
	    g->da = NULL;
	}
    }

    return (g->da != NULL ? 0 : -1);
}
