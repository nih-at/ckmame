/*
 archive_dir.c -- implementation of archive from directory
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

#include "funcs.h"

#include "archive.h"
#include "error.h"
#include "globals.h"
#include "xmalloc.h"

#define BUFSIZE 8192

typedef enum {
    CHANGE_NONE,
    CHANGE_ADD,
    CHANGE_REPLACE,
    CHANGE_DELETE,
    CHANGE_RENAME
} change_type_t;

typedef struct {
    change_type_t type;
    char *current_name;
    char *final_name;
    time_t mtime;
} change_t;

typedef struct {
    dbh_t *db;
    int id;
    array_t *change;
} ud_t;

static ud_t *ud_new(int);

static int match_detector(struct zip *, int, file_t *);

static int op_check(archive_t *);
static int op_commit(archive_t *);
static int op_file_add_empty(archive_t *, const char *);
static int op_file_copy(archive_t *, int, archive_t *, const char *, off_t, off_t);
static int op_file_delete(archive_t *, int);
static void *op_file_open(archive_t *, int);
static int op_file_read(void *, char *, int);
static int op_file_rename(archive_t *, int, const char *);
static const char *op_file_strerror(void *);
static int op_read_infos(archive_t *);
static int op_rollback(archive_t *);

struct archive_ops ops_dir = {
    op_check,
    op_commit,
    op_commit,
    op_file_add_empty,
    (void (*)(void *))fclose,
    op_file_copy,
    op_file_delete,
    op_file_open,
    op_file_read,
    op_file_rename,
    op_file_strerror,
    op_read_infos,
    op_rollback
};


int
archive_init_dir(archive_t *a)
{
    a->ops = &ops_dir;

    if ((a->ud = ud_new(archive_num_files(a))) == NULL)
	return -1;

    switch (archive_where(a)) {
    case FILE_ROMSET:
	/** \todo ensure and set ROMs file map */
	break;

    case FILE_NEEDED:
	/** \todo ensure and set needed file map */
	break;
    }
    
    return 0;
}


static void
change_fini(change_t *ch)
{
    free(ch->current_name);
    free(ch->final_name);
}


static void
change_init(change_t *ch)
{
    ch->type = CHANGE_NONE;
    ch->current_name = ch->final_name = NULL;
    ch->mtime = 0;
}


static void
change_rollback(change_t *ch)
{
    switch (ch->type) {
    case CHANGE_NONE:
    case CHANGE_DELETE:
    case CHANGE_RENAME:
	break;
	
    case CHANGE_ADD:
    case CHANGE_REPLACE:
	unlink(ch->current_name);
	break;
    }

    change_fini(ch);
    change_init(ch);
}

static int
change_set(change_t *ch, change_type_t type, char *current_name, char *final_name)
{
    struct stat st;

    if (ch->type != CHANGE_NONE) {
	/** \todo shouldn't happen, investigate if it does */
	fprintf(stderr, "WARNING: discarding old change\n");
    }

    if (current_name) {
	if (stat(current_name, &st) < 0)
	    return -1;
    }

    change_rollback(ch);

    ch->type = type;
    ch->final_name = final_name;
    ch->current_name = current_name;
    if (current_name)
	ch->mtime = st.st_mtime;
    else
	ch->mtime = 0;

    return 0;
}


static int
ensure_dir(archive_t *a)
{
    stuct stat st;

    if (stat(archive_name(a), &st) < 0) {
	if (errno == ENOENT && (archive_flags(a) & ARCHIVE_FLAGS_CREATE))
	    return mkdir(archive_name(a), 0777);
	return -1;
    }
    return 0;
}


static int
match_detector(struct zip *za, int idx, file_t *r)
{
    struct zip_file *zf;
    int ret;
    
    if ((zf=zip_fopen_index(za, idx, 0)) == NULL) {
        myerror(ERRZIP, "error opening index %d: %s", idx, zip_strerror(za));
        file_status(r) = STATUS_BADDUMP;
        return -1;
    }
    
    ret = detector_execute(detector, r, (detector_read_cb)zip_fread, zf);
    
    zip_fclose(zf);
    
    return ret;
}


static int
op_check(archive_t *a)
{
    struct stat st;

    if (!is_writable_directory(archive_name(a))) {
	if (errno == ENOENT && (a->flags & ARCHIVE_FLAG_CREATE)) {
	    char *parent = mydirname(archive_name(a));
	    int ret = is_writable_directory(parent);
	    free(parent);
	    return ret;
	}
	return -1;
    }
    
    return 0;
}


static int
op_commit(archive_t *a)
{
    if (a->flags & ARCHIVE_FL_TORRENTZIP) {
	if (archive_zip(a) == NULL || zip_get_archive_flag(archive_zip(a), ZIP_AFL_TORRENT, 0) == 0)
            a->flags |= ARCHIVE_IFL_MODIFIED;
    }
    
    if (!(a->flags & ARCHIVE_IFL_MODIFIED))
	return 0;
    
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
        
#ifdef FD_DEBUGGING
	fprintf(stderr, "zip_close %s\n", archive_name(a));
#endif
	if (zip_close(archive_zip(a)) < 0) {
	    seterrinfo(NULL, archive_name(a));
	    myerror(ERRZIP, "cannot commit changes: %s", zip_strerror(archive_zip(a)));
	    return -1;
	}
        
	archive_zip(a) = NULL;
    }
    
    return 0;
}


