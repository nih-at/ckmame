/*
 archive_dir.c -- implementation of archive from directory
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

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

#include "funcs.h"

#include "archive.h"
#include "dir.h"
#include "error.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"

#define BUFSIZE 8192

#ifndef SIZE_T_MAX
#define SIZE_T_MAX (~(size_t)0)
#endif

typedef enum {
    CHANGE_NONE,
    CHANGE_ADD,
    CHANGE_REPLACE,
    CHANGE_DELETE,
    CHANGE_RENAME
} change_type_t;

typedef struct {
    char *name;
    char *data;
} fileinfo_t;

typedef struct {
    fileinfo_t original;
    fileinfo_t final;
    time_t mtime;
} change_t;

typedef struct {
    array_t *change;
} ud_t;

static void change_init(change_t *);

static int ensure_archive_dir(archive_t *);
static char *get_full_name(archive_t *, int);
static char *make_full_name(archive_t *, const char *);
static char *make_tmp_name(archive_t *, const char *);

static int op_check(archive_t *);
static int op_close(archive_t *);
static int op_commit(archive_t *);
static void op_commit_cleanup(archive_t *);
static int op_file_add_empty(archive_t *, const char *);
static int op_file_copy(archive_t *, int, archive_t *, int, const char *, off_t, off_t);
static int op_file_delete(archive_t *, int);
static void *op_file_open(archive_t *, int);
static int64_t op_file_read(void *, void *, uint64_t);
static int op_file_rename(archive_t *, int, const char *);
static const char *op_file_strerror(void *);
static bool op_get_last_update(archive_t *a, time_t *mtime, off_t *size);
static bool op_read_infos(archive_t *);
static int op_rollback(archive_t *);

struct archive_ops ops_dir = {
    op_check,
    op_close,
    op_commit,
    op_commit_cleanup,
    op_file_add_empty,
    (void (*)(void *))fclose,
    op_file_copy,
    op_file_delete,
    op_file_open,
    op_file_read,
    op_file_rename,
    op_file_strerror,
    op_get_last_update,
    op_read_infos,
    op_rollback
};

#define archive_file_change(a, idx)	(array_get(((ud_t *)archive_user_data(a))->change, (idx)))


int
archive_init_dir(archive_t *a)
{
    ud_t *ud;

    a->ops = &ops_dir;

    ud = xmalloc(sizeof(*ud));

    ud->change = array_new(sizeof(change_t));

    archive_user_data(a) = ud;

    return 0;
}


static void
fileinfo_init(fileinfo_t *f)
{
    f->name = f->data = NULL;
}

static void
fileinfo_fini(fileinfo_t *f)
{
    free(f->name);
    f->name = NULL;
    free(f->data);
    f->data = NULL;
}

static void
change_fini(change_t *ch)
{
    fileinfo_fini(&ch->original);
    fileinfo_fini(&ch->final);
}

static void
change_init(change_t *ch)
{
    fileinfo_init(&ch->original);
    fileinfo_init(&ch->final);
    ch->mtime = 0;
}

static bool
change_is_unchanged(const change_t *ch)
{
    return ch->original.name == NULL && ch->final.name == NULL;
}

static bool
change_is_added(const change_t *ch)
{
    return ch->original.name == NULL && ch->final.name != NULL;
}

static bool
change_is_deleted(const change_t *ch)
{
    return ch->original.name != NULL && ch->final.name == NULL;
}

static bool
change_is_rename(const change_t *ch)
{
    if (ch->original.data == NULL || ch->final.data == NULL)
	return false;
    return strcmp(ch->original.data, ch->final.data) == 0;
}

static bool
change_has_new_data(const change_t *ch)
{
    if (ch->final.name == NULL)
	return false;

    return (ch->original.name == NULL || strcmp(ch->original.data, ch->final.data) != 0);
}

static int
fileinfo_apply(const fileinfo_t *f)
{
    if (f->name == NULL || strcmp(f->name, f->data) == 0)
	return 0;

    if (rename(f->data, f->name) < 0) {
	myerror(ERRZIP, "apply: cannot rename '%s' to '%s': %s", f->data, f->name, strerror(errno));
	return -1;
    }
    return 0;   
}

static int
fileinfo_discard(archive_t *a, const fileinfo_t *f)
{
    if (f->name == NULL || strcmp(f->name, f->data) == 0)
	return 0;

    /* TODO: stop before removing archive_name itself, e.g. extra_dir */
    if (remove_file_and_containing_empty_dirs(f->data, archive_name(a)) < 0) {
	myerror(ERRZIP, "cannot delete '%s': %s", f->data, strerror(errno));
	return -1;
    }
    return 0;   
}


