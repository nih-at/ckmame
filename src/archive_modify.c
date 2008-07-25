/*
  archive_modify.c -- functions to modify an archive
  Copyright (C) 1999-2008 Dieter Baron and Thomas Klausner

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
#include "memdb.h"
#include "xmalloc.h"

/* XXX: for TorrentZip feature check */
#include <zip.h>



static void _add_file(archive_t *, int, const char *, const file_t *);
static int _copy_chd(archive_t *, int, archive_t *, const char *, off_t, off_t);
static int _copy_zip(archive_t *, int, archive_t *, const char *, off_t, off_t);
static int _delete_chd(archive_t *, int);
static int _delete_zip(archive_t *, int);
static int _file_cmp_name(const file_t *, const file_t *);
static int _rename_chd(archive_t *, int, const char *);
static int _rename_zip(archive_t *, int, const char *);

/* keep in sync with types.h:enum filetype */
struct {
    int (*copy)(archive_t *, int, archive_t *, const char *, off_t, off_t);
    int (*delete)(archive_t *, int);
    int (*rename)(archive_t *, int, const char *);
} ops[] = {
    { _copy_zip, _delete_zip, _rename_zip },
    { NULL, NULL, NULL },			/* samples, not used */
    { _copy_chd, _delete_chd, _rename_chd },
};



int
archive_commit(archive_t *a)
{
    int i;

    if (a->flags & ARCHIVE_FL_TORRENTZIP) {
	if (a->za == NULL
	    || zip_get_archive_flag(a->za, ZIP_AFL_TORRENT, 0) == 0)
	a->flags |= ARCHIVE_IFL_MODIFIED;
    }

    if (!(a->flags & ARCHIVE_IFL_MODIFIED))
	return 0;

    /* XXX: handle disks */

    if (a->za) {
	if (!archive_is_empty(a)) {
	    if (ensure_dir(archive_name(a), 1) < 0)
		return -1;
	}

	if (a->flags & ARCHIVE_FL_TORRENTZIP) {
	    if (zip_set_archive_flag(a->za, ZIP_AFL_TORRENT, 1) < 0) {
		seterrinfo(NULL, archive_name(a));
		myerror(ERRZIP, "cannot torrentzip: %s",
			zip_strerror(a->za));
		return -1;
	    }
	}

	if (zip_close(a->za) < 0) {
	    seterrinfo(NULL, archive_name(a));
	    myerror(ERRZIP, "cannot commit changes: %s",
		    zip_strerror(a->za));
	    return -1;
	}

	a->za = NULL;
    }

    for (i=0; i<archive_num_files(a); i++) {
	switch (file_where(archive_file(a, i))) {
	case FILE_DELETED:
	    if (ARCHIVE_IS_INDEXED(a)) {
		/* XXX: handle error (how?) */
		memdb_file_delete(a, i, archive_is_rdwr(a));
	    }

	    if (archive_is_rdwr(a)) {
		array_delete(archive_files(a), i, file_finalize);
		i--;
	    }
	    break;

	case FILE_ADDED:
	    if (ARCHIVE_IS_INDEXED(a)) {
		/* XXX: handle error (how?) */
		memdb_file_insert(NULL, a, i);
	    }
	    file_where(archive_file(a, i)) = FILE_INZIP;
	    break;

	default:
	    break;
	}
    }

    if (a->flags & ARCHIVE_FL_TORRENTZIP) {
	/* Currently, only internal archives are torrtentzipped and
	   only external archives are indexed.  So we don't need to
	   worry about memdb. */

	array_sort(archive_files(a), _file_cmp_name);
    }

    return 0;
}