static int
op_file_add_empty(archive_t *a, const char *name)
{
    return op_file_copy(NULL, -1, a, -1, name, 0, 0);
}


static int
op_file_copy(archive_t *sa, int sidx, archive_t *da, int didx, const char *dname, off_t start, off_t len)
{
    if (ensure_dir(da) < 0)
	return -1;

    char *tmpname = mktmpname(da, dname);

    if (tmpname == NULL)
	return -1;

    char *srcname;
    
    if (sa) {
	if ((srcname = get_full_name(sa, sidx)) == NULL) {
	    free(tmpname);
	    return -1;
	}
    }

    if (srcname && start == 0 && (len == -1 || (uint64_t)len == file_size(archive_file(sa, sidx)))) {
	if (link_or_copy(tmpname, srcname) < 0) {
	    free(tmpname);
	    free(srcname);
	    return -1;
	}
    }
    else {
	if (srcname) {
	    if (copy_file(srcname, tmpname, start, len) < 0) {
		/** \todo error message */
		free(tmpname);
		free(srcname);
		return -1;
	    }
	    free(srcname);
	}
	else {
	    FILE *fout;

	    if ((fout = fopen(tmpname, "w")) == NULL) {
		/** \todo error message */
		free(tmpname);
		return -1;
	    }
	    fclose(fout);
	}
    }

    ud_t *ud = archive_user_data(a);
    change_t *ch;

    if (didx >= 0)
	ch = archive_get(ud->change, didx);
    else {
	if ((ch = archive_grow(ud->change, change_init)) == NULL) {
	    /** \todo error message */
	    unlink(tmpname);
	    free(tmpname);
	    return -1;
	}
    }

    if (change_set(ch, didx >= 0 ? CHANGE_REPLACE : CHANGE_ADD, tmpname) < 0) {
	/** \todo error message */
	unlink(tmpname);
	free(tmpname);
	return -1;
    }
    
    return 0;
}


static int
op_file_delete(archive_t *a, int idx)
{
    struct stat st;
    
    char *full_name = get_full_name(a, idx);

    if (stat(full_name, &st) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot delete `%s': %s", file_name(archive_file(a, idx), strerror(errno)));
	free(full_name);
	return -1;
    }
    
    ud_t *ud = archive_user_data(a);
    
    change_set(array_get(ud->change, idx), CHANGE_DELETE, full_name, NULL);
    
    return 0;
}


static void *
op_file_open(archive_t *a, int idx)
{
    FILE *f;

    char *full_name = get_full_name(a, idx);

    if ((f=fopen(full_name, "r")) == NULL) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot open `%s': %s", file_name(archive_file(a, idx), strerror(errno)));
	free(full_name);
	return NULL;
    }

    free(full_name);

    return f;
}


static int64_t
op_file_read(void *f, char *buf, uint64_t n)
{
    /** \todo handle short reads, integer overflow */
    return fread(buf, 1, n, f);
}


static int
op_file_rename(archive_t *a, int idx, const char *name)
{
    struct stat st;
    
    char *current_name = get_full_name(a, idx);
    

    if (stat(current_name, &st) < 0) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot rename `%s': %s", file_name(archive_file(a, idx), strerror(errno)));
	free(current_name);
	return -1;
    }
    
    car *final_name = make_full_name(a, name);
    ud_t *ud = archive_user_data(a);
    
    change_set(array_get(ud->change, idx), CHANGE_RENAME, current_name, final_name);
    
    return 0;
}


static const char *
op_file_strerror(void *f)
{
    return strerror(errno);
}


static int
op_read_infos(archive_t *a)
{
    struct zip *za;
    struct zip_stat zsb;
    file_t *r;
    int i;
    
    if (ensure_zip(a) < 0)
	return -1;
    
    za = (struct zip *)archive_user_data(a);
    
    seterrinfo(NULL, archive_name(a));
    
    for (i=0; i<zip_get_num_files(za); i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s", i, zip_strerror(za));
	    continue;
	}
        
	r = (file_t *)array_grow(archive_files(a), file_init);
	file_size(r) = zsb.size;
	file_name(r) = xstrdup(zsb.name);
	file_status(r) = STATUS_OK;
        
	hashes_init(file_hashes(r));
	file_hashes(r)->types = HASHES_TYPE_CRC;
	file_hashes(r)->crc = zsb.crc;
        
	if (detector)
	    match_detector(za, i, r);
        
	if (a->flags & ARCHIVE_FL_CHECK_INTEGRITY)
	    archive_file_compute_hashes(a, i, romhashtypes);
    }
    
    return 0;
    
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