/* returns: -1 on error, 1 if file was moved, 0 if nothing needed to be done */
static int
move_original_file_out_of_the_way(archive_t *a, int idx)
{
    change_t *ch = archive_file_change(a, idx);
    char *name = file_name(archive_file(a, idx));

    if (change_is_added(ch) || ch->original.name)
	return 0;

    char *full_name = get_full_name(a, idx);
    char *tmp = make_tmp_name(a, name);

    if (rename(full_name, tmp) < 0) {
	myerror(ERRZIP, "move: cannot rename '%s' to '%s': %s", name, tmp, strerror(errno));
	free(full_name);
	free(tmp);
	return -1;
    }

    ch->original.name = full_name;
    ch->original.data = tmp;

    return 1;

}


static int
change_apply(archive_t *a, int idx, change_t *ch)
{
    if (ch->final.name != NULL) {
	if (ensure_dir(ch->final.name, 1) < 0) {
	    myerror(ERRZIP, "destination directory cannot be created: %s", strerror(errno));
	    return -1;
	}

	if (fileinfo_apply(&ch->final) < 0)
	    return -1;
        
        struct stat st;
        if (stat(ch->final.name, &st) < 0) {
            myerror(ERRZIP, "can't stat created file '%s': %s", ch->final.name, strerror(errno));
            return -1;
        }
        file_mtime(archive_file(a, idx)) = st.st_mtime;
    }

    if (!change_is_rename(ch))
	if (fileinfo_discard(a, &ch->original) < 0)
	    return -1;

    change_fini(ch);
    change_init(ch);

    return 0;
}

static void
change_rollback(archive_t *a, change_t *ch)
{
    fileinfo_apply(&ch->original);

    if (!change_is_rename(ch))
	fileinfo_discard(a, &ch->final);

    change_fini(ch);
    change_init(ch);
}


static int
ensure_archive_dir(archive_t *a)
{
    return ensure_dir(archive_name(a), 0);
}


static char *
get_full_name(archive_t *a, int idx)
{
    change_t *ch = archive_file_change(a, idx);

    if (ch->final.data)
	return strdup(ch->final.data);

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
    size_t i, len;
    char *s;

    if (a == NULL || name == NULL) {
	errno = EINVAL;
	return NULL;
    }

    len = strlen(archive_name(a)) + strlen(name) + 17;

    if ((s=malloc(len)) == NULL)
	return NULL;

    snprintf(s, len, "%s/.ckmame-%s.XXXXXX", archive_name(a), name);
    for (i = strlen(archive_name(a))+1; i < len; i++) {
	if (s[i] == '/')
	    s[i] = '_';
    }
    
    if (mktemp(s) == NULL) {
	free(s);
	return NULL;
    }
    return s;
}


static int
op_check(archive_t *a)
{
    return 0;
}


static int
op_close(archive_t *a)
{
    ud_t *ud = archive_user_data(a);

    if (ud == NULL) {
	/* error during initialization, do nothing */
	return 0;
    }

#if 0
    if (ud->dbh_changed) {
	/* update db */
	if (!ud->dbh)
	    ud->dbh = dbh_cache_get_db_for_archive(archive_name(a));
	if (ud->dbh) {
	    if (ud->id > 0)
		dbh_cache_delete(ud->dbh, ud->id);
	    if (archive_num_files(a) > 0) {
		ud->id = dbh_cache_write(ud->dbh, ud->id, mybasename(archive_name(a)), 0, 0, archive_files(a));
		if (ud->id < 0) {
		    seterrdb(ud->dbh);
		    myerror(ERRDB, "%s: error writing to " DBH_CACHE_DB_NAME, archive_name(a));
		    ud->id = 0;
		}
	    }
	    else
		ud->id = 0;
	}
	else
	    ud->id = 0;
	ud->dbh_changed = false;
    }
#endif

    return 0;
}


