/*
 archive_zip.c -- implementation of archive from zip
 Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner
 
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

#include "funcs.h"

#include "archive.h"
#include "error.h"
#include "globals.h"
#include "xmalloc.h"

#define BUFSIZE 8192

#define archive_zip(a)  (archive_user_data(a))

static int ensure_zip(archive_t *);
static int match_detector(struct zip *, int, file_t *);

static int op_check(archive_t *);
static int op_close(archive_t *);
static int op_commit(archive_t *);
static int op_file_add_empty(archive_t *, const char *);
static void op_file_close(void *);
static int op_file_copy(archive_t *, int, archive_t *, int, const char *, off_t, off_t);
static int op_file_delete(archive_t *, int);
static void *op_file_open(archive_t *, int);
static int op_file_rename(archive_t *, int, const char *);
static const char *op_file_strerror(void *);
static bool op_get_last_update(archive_t *a, time_t *mtime, off_t *size);
static bool op_read_infos(archive_t *);
static int op_rollback(archive_t *);

struct archive_ops ops_zip = {
    op_check,
    op_close,
    op_commit,
    NULL, /* commit_cleanup */
    op_file_add_empty,
    op_file_close,
    op_file_copy,
    op_file_delete,
    op_file_open,
    (int64_t (*)(void *, void *, uint64_t))zip_fread,
    op_file_rename,
    op_file_strerror,
    op_get_last_update,
    op_read_infos,
    op_rollback
};


int
archive_init_zip(archive_t *a)
{
    a->ops = &ops_zip;
    
    return 0;
}

static int
ensure_zip(archive_t *a)
{
    int flags;
    
    if (archive_zip(a))
	return 0;
    
    if (archive_filetype(a) != TYPE_ROM)
	return -1;
    
    flags = (a->flags & ARCHIVE_FL_CHECK_INTEGRITY) ? ZIP_CHECKCONS : 0;
    if (a->flags & ARCHIVE_FL_CREATE)
	flags |= ZIP_CREATE;
    
    if ((archive_zip(a)=my_zip_open(archive_name(a), flags)) == NULL)
	return -1;
#ifdef FD_DEBUGGING
    fprintf(stderr, "zip_open: %s\n", archive_name(a));
#endif
    
    return 0;
}


static int
op_check(archive_t *a)
{
    return ensure_zip(a);
}


static int
op_close(archive_t *a)
{
    if (archive_zip(a) == NULL)
	return 0;
    
#ifdef FD_DEBUGGING
    fprintf(stderr, "zip_close %s\n", archive_name(a));
#endif
    if (zip_close(archive_zip(a)) < 0) {
	/* error closing, so zip is still valid */
	myerror(ERRZIP, "error closing zip: %s", zip_strerror(archive_zip(a)));
        
	/* TODO: really do this here? */
	/* discard all changes and close zipfile */
	zip_discard(archive_zip(a));
	archive_zip(a) = NULL;
	return -1;
    }
    
    archive_zip(a) = NULL;
    
    return 0;
}


static int
op_commit(archive_t *a)
{
    if (a->flags & ARCHIVE_FL_TORRENTZIP) {
	if (archive_zip(a) == NULL || zip_get_archive_flag(archive_zip(a), ZIP_AFL_TORRENT, 0) == 0)
            a->flags |= ARCHIVE_IFL_MODIFIED;
    }
    
    if ((archive_flags(a) & ARCHIVE_IFL_MODIFIED) && (archive_flags(a) & ARCHIVE_FL_RDONLY) == 0) {
	if (archive_zip(a)) {
	    if (!archive_is_empty(a)) {
		if (ensure_dir(archive_name(a), 1) < 0)
		    return -1;
	    }
	    
	    if (a->flags & ARCHIVE_FL_TORRENTZIP) {
		if (zip_set_archive_flag(archive_zip(a), ZIP_AFL_TORRENT, 1) < 0) {
		    seterrinfo(NULL, archive_name(a));
		    myerror(ERRZIP, "cannot torrentzip: %s", zip_strerror(archive_zip(a)));
		    return -1;
		}
	    }
	}
    }

    if (archive_zip(a)) {
#ifdef FD_DEBUGGING
	fprintf(stderr, "zip_close %s\n", archive_name(a));
#endif
	if (zip_close(archive_zip(a)) < 0) {
	    seterrinfo(NULL, archive_name(a));
	    myerror(ERRZIP, "cannot commit changes: %s", zip_strerror(archive_zip(a)));
	    zip_discard(archive_zip(a));
	    archive_zip(a) = NULL;
	    return -1;
	}
        
	archive_zip(a) = NULL;
    }
    
    return 0;
}


