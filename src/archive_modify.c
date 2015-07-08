/*
  archive_modify.c -- functions to modify an archive
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include "archive.h"
#include "dbh_cache.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"


static void _add_file(archive_t *, int, const char *, const file_t *);
static int _file_cmp_name(const file_t *, const file_t *);


int
archive_commit(archive_t *a)
{
    int i;

    if (archive_is_modified(a)) {
        seterrinfo(NULL, archive_name(a));
        
        a->cache_changed = true;

        if (a->ops->commit(a) < 0) {
            return -1;
        }
    
        for (i=0; i<archive_num_files(a); i++) {
            switch (file_where(archive_file(a, i))) {
                case FILE_DELETED:
                    if (ARCHIVE_IS_INDEXED(a)) {
                        /* TODO: handle error (how?) */
                        memdb_file_delete(a, i, archive_is_writable(a));
                    }

                    if (archive_is_writable(a)) {
                        array_delete(archive_files(a), i, file_finalize);
                        i--;
                    }
                    break;

                case FILE_ADDED:
                    if (ARCHIVE_IS_INDEXED(a)) {
                        /* TODO: handle error (how?) */
                        memdb_file_insert(NULL, a, i);
                    }
                    file_where(archive_file(a, i)) = FILE_INZIP;
                    break;

                default:
                    break;
            }
        }

        if (a->ops->commit_cleanup) {
            a->ops->commit_cleanup(a);
        }

        archive_flags(a) &= ~ARCHIVE_IFL_MODIFIED;
    }

    if (a->cache_changed) {
        if (!a->cache_db) {
            a->cache_db = dbh_cache_get_db_for_archive(archive_name(a));
        }
        if (a->cache_db) {
            if (a->cache_id > 0) {
                if (dbh_cache_delete(a->cache_db, a->cache_id) < 0) {
                    seterrdb(a->cache_db);
                    myerror(ERRDB, "%s: error writing to " DBH_CACHE_DB_NAME, archive_name(a));
                    /* TODO: handle errors */
                }
            }
            if (archive_num_files(a) > 0) {
                if (a->ops->get_last_update) {
                    a->ops->get_last_update(a, &archive_mtime(a), &archive_size(a)); /* TODO: handle errors */
                }

                a->cache_id = dbh_cache_write(a->cache_db, a->cache_id, a);
                if (a->cache_id < 0) {
                    seterrdb(a->cache_db);
                    myerror(ERRDB, "%s: error writing to " DBH_CACHE_DB_NAME, archive_name(a));
                    a->cache_id = 0;
                }
            }
            else {
                a->cache_id = 0;
            }
        }
        else {
            a->cache_id = 0;
        }
        a->cache_changed = false;
    }

    return 0;
}


int
archive_file_add_empty(archive_t *a, const char *name)
{
    struct hashes_update *hu;
    file_t f;
    
    if (!archive_is_writable(a)) {
	seterrinfo(archive_name(a), NULL);
        myerror(ERRZIP, "cannot add to read-only archive");
	return -1;
    }

    if (a->ops->file_add_empty(a, name) < 0)
        return -1;
    
    file_init(&f);
    file_size(&f) = 0;
    hashes_types(file_hashes(&f)) = HASHES_TYPE_ALL;
    hu = hashes_update_new(file_hashes(&f));
    hashes_update_final(hu);

    _add_file(a, -1, name, &f);

    return 0;
}


int
archive_file_copy(archive_t *sa, int sidx, archive_t *da, const char *dname)
{
    return archive_file_copy_part(sa, sidx, da, dname, 0, -1, archive_file(sa, sidx));
}

int
archive_file_copy_or_move(archive_t *sa, int sidx, archive_t *da, const char *dname, int copyp)
{
    if (copyp)
        return archive_file_copy(sa, sidx, da, dname);
    else
        return archive_file_move(sa, sidx, da, dname);
}