int
archive_file_add_empty(archive_t *a, const char *name)
{
    struct zip_source *source;
    struct hashes_update *hu;
    file_t f;

    if (archive_filetype(a) != TYPE_ROM) {
	seterrinfo(archive_name(a), NULL);
	myerror(ERRZIP, "cannot add files to disk");
	return -1;
    }

    if (archive_is_rdwr(a)) {
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
archive_file_copy_or_move(archive_t *sa, int sidx, archive_t *da,
			  const char *dname, int copyp)
{
    if (copyp)
	return archive_file_copy(sa, sidx, da, dname);
    else
	return archive_file_move(sa, sidx, da, dname);
}



int
archive_file_copy_part(archive_t *sa, int sidx, archive_t *da,
		       const char *dname, off_t start, off_t len)
{
    if (archive_filetype(sa) != archive_filetype(da)) {
	seterrinfo(archive_name(sa), NULL);
	myerror(ERRZIP, "cannot copy to archive of different type `%s'",
		archive_name(da));
	return -1;
    }

    if (file_where(archive_file(sa, sidx)) != FILE_INZIP) {
	seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
	myerror(ERRZIP, "cannot copy broken/added/deleted file");
	return -1;
    }

    if (start < 0 || (len != -1 &&
	      (len < 0 ||
	       (uint64_t)(start+len) > file_size(archive_file(sa, sidx))))) {
	seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
	/* XXX: print off_t properly */
	myerror(ERRZIP, "invalid range (%ld, %ld)",
		(long)start, (long)len);
	return -1;
    }
	
    if (archive_is_rdwr(sa))
	if (ops[archive_filetype(sa)].copy(sa, sidx, da, dname, start, len) < 0)
	    return -1;

    if (start == 0 && (len == -1
		       || (uint64_t)len == file_size(archive_file(sa, sidx))))
	_add_file(da, -1, dname, archive_file(sa, sidx));
    else {
	/* XXX: don't know checksums here */
    }

    return 0;
}




int
archive_file_delete(archive_t *a, int idx)
{
    if (file_where(archive_file(a, idx)) != FILE_INZIP) {
	seterrinfo(archive_name(a), NULL);
	myerror(ERRZIP, "cannot copy broken/added/deleted file");
	return -1;
    }
	
    if (archive_is_rdwr(a))
	if (ops[archive_filetype(a)].delete(a, idx) < 0)
	    return -1;

    file_where(archive_file(a, idx)) = FILE_DELETED;
    a->flags |= ARCHIVE_IFL_MODIFIED;

    return 0;
}



int
archive_file_move(archive_t *sa, int sidx, archive_t *da, const char *dname)
{
    if (archive_file_copy(sa, sidx, da, dname) < 0)
	return -1;

    return archive_file_delete(sa, sidx);
}

int
archive_file_rename(archive_t *a, int idx, const char *name)
{
    if (file_where(archive_file(a, idx)) != FILE_INZIP) {
	seterrinfo(archive_name(a), NULL);
	myerror(ERRZIP, "cannot copy broken/added/deleted file");
	return -1;
    }
	
    if (archive_is_rdwr(a))
	if (ops[archive_filetype(a)].rename(a, idx, name) < 0)
	    return -1;
    
    free(file_name(archive_file(a, idx)));
    file_name(archive_file(a, idx)) = xstrdup(name);

    return 0;
}



int
archive_rollback(archive_t *a)
{
    int i;

    if (!(a->flags & ARCHIVE_IFL_MODIFIED))
	return 0;

    /* XXX: unchange archive name on disk rename */

    if (a->za == NULL)
	return 0;

    if (zip_unchange_all(a->za) < 0)
	return -1;

    for (i=0; i<archive_num_files(a); i++) {
	if (file_where(archive_file(a, i)) == FILE_ADDED) {
	    array_truncate(archive_files(a), i, file_finalize);
	    break;
	}

	if (file_where(archive_file(a, i)) == FILE_DELETED)
	    file_where(archive_file(a, i)) = FILE_INZIP;

	if (strcmp(file_name(archive_file(a, i)),
		   zip_get_name(a->za, i, 0)) != 0) {
	    free(file_name(archive_file(a, i)));
	    file_name(archive_file(a, i)) = xstrdup(zip_get_name(a->za, i, 0));
	}

    }

    return 0;
}



static void
_add_file(archive_t *a, int idx, const char *name, const file_t *f)
{
    file_t *nf;

    nf = array_insert(archive_files(a), idx, f);

    file_name(nf) = xstrdup(name);
    file_where(nf) = FILE_ADDED;
    a->flags |= ARCHIVE_IFL_MODIFIED;
}



static int
_copy_chd(archive_t *sa, int sidx, archive_t *da, const char *dname,
	  off_t start, off_t len)
{
    /* XXX: build new name */
    return link_or_copy(archive_name(sa), archive_name(da));
}



static int
_copy_zip(archive_t *sa, int sidx, archive_t *da, const char *dname,
	  off_t start, off_t len)
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
	    my_zip_rename_to_unique(archive_zip(da), didx);
    }
    
    if ((source=zip_source_zip(archive_zip(da), archive_zip(sa),
			       sidx, 0, start, len)) == NULL
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
    if (archive_ensure_zip(a) < 0)
	return -1;

    if (zip_delete(a->za, idx) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot delete `%s': %s",
		zip_get_name(a->za, idx, 0), zip_strerror(a->za));
	return -1;
    }

    return 0;
}



static int
_file_cmp_name(const file_t *a, const file_t *b)
{
    return strcasecmp(file_name(a), file_name(b));
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
    if (archive_ensure_zip(a) < 0)
	return -1;

    if (my_zip_rename(a->za, idx, name) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot rename `%s' to `%s': %s",
		zip_get_name(a->za, idx, 0), name, zip_strerror(a->za));
	return -1;
    }
    
    return 0;
}
