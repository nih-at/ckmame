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

#include <sys/stat.h>
#include <errno.h>
#include <fts.h>
#include <unistd.h>

#include "funcs.h"

#include "archive.h"
#include "dbh_dir.h"
#include "error.h"
#include "globals.h"
#include "util.h"
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

static int cmp_file_by_name(file_t *f, const char *);
static int ensure_archive_dir(archive_t *);
static char *get_full_name(archive_t *, int);
static char *make_full_name(archive_t *, const char *);
static char *make_tmp_name(archive_t *, const char *);
static int my_fts_sort_names(const FTSENT **, const FTSENT **);

static int op_check(archive_t *);
static int op_close(archive_t *);
static int op_commit(archive_t *);
static int op_file_add_empty(archive_t *, const char *);
static int op_file_copy(archive_t *, int, archive_t *, int, const char *, off_t, off_t);
static int op_file_delete(archive_t *, int);
static void *op_file_open(archive_t *, int);
static int64_t op_file_read(void *, void *, uint64_t);
static int op_file_rename(archive_t *, int, const char *);
static char *op_file_rename_unique(archive_t *, int);
static const char *op_file_strerror(void *);
static int op_read_infos(archive_t *);
static int op_rollback(archive_t *);

struct archive_ops ops_dir = {
    op_check,
    op_close,
    op_commit,
    op_file_add_empty,
    (void (*)(void *))fclose,
    op_file_copy,
    op_file_delete,
    op_file_open,
    op_file_read,
    op_file_rename,
    op_file_rename_unique,
    op_file_strerror,
    op_read_infos,
    op_rollback
};


int
archive_init_dir(archive_t *a)
{
    a->ops = &ops_dir;
    
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
	change_rollback(ch);
    }

    if (current_name) {
	if (stat(current_name, &st) < 0)
	    return -1;
    }

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
cmp_file_by_name(file_t *f, const char *name)
{
    return strcmp(file_name(f), name);
}


static int
ensure_archive_dir(archive_t *a)
{
    struct stat st;

    if (stat(archive_name(a), &st) < 0) {
	if (errno == ENOENT && (archive_flags(a) & ARCHIVE_FL_CREATE))
	    return mkdir(archive_name(a), 0777);
	return -1;
    }
    return 0;
}


static char *
get_full_name(archive_t *a, int idx)
{
    return make_full_name(a, file_name(archive_file(a, idx)));
}


static char *
make_full_name(archive_t *a, const char *name)
{
    size_t len;
    char *s;

    len = strlen(archive_name(a)) + strlen(name) + 2;

    if ((s=malloc(len)) == NULL)
	return NULL;

    snprintf(s, len, "%s/%s", archive_name(a), name);

    return s;
}

static char *
make_tmp_name(archive_t *a, const char *name)
{
    size_t len;
    char *s;

    if (a == NULL || name == NULL) {
	errno = EINVAL;
	return NULL;
    }

    len = strlen(archive_name(a)) + strlen(name) + 17;

    if ((s=malloc(len)) == NULL)
	return NULL;

    snprintf(s, len, "%s/.ckmame-%s.XXXXXX", archive_name(a), name);
    
    if (mktemp(s) == NULL) {
	free(s);
	return NULL;
    }
    return s;
}


static int
my_fts_sort_names(const FTSENT **a, const FTSENT **b)
{
    /** \todo implement */
    return 0;
}


