/*
  archive_modify.c -- functions to modify an archive
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



#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "xmalloc.h"


static void _add_file(archive_t *, int, const char *, const file_t *);
static int _file_cmp_name(const file_t *, const file_t *);


int
archive_commit(archive_t *a)
{
    int i;
    
    if (a->ops->commit(a) < 0)
        return -1;
    
    for (i=0; i<archive_num_files(a); i++) {
	switch (file_where(archive_file(a, i))) {
            case FILE_DELETED:
                if (ARCHIVE_IS_INDEXED(a)) {
                    /* XXX: handle error (how?) */
                    memdb_file_delete(a, i, archive_is_writable(a));
                }
                
                if (archive_is_writable(a)) {
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
	/* Currently, only internal archives are torrentzipped and
         only external archives are indexed.  So we don't need to
         worry about memdb. */
        
	array_sort(archive_files(a), _file_cmp_name);
    }
    
    return 0;
}


int
archive_file_add_empty(archive_t *a, const char *name)
{
    struct hashes_update *hu;
    file_t f;
    
    if (a->ops->file_add_empty(a, name) < 0)
        return -1;
    
    file_init(&f);
    file_size(&f) = 0;
    hashes_types(file_hashes(&f)) = romhashtypes;
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
    if (archive_filetype(sa) != archive_filetype(da)) {
        seterrinfo(archive_name(sa), NULL);
        myerror(ERRZIP, "cannot copy to archive of different type `%s'", archive_name(da));
        return -1;
    }
    if (file_status(archive_file(sa, sidx)) == STATUS_BADDUMP) {
	seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
	myerror(ERRZIPFILE, "not copying broken file");
	return -1;
    }
    if (file_where(archive_file(sa, sidx)) != FILE_INZIP) {
        seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
        myerror(ERRZIP, "cannot copy broken/added/deleted file");
        return -1;
    }
    if (start < 0 || (len != -1 && (len < 0 || (uint64_t)(start+len) > file_size(archive_file(sa, sidx))))) {
        seterrinfo(archive_name(sa), file_name(archive_file(sa, sidx)));
        /* XXX: print off_t properly */
        myerror(ERRZIP, "invalid range (%ld, %ld)", (long)start, (long)len);
        return -1;
    }

    /* if exists, delete broken file with same name */
    bool replace = false;
    int didx = archive_file_index_by_name(da, dname);
    if (didx >= 0) {
	if (sa == da && sidx == didx)
	    replace = true;
	else {
	    if (file_status(archive_file(da, didx)) == STATUS_BADDUMP) {
		if (archive_file_delete(da, didx) < 0)
		    return -1;
	    }
	    else {
		if (archive_file_rename_to_unique(da, didx) < 0)
		    return -1;
	    }
	}
    }

    if (archive_is_writable(da)) {
        if (sa->ops->file_copy(sa, sidx, da, replace ? didx : -1, dname, start, len) < 0) {
	    /** \todo undo rename_to_unique? */	       
            return -1;
	}
    }

    if (replace) {
	/** \todo update archive_file(sa, sidx) */
    }
    else {
	if (start == 0 && (len == -1 || (uint64_t)len == file_size(archive_file(sa, sidx))))
	    _add_file(da, -1, dname, archive_file(sa, sidx));
	else
	    _add_file(da, -1, dname, f);
    }

    return 0;
}




int
archive_file_delete(archive_t *a, int idx)
{
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
    if (file_where(archive_file(a, idx)) != FILE_INZIP) {
        seterrinfo(archive_name(a), NULL);
        myerror(ERRZIP, "cannot copy broken/added/deleted file");
        return -1;
    }
	
    if (archive_is_writable(a))
        if (a->ops->file_rename(a, idx, name) < 0)
            return -1;
    
    free(file_name(archive_file(a, idx)));
    file_name(archive_file(a, idx)) = xstrdup(name);

    return 0;
}


int
archive_file_rename_to_unique(archive_t *a, int idx)
{
    if (!archive_is_writable(a))
        return 0;
    
    char *new_name = a->ops->file_rename_unique(a, idx);
    if (new_name == NULL)
        return -1;
    
    file_t *r = archive_file(a, idx);
    
    free(file_name(r));
    file_name(r) = new_name;

    return 0;
}

int
archive_rollback(archive_t *a)
{
    if (!(a->flags & ARCHIVE_IFL_MODIFIED))
        return 0;

    return a->ops->rollback(a);
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


#if 0
/* some not used here, but prototype must match others */
/* ARGSUSED */
static int
_copy_chd(archive_t *sa, int sidx, archive_t *da, const char *dname,
	  off_t start, off_t len)
{
    /* XXX: build new name */
    return link_or_copy(archive_name(sa), archive_name(da));
}
#endif



/* ARGSUSED */
static int
_delete_chd(archive_t *a, int idx)
{
    return my_remove(archive_name(a));
}




/* ARGSUSED */
static int
_rename_chd(archive_t *a, int idx, const char *name)
{
    /* XXX */
    return 0;
}