static int
op_commit(archive_t *a)
{
    int idx;
    ud_t *ud = archive_user_data(a);
    change_t *ch;

    if (!archive_is_modified(a))
	return 0;

    bool is_empty = true;
    for (idx = 0; idx < archive_num_files(a); idx++) {
	if (file_where(archive_file(a, idx)) != FILE_DELETED) {
	    is_empty = false;
	    break;
	}
    }

    int ret = 0;
    for (idx=0; idx<archive_num_files(a); idx++) {
	if (idx >= array_length(ud->change))
	    break;
	ch = array_get(ud->change, idx);

	if (change_apply(a, idx, ch) < 0)
	    ret = -1;
    }

    if (is_empty && archive_is_writable(a) && !(archive_flags(a) & ARCHIVE_FL_KEEP_EMPTY)) {
	if (rmdir(archive_name(a)) < 0 && errno != ENOENT) {
	    myerror(ERRZIP, "cannot remove empty archive '%s': %s", archive_name(a), strerror(errno));
	    ret = -1;
	}
    }

#if 0
    if (archive_is_modified(a)) 
	ud->dbh_changed = true;
#endif

    return ret;
}


static void
op_commit_cleanup(archive_t *a)
{
    ud_t *ud = archive_user_data(a);

    array_truncate(ud->change, archive_num_files(a), change_fini);
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
    /* archive layer already grew archive files if didx < 0 */
    file_t *f = archive_file(da, (didx >= 0 ? didx : archive_num_files(da)-1));
    
    if (tmpname == NULL)
	return -1;

    char *srcname = NULL;
    
    if (sa) {
	if ((srcname = get_full_name(sa, sidx)) == NULL) {
	    free(tmpname);
	    return -1;
	}

	if (start == 0 && (len == -1 || (uint64_t)len == file_size(archive_file(sa, sidx)))) {
	    if (link_or_copy(srcname, tmpname) < 0) {
		free(tmpname);
		free(srcname);
		return -1;
	    }
	}
	else {
	    if (copy_file(srcname, tmpname, start, len, file_hashes(f)) < 0) {
		myerror(ERRZIP, "cannot copy '%s' to '%s': %s", srcname, tmpname, strerror(errno));
		free(tmpname);
		free(srcname);
		return -1;
	    }
	}
	free(srcname);
    }
    else {
	FILE *fout;

	if ((fout = fopen(tmpname, "w")) == NULL) {
	    myerror(ERRZIP, "cannot open '%s': %s", tmpname, strerror(errno));
	    free(tmpname);
	    return -1;
	}
	fclose(fout);
    }

    ud_t *ud = archive_user_data(da);
    change_t *ch;

    bool err = false;
    if (didx >= 0 && didx < array_length(ud->change)) {
	ch = array_get(ud->change, didx);

	if (!change_is_added(ch)) {
	    if (strcmp(dname, file_name(archive_file(da, didx))) != 0) {
		if (move_original_file_out_of_the_way(da, didx) < 0)
		    err = true;
	    }
	    else {
		if (change_is_unchanged(ch)) {
		    ch->original.name = make_full_name(da, dname);
		    ch->original.data = xstrdup(ch->original.name);
		}
	    }
	}
    }
    else {
	if ((ch = array_grow(ud->change, change_init)) == NULL) {
	    myerror(ERRDEF, "cannot grow array: %s", strerror(errno));
	    err = true;
	}
    }

    if (err) {
	unlink(tmpname);
	free(tmpname);
	return -1;
    }

    char *full_name = make_full_name(da, dname);

    if (change_has_new_data(ch))
	fileinfo_discard(da, &ch->final);
    fileinfo_fini(&ch->final);
    ch->final.name = full_name;
    ch->final.data = tmpname;

    return 0;
}


