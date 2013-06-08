/*
  archive.c -- information about an archive
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"



int _archive_global_flags = 0;

archive_t *
archive_by_id(int id)
{
    archive_t *a;

    a = (archive_t *)memdb_get_ptr_by_id(id);

    if (a != NULL)
	a->refcount++;

    return a;
}

int
archive_check(archive_t *a)
{
    return a->ops->check(a);
}



int
archive_close(archive_t *a)
{
    return a->ops->close(a);
}


int
archive_file_compute_hashes(archive_t *a, int idx, int hashtypes)
{
    return a->ops->file_compute_hashes(a, idx, hashtypes);
}


off_t
archive_file_find_offset(archive_t *a, int idx, int size, const hashes_t *h)
{
    return a->ops->file_find_offset(a, idx, size, h);
}



int
archive_file_index_by_name(const archive_t *a, const char *name)
{
    int i;

    for (i=0; i<archive_num_files(a); i++) {
	if (strcmp(file_name(archive_file(a, i)), name) == 0) {
	    if (file_where(archive_file(a, i)) == FILE_DELETED)
		return -1;
	    return i;
	}
    }

    return -1;
}



int
archive_free(archive_t *a)
{
    int ret;

    if (a == NULL)
	return 0;

    if (--a->refcount != 0)
	return 0;

    if (a->flags & ARCHIVE_IFL_MODIFIED) {
	/* XXX: warn about freeing modified archive */
    }

    ret = archive_close(a);

    if (a->flags & ARCHIVE_FL_NOCACHE)
	archive_real_free(a);

    return ret;
}



void
archive_global_flags(int fl, bool setp)
{
    if (setp)
	_archive_global_flags |= fl;
    else
	_archive_global_flags &= ~fl;
}



archive_t *
archive_new(const char *name, filetype_t ft, where_t where, int flags)
{
    archive_t *a;
    int i, id;

    if ((a=memdb_get_ptr(name)) != 0) {
	/* XXX: check for compatibility of a->flags and flags */
	a->refcount++;
	return a;
    }

    a = xmalloc(sizeof(*a));
    a->id = -1;
    a->name = xstrdup(name);
    a->refcount = 1;
    a->where = where;
    a->files = array_new(sizeof(file_t));
    a->ud = NULL;
    a->flags = ((flags|_archive_global_flags) & ARCHIVE_FL_MASK);

    switch (ft) {
    case TYPE_ROM:
    case TYPE_SAMPLE:
	archive_filetype(a) = TYPE_ROM;
        if (archive_init_zip(a) < 0) {
            archive_real_free(a);
            return NULL;
        }
        if (archive_read_infos(a) < 0) {
            if (!(a->flags & ARCHIVE_FL_CREATE)) {
                archive_real_free(a);
                return NULL;
            }
        }
	break;

    case TYPE_DISK:
	archive_filetype(a) = TYPE_DISK;
	/* XXX */
	break;

    default:
	archive_real_free(a);
	return NULL;
    }

    for (i=0; i<archive_num_files(a); i++) {
	/* XXX: file_state(archive_file(a, i)) = FILE_UNKNOWN; */
	file_where(archive_file(a, i)) = FILE_INZIP;
    }

    if (!(a->flags & ARCHIVE_FL_NOCACHE)) {
	if ((id=memdb_put_ptr(name, a)) < 0) {
	    archive_real_free(a);
	    return NULL;
	}
	a->id = id;

	if (IS_EXTERNAL(archive_where(a)))
	    memdb_file_insert_archive(a);
    }

    return a;
}




int
archive_read_infos(archive_t *a)
{
    return a->ops->read_infos(a);
}


void
archive_real_free(archive_t *a)
{
    if (a == NULL)
	return;

    archive_close(a);
    free(a->name);
    array_free(archive_files(a), file_finalize);
    free(a);
}



int
archive_refresh(archive_t *a)
{
    archive_close(a);
    array_truncate(archive_files(a), 0, file_finalize);
    archive_read_infos(a);

    return 0;
}



bool
archive_is_empty(const archive_t *a)
{
    int i;

    for (i=0; i<archive_num_files(a); i++)
	if (file_where(archive_file(a, i)) != FILE_DELETED)
	    return false;

    return true;
}