static int
op_check(archive_t *a)
{
    if (!is_writable_directory(archive_name(a))) {
	if (errno == ENOENT && (a->flags & ARCHIVE_FL_CREATE)) {
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
op_close(archive_t *a)
{
    ud_t *ud = archive_user_data(a);
    
    /** \todo update database in commit */
#if 0
    if (archive_where(a) == FILE_ROMSET) {
        if (ud->id > 0)
            dbh_dir_delete(ud->id);
        dbh_dir_write(ud->id, archive_name(a), archive_files(a));
    }
#endif
    
    return op_commit(a);
}

static int
op_commit(archive_t *a)
{
    int idx;
    ud_t *ud = archive_user_data(a);
    change_t *ch;

    /** \todo update db */
    /** \todo handle errors */

    for (idx=0; idx<archive_num_files(a); idx++) {
	ch = array_get(ud->change, idx);
	switch (ch->type) {
	case CHANGE_NONE:
	    break;

	case CHANGE_DELETE:
	    unlink(ch->current_name);
	    break;

	case CHANGE_ADD:
	case CHANGE_REPLACE:
	case CHANGE_RENAME:
	    rename(ch->current_name, ch->final_name);
	    break;
	}

	change_fini(ch);
	change_init(ch);
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
    if (ensure_archive_dir(da) < 0)
	return -1;

    char *tmpname = make_tmp_name(da, dname);

    if (tmpname == NULL)
	return -1;

    char *srcname = NULL;
    
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

    ud_t *ud = archive_user_data(da);
    change_t *ch;

    if (didx >= 0)
	ch = array_get(ud->change, didx);
    else {
	if ((ch = array_grow(ud->change, change_init)) == NULL) {
	    /** \todo error message */
	    unlink(tmpname);
	    free(tmpname);
	    return -1;
	}
    }

    int ret;
    if (didx >= 0)
        ret = change_set(ch, CHANGE_REPLACE, tmpname, make_full_name(da, dname));
    else
        ret = change_set(ch, CHANGE_ADD, tmpname, make_full_name(da, dname));
    
    if (ret < 0) {
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
	myerror(ERRZIP, "cannot delete `%s': %s", file_name(archive_file(a, idx)), strerror(errno));
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
	myerror(ERRZIP, "cannot open `%s': %s", file_name(archive_file(a, idx)), strerror(errno));
	free(full_name);
	return NULL;
    }

    free(full_name);

    return f;
}


static int64_t
op_file_read(void *f, void *buf, uint64_t n)
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
	myerror(ERRZIP, "cannot rename `%s': %s", file_name(archive_file(a, idx)), strerror(errno));
	free(current_name);
	return -1;
    }
    
    char *final_name = make_full_name(a, name);
    ud_t *ud = archive_user_data(a);
    
    change_set(array_get(ud->change, idx), CHANGE_RENAME, current_name, final_name);
    
    return 0;
}


static char *
op_file_rename_unique(archive_t *a, int idx)
{
    /*** \todo implement */
    return NULL;
}


static const char *
op_file_strerror(void *f)
{
    return strerror(errno);
}


static int
op_read_infos(archive_t *a)
{
    int id = 0;
    array_t *files = NULL;
    FTS *ftsp;
    FTSENT *ent;
    file_t *fdir, *fdb;
    char * const names[2] = { archive_name(a), NULL };
    
    if (archive_where(a) == FILE_ROMSET) {
        if (ensure_romset_dir_db() < 0)
            return -1;
        
        files = array_new(sizeof(file_t));

        if ((id = dbh_dir_read(mybasename(archive_name(a)), files)) < 0) {
            /** \todo error message */
        }
    }
    
    if ((ftsp = fts_open(names, FTS_LOGICAL|FTS_NOCHDIR, my_fts_sort_names)) == NULL) {
        /** \todo error message */
        array_free(files, file_finalize);
        return -1;
    }
    
    while ((ent=fts_read(ftsp)) != NULL) {
        switch (ent->fts_info) {
            case FTS_D:
            case FTS_DP:
                break;
                
            case FTS_DC:
            case FTS_DNR:
            case FTS_ERR:
            case FTS_NS:
            case FTS_SLNONE:
                /** \todo error message */
                break;
                
            case FTS_F:
                fdir = array_grow(archive_files(a), file_init);
                
                file_name(fdir) = xstrdup(ent->fts_name);
                file_size(fdir) = ent->fts_statp->st_size;
                file_mtime(fdir) = ent->fts_statp->st_mtime;
                
                if (files) {
                    fdb = array_get(files, array_index(files, file_name(fdir), cmp_file_by_name));
                
                    if (fdb) {
                        if (file_mtime(fdb) == file_mtime(fdir) && file_size(fdb) == file_size(fdir)) {
                            memcpy(fdir->sh, fdb->sh, sizeof(fdir->sh));
                        }
                    }
                }
                break;

            case FTS_DEFAULT:
            case FTS_NSOK:
            case FTS_SL:
            case FTS_DOT:
                /** \todo shouldn't happen */
                break;
        }
    }
    
    if (errno != 0) {
        fts_close(ftsp);
        /** \todo handle fts error */
    }
    
    fts_close(ftsp);
    
    array_free(files, file_finalize);

    /** \todo set up ud */
    
    return 0;
    
}


static int
op_rollback(archive_t *a)
{
    int idx;
    ud_t *ud = archive_user_data(a);
    change_t *ch;

    for (idx=0; idx<archive_num_files(a); idx++) {
	ch = array_get(ud->change, idx);
	switch (ch->type) {
	case CHANGE_NONE:
	case CHANGE_DELETE:
	case CHANGE_RENAME:
	    break;

	case CHANGE_ADD:
	case CHANGE_REPLACE:
	    unlink(ch->current_name);
	    /** \todo warning if failed */
	    break;
	}

	change_fini(ch);
	change_init(ch);
    }

    return 0;
}