static int
op_file_add_empty(archive_t *a, const char *name)
{
    struct zip_source *source;
    
    if (archive_filetype(a) != TYPE_ROM) {
	seterrinfo(archive_name(a), NULL);
	myerror(ERRZIP, "cannot add files to disk");
	return -1;
    }
    
    if (archive_is_writable(a)) {
	if (ensure_zip(a) < 0)
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
    
    return 0;
}


static void
op_file_close(void *zf)
{
    zip_fclose(zf);
}


static int
op_file_copy(archive_t *sa, int sidx, archive_t *da, int didx, const char *dname, off_t start, off_t len)
{
    struct zip_source *source;
    
    if (ensure_zip(sa) < 0 || ensure_zip(da) < 0)
	return -1;
    
    if ((source=zip_source_zip(archive_zip(da), archive_zip(sa), sidx, didx >= 0 ? ZIP_FL_UNCHANGED : 0, start, len)) == NULL
        || (didx >= 0 ? zip_replace(archive_zip(da), didx, source) : zip_add(archive_zip(da), dname, source)) < 0) {
        zip_source_free(source);
        seterrinfo(archive_name(da), dname);
        myerror(ERRZIPFILE, "error adding '%s' from `%s': %s", file_name(archive_file(sa, sidx)), archive_name(sa), zip_strerror(archive_zip(da)));
        return -1;
    }
    
    return 0;
}


static int
op_file_delete(archive_t *a, int idx)
{
    if (ensure_zip(a) < 0)
	return -1;
    
    if (zip_delete(archive_zip(a), idx) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot delete '%s': %s", zip_get_name(archive_zip(a), idx, 0), zip_strerror(archive_zip(a)));
	return -1;
    }
    
    return 0;
}


static void *
op_file_open(archive_t *a, int idx)
{
    if (ensure_zip(a) < 0)
	return NULL;

    struct zip *za = archive_zip(a);
    struct zip_file *zf;

    if ((zf = zip_fopen_index(za, idx, 0)) == NULL) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot open '%s': %s", file_name(archive_file(a, idx)), zip_strerror(za));
	return NULL;
    }

    return zf;
}


static int
op_file_rename(archive_t *a, int idx, const char *name)
{
    if (ensure_zip(a) < 0)
	return -1;
    
    if (my_zip_rename(archive_zip(a), idx, name) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot rename '%s' to `%s': %s", zip_get_name(archive_zip(a), idx, 0), name, zip_strerror(archive_zip(a)));
	return -1;
    }
    
    return 0;
}


static const char *
op_file_strerror(void *zf)
{
    return zip_file_strerror(zf);
}


static bool
op_get_last_update(archive_t *a, time_t *mtime, off_t *size)
{
    struct stat st;
    if (stat(archive_name(a), &st) < 0)
	return false;

    *mtime = st.st_mtime;
    *size = st.st_size;

    return true;
}




static bool
op_read_infos(archive_t *a)
{
    struct zip *za;
    struct zip_stat zsb;
    file_t *r;
    int i;
    
    if (ensure_zip(a) < 0)
	return false;
    
    za = (struct zip *)archive_user_data(a);
    
    seterrinfo(NULL, archive_name(a));
    
    for (i=0; i<zip_get_num_files(za); i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s", i, zip_strerror(za));
	    continue;
	}
        
	r = (file_t *)array_grow(archive_files(a), file_init);
        file_mtime(r) = zsb.mtime;
	file_size(r) = zsb.size;
	file_name(r) = xstrdup(zsb.name);
	file_status(r) = STATUS_OK;
        
	hashes_init(file_hashes(r));
	file_hashes(r)->types = HASHES_TYPE_CRC;
	file_hashes(r)->crc = zsb.crc;
        
	if (detector)
	    archive_file_match_detector(a, i);
        
	if (a->flags & ARCHIVE_FL_CHECK_INTEGRITY)
	    archive_file_compute_hashes(a, i, a->flags & ARCHIVE_FL_HASHTYPES_MASK);
    }
    
    return true;
    
}

static int
op_rollback(archive_t *a)
{
    int i;
    
    if (archive_zip(a) == NULL)
        return 0;
    
    if (zip_unchange_all(archive_zip(a)) < 0)
        return -1;
    
    for (i=0; i<archive_num_files(a); i++) {
        if (file_where(archive_file(a, i)) == FILE_ADDED) {
            array_truncate(archive_files(a), i, file_finalize);
            break;
        }
        
        if (file_where(archive_file(a, i)) == FILE_DELETED)
            file_where(archive_file(a, i)) = FILE_INZIP;
        
        if (strcmp(file_name(archive_file(a, i)), zip_get_name(archive_zip(a), i, 0)) != 0) {
            free(file_name(archive_file(a, i)));
            file_name(archive_file(a, i)) = xstrdup(zip_get_name(archive_zip(a), i, 0));
        }
    }

    return 0;
}