static int
op_file_delete(archive_t *a, int idx)
{
    change_t *ch = archive_file_change(a, idx);

    if (move_original_file_out_of_the_way(a, idx) < 0)
	return -1;

    int ret = 0;

    if (change_has_new_data(ch)) {
	if (fileinfo_discard(a, &ch->final) < 0)
	    ret = -1;
    }
    fileinfo_fini(&ch->final);

    return ret;
}



static void *
op_file_open(archive_t *a, int idx)
{
    FILE *f;

    char *full_name = get_full_name(a, idx);

    if ((f=fopen(full_name, "r")) == NULL) {
	seterrinfo(NULL, archive_name(a));
	myerror(ERRZIP, "cannot open '%s': %s", file_name(archive_file(a, idx)), strerror(errno));
	free(full_name);
	return NULL;
    }

    free(full_name);

    return f;
}


static int64_t
op_file_read(void *f, void *buf, uint64_t n)
{
    /* TODO handle short reads */
    if (n > SIZE_T_MAX) {
	errno = EINVAL;
	return -1;
    }
    return fread(buf, 1, n, f);
}


static bool
file_will_exist_after_commit(archive_t *a, const char *full_name)
{
    struct stat st;
    int i;
    
    for (i=0; i<archive_num_files(a); i++) {
	change_t *ch = archive_file_change(a, i);

	if (ch->final.name && strcmp(ch->final.name, full_name) == 0)
	    return true;
    }

    if (stat(full_name, &st) == 0)
	return true;

    return false;
}


static int
op_file_rename(archive_t *a, int idx, const char *name)
{
    change_t *ch = archive_file_change(a, idx);

    if (change_is_deleted(ch)) {
	myerror(ERRZIP, "cannot rename deleted file '%s'", file_name(archive_file(a, idx)));
	return -1;
    }
   
    char *final_name = make_full_name(a, name);

    if (file_will_exist_after_commit(a, final_name)) {
	free(final_name);
	errno = EEXIST;
	myerror(ERRZIP, "cannot rename '%s' to '%s': %s", file_name(archive_file(a, idx)), name, strerror(errno));
	return -1;
    }

    switch (move_original_file_out_of_the_way(a, idx)) {
    case -1:
	free(final_name);
	return -1;

    case 1:
	ch->final.data = xstrdup(ch->original.data);
	break;

    default:
	break;
    }

    free(ch->final.name);
    ch->final.name = final_name;

    return 0;
}


static const char *
op_file_strerror(void *f)
{
    return strerror(errno);
}


static bool
op_get_last_update(archive_t *a, time_t *mtime, off_t *size)
{
    time(mtime);
    *size = 0;

    return true;
}


static bool
op_read_infos(archive_t *a)
{
    ud_t *ud = archive_user_data(a);
    dir_t *dir;
    char namebuf[8192];
    dir_status_t status;

    if ((dir=dir_open(archive_name(a), DIR_RECURSE)) == NULL) {
	return false;
    }

    while ((status=dir_next(dir, namebuf, sizeof(namebuf))) == DIR_OK) {
	struct stat sb;
	if (strcmp(namebuf, archive_name(a)) == 0)
	    continue;

	if (stat(namebuf, &sb) < 0) {
	    dir_close(dir);
	    return false;
	}

	if (S_ISREG(sb.st_mode)) {
	    file_t *f;
	    
	    f = array_grow(archive_files(a), file_init);
	    array_grow(ud->change, change_init);
	    
	    file_name(f) = xstrdup(namebuf+strlen(archive_name(a))+1);
	    file_size(f) = sb.st_size;
	    file_mtime(f) = sb.st_mtime;
	}
    }

    if (status != DIR_EOD) {
	myerror(ERRDEF, "error reading directory: %s", strerror(errno));
	dir_close(dir);
	return false;
    }
    if (dir_close(dir) < 0) {
	myerror(ERRDEF, "cannot close directory '%s': %s", archive_name(a), strerror(errno));
	return false;
    }

    return true;
}


static int
op_rollback(archive_t *a)
{
    int idx;
    ud_t *ud = archive_user_data(a);
    change_t *ch;

    for (idx=0; idx<archive_num_files(a); idx++) {
	if (idx >= array_length(ud->change))
	    break;
	ch = array_get(ud->change, idx);
	change_rollback(a, ch);
    }

    return 0;
}