int
archive_file_copy_part(archive_t *sa, int sidx, archive_t *da, const char *dname, off_t start, off_t len, const file_t *f)
{
    if (!archive_is_writable(da)) {
	seterrinfo(archive_name(da), NULL);
        myerror(ERRZIP, "cannot add to read-only archive");
	return -1;
    }

    if (archive_filetype(sa) != archive_filetype(da)) {
        seterrinfo(archive_name(sa), NULL);
        myerror(ERRZIP, "cannot copy to archive of different type '%s'", archive_name(da));
        return -1;
    }
    if (archive_file_index_by_name(da, dname) != -1) {
	seterrinfo(archive_name(da), NULL);
	errno = EEXIST;
        myerror(ERRZIP, "can't copy to %s: %s", dname, strerror(errno));
        return -1;
    }
    seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
    if (file_status(archive_file(sa, sidx)) == STATUS_BADDUMP) {
	myerror(ERRZIPFILE, "not copying broken file");
	return -1;
    }
    if (file_where(archive_file(sa, sidx)) != FILE_INZIP && file_where(archive_file(sa, sidx)) != FILE_DELETED) {
        myerror(ERRZIP, "cannot copy broken/added file");
        return -1;
    }
    if (start < 0 || (len != -1 && (len < 0 || (uint64_t)(start+len) > file_size(archive_file(sa, sidx))))) {
        /* TODO: print off_t properly */
        myerror(ERRZIP, "invalid range (%ld, %ld)", (long)start, (long)len);
        return -1;
    }

    if (start == 0 && (len == -1 || (uint64_t)len == file_size(archive_file(sa, sidx))))
	_add_file(da, -1, dname, archive_file(sa, sidx));
    else
	_add_file(da, -1, dname, f);

    if (archive_is_writable(da)) {
        if (sa->ops->file_copy(sa, sidx, da, -1, dname, start, len) < 0) {
            array_delete(archive_files(da), archive_num_files(da)-1, file_finalize);
            return -1;
        }
    }


    return 0;
}



int
archive_file_delete(archive_t *a, int idx)
{
    if (!archive_is_writable(a)) {
	seterrinfo(archive_name(a), NULL);
        myerror(ERRZIP, "cannot delete from read-only archive");
	return -1;
    }

    if (file_where(archive_file(a, idx)) != FILE_INZIP) {
        seterrinfo(archive_name(a), NULL);
        myerror(ERRZIP, "cannot delete broken/added/deleted file");
        return -1;
    }
	
    if (archive_is_writable(a))
        if (a->ops->file_delete(a, idx) < 0)
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
    seterrinfo(archive_name(a), NULL);

    if (!archive_is_writable(a)) {
        myerror(ERRZIP, "cannot rename in read-only archive");
	return -1;
    }
    if (file_where(archive_file(a, idx)) != FILE_INZIP) {
        myerror(ERRZIP, "cannot copy broken/added/deleted file");
        return -1;
    }

    if (archive_file_index_by_name(a, name) != -1) {
	errno = EEXIST;
        myerror(ERRZIP, "can't rename %s to %s: %s", file_name(archive_file(a, idx)), name, strerror(errno));
        return -1;
    }

    if (archive_is_writable(a)) {
        if (a->ops->file_rename(a, idx, name) < 0)
            return -1;
    }
    free(file_name(archive_file(a, idx)));
    file_name(archive_file(a, idx)) = xstrdup(name);
    a->flags |= ARCHIVE_IFL_MODIFIED;

    return 0;
}


int
archive_file_rename_to_unique(archive_t *a, int idx)
{
    if (!archive_is_writable(a)) {
	seterrinfo(archive_name(a), NULL);
        myerror(ERRZIP, "cannot rename in read-only archive");
	return -1;
    }
    
    char *new_name = archive_make_unique_name(a, file_name(archive_file(a, idx)));
    if (new_name == NULL)
        return -1;
    
    int ret = archive_file_rename(a, idx, new_name);
    free(new_name);
    return ret;
}


int
archive_rollback(archive_t *a)
{
    int i, ret;

    if (!archive_is_modified(a))
        return 0;

    if ((ret = a->ops->rollback(a)) < 0) {
	return -1;
    }

    archive_flags(a) &= ~ARCHIVE_IFL_MODIFIED;

    for (i=0; i<archive_num_files(a); i++) {
	switch (file_where(archive_file(a, i))) {
	case FILE_DELETED:
	    file_where(archive_file(a, i)) = FILE_INZIP;
	    break;
	    
	case FILE_ADDED:
	    array_delete(archive_files(a), i, file_finalize);
	    i--;
	    break;
	    
	default:
	    break;
	}
    }

    return ret;
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
_file_cmp_name(const file_t *a, const file_t *b)
{
    return strcasecmp(file_name(a), file_name(b));
}
