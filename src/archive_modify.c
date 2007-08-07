/*
  archive_edit.c -- functions to modify an archive
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



#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "xmalloc.h"



static void _add_file(archive_t *, int, const char *, const file_t *);
static int _copy_chd(archive_t *, int, archive_t *, const char *);
static int _copy_zip(archive_t *, int, archive_t *, const char *);
static int _delete_chd(archive_t *, int);
static int _delete_zip(archive_t *, int);
static int _rename_chd(archive_t *, int, const char *);
static int _rename_zip(archive_t *, int, const char *);

/* keep in sync with types.h:enum filetype */
struct {
    int (*copy)(archive_t *, int, archive_t *, const char *);
    int (*delete)(archive_t *, int);
    int (*rename)(archive_t *, int, const char *);
} ops[] = {
    { _copy_zip, _delete_zip, _rename_zip },
    { NULL, NULL, NULL },			/* samples, not used */
    { _copy_chd, _delete_chd, _rename_chd },
};



int
archive_file_add_empty(archive_t *a, const char *name, bool dochange)
{
    struct zip_source *source;
    struct hashes_update *hu;
    file_t f;

    if (archive_filetype(a) != TYPE_ROM) {
	seterrinfo(archive_name(a), NULL);
	myerror(ERRZIP, "cannot add files to disk");
	return -1;
    }

    if (dochange) {
	if (archive_ensure_zip(a) < 0)
	    return -1;

	if ((source=zip_source_buffer(archive_zip(a), NULL, 0, 0)) == NULL
	    || zip_add(archive_zip(a), name, source) < 0) {
	    zip_source_free(source);
	    seterrinfo(archive_name(a), name);
	    myerror(ERRZIPFILE, "error creating empty file: %s",
		    zip_strerror(archive_zip(a)));
	    return -1;
	}
    }

    file_init(&f);
    file_size(&f) = 0;
    hashes_types(file_hashes(&f)) = romhashtypes;
    hu = hashes_update_new(file_hashes(&f));
    hashes_update_final(hu);

    _add_file(a, -1, name, &f);

    return 0;
}



int
archive_file_copy(archive_t *sa, int sidx, archive_t *da, const char *dname,
		  bool dochange)
{
    if (archive_filetype(sa) != archive_filetype(da)) {
	seterrinfo(archive_name(sa), NULL);
	myerror(ERRZIP, "cannot copy to archive of different type `%s'",
		archive_name(da));
	return -1;
    }

    if (dochange)
	if (ops[archive_filetype(sa)].copy(sa, sidx, da, dname) < 0)
	    return -1;

    _add_file(da, -1, dname, archive_file(sa, sidx));

    /* XXX: add to memdb (or do that in commit?) */

    return 0;
}



int
archive_file_copy_or_move(archive_t *sa, int sidx, archive_t *da,
			  const char *dname, int copyp, bool dochange)
{
    if (copyp)
	return archive_file_copy(sa, sidx, da, dname, dochange);
    else
	return archive_file_move(sa, sidx, da, dname, dochange);
}



int
archive_file_delete(archive_t *a, int idx, bool dochange)
{
    if (dochange)
	if (ops[archive_filetype(a)].delete(a, idx) < 0)
	    return -1;

    file_where(archive_file(a, idx)) = FILE_DELETED;

    return 0;
}



int
archive_file_move(archive_t *sa, int sidx, archive_t *da, const char *dname,
		  bool dochange)
{
    if (archive_file_copy(sa, sidx, da, dname, dochange) < 0)
	return -1;

    return archive_file_delete(sa, sidx, dochange);
}

int
archive_file_rename(archive_t *a, int idx, const char *name, bool dochange)
{
    if (dochange)
	if (ops[archive_filetype(a)].rename(a, idx, name) < 0)
	    return -1;
    
    /* XXX: update archive */

    return 0;
}



int
archive_rollback(archive_t *a)
{
    int ret, i;

    if (a->za == NULL)
	return 0;

    ret = zip_unchange_all(a->za);

    /* XXX: how to undo renames? */

    for (i=0; i<archive_num_files(a); i++) {
	switch (file_where(archive_file(a, i))) {
	    case FILE_DELETED:
		file_where(archive_file(a, i)) = FILE_INZIP;
		break;

	    case FILE_ADDED:
		array_truncate(archive_files(a), i, file_finalize);
		break;

	    default:
		break;
	    }
    }

    /* XXX: update memdb (or only in commit?) */

    return ret;
}



static void
_add_file(archive_t *a, int idx, const char *name, const file_t *f)
{
    file_t *nf;

    nf = array_insert(archive_files(a), idx, f);

    file_name(nf) = xstrdup(name);
    file_where(nf) = FILE_ADDED;
}



static int
_copy_chd(archive_t *sa, int sidx, archive_t *da, const char *dname)
{
    /* XXX: build new name */
    return link_or_copy(archive_name(sa), archive_name(da));
}



static int
_copy_zip(archive_t *sa, int sidx, archive_t *da, const char *dname)
{
    int didx;
    struct zip_source *source;

    if (file_status(archive_file(sa, sidx)) == STATUS_BADDUMP) {
	seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
	myerror(ERRZIPFILE, "not copying broken file");
	return -1;
    }

    if (archive_ensure_zip(sa) < 0 || archive_ensure_zip(da) < 0)
	return -1;

    /* if exists, delete broken file with same name */
    didx = archive_file_index_by_name(da, dname);
    if (didx >= 0) {
	if (file_status(archive_file(da, didx)) == STATUS_BADDUMP)
	    zip_delete(archive_zip(da), didx);
	else
	    didx = -1;
    }
    
    if ((source=zip_source_zip(archive_zip(da), archive_zip(sa),
			       sidx, 0, 0, -1)) == NULL
	|| zip_add(archive_zip(da), dname, source) < 0) {
	zip_source_free(source);
	seterrinfo(archive_name(da), dname);
	myerror(ERRZIPFILE, "error adding `%s' from `%s': %s",
		file_name(archive_file(sa, sidx)), archive_name(sa),
		zip_strerror(archive_zip(da)));
	if (didx >= 0)
	    zip_unchange(archive_zip(da), didx);

	return -1;
    }

    return 0;
}



static int
_delete_chd(archive_t *a, int idx)
{
    return my_remove(archive_name(a));
}



static int
_delete_zip(archive_t *a, int idx)
{
    /* XXX */
    return 0;
}



static int
_rename_chd(archive_t *a, int idx, const char *name)
{
    /* XXX */
    return 0;
}



static int
_rename_zip(archive_t *a, int idx, const char *name)
{
    /* XXX */
    return 0;
}
