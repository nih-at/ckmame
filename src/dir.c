/*
  dir.h -- reading a directory
  Copyright (C) 2005-2017 Dieter Baron and Thomas Klausner

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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* copied from autoconf manual (AC_HEADER_DIRENT) */

#include "config.h"

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include "dir.h"
#include "xmalloc.h"

struct dir_one {
    parray_t *entries;
    size_t index;
    char *name;
    size_t len;
};

typedef struct dir_one dir_one_t;

static void dir_one_free(dir_one_t *);
static dir_one_t *dir_one_new(const char *);


int
dir_close(dir_t *dir) {
    dir_one_t *d;
    int ret;

    ret = 0;
    while ((d = parray_pop(dir->stack)) != NULL)
	dir_one_free(d);

    parray_free(dir->stack, (void (*)(void *))dir_one_free);
    free(dir);

    return ret;
}


dir_status_t
dir_next(dir_t *dir, char *name, int len) {
    char *entry;
    dir_one_t *d;
    struct stat st;

    d = parray_get_last(dir->stack);

    for (;;) {
	if (d->index == parray_length(d->entries)) {
	    dir_one_free(d);
	    parray_pop(dir->stack);
	    if ((d = parray_get_last(dir->stack)) == NULL)
		return DIR_EOD;
	    continue;
	}

	entry = parray_get(d->entries, d->index++);

	if (d->len + strlen(entry) + 2 > len) {
	    errno = ENAMETOOLONG;
	    return DIR_ERROR;
	}

	sprintf(name, "%s/%s", d->name, entry);

	if (dir->flags & DIR_RECURSE) {
	    if (stat(name, &st) < 0)
		return DIR_ERROR;

	    if ((st.st_mode & S_IFMT) == S_IFDIR) {
		if ((d = dir_one_new(name)) == NULL)
		    return DIR_ERROR;

		parray_push(dir->stack, d);
		if (dir->flags & DIR_RETURN_DIRECTORIES)
		    return DIR_DIRECTORY;
		continue;
	    }
	}
	return DIR_OK;
    }
}


dir_t *
dir_open(const char *name, int flags) {
    dir_one_t *d;
    dir_t *dir;

    if ((d = dir_one_new(name)) == NULL)
	return NULL;

    dir = xmalloc(sizeof(*dir));
    dir->stack = parray_new();
    parray_push(dir->stack, d);
    dir->flags = flags;

    return dir;
}


static void
dir_one_free(dir_one_t *d) {
    if (d == NULL) {
	return;
    }

    parray_free(d->entries, free);
    free(d->name);
    free(d);
}


static dir_one_t *
dir_one_new(const char *name) {
    DIR *dir;
    dir_one_t *d;
    parray_t *entries;
    size_t l;
    char *entry;
    struct dirent *de;

    if ((dir = opendir(name)) == NULL) {
	return NULL;
    }

    entries = parray_new();

    while ((de = readdir(dir)) != NULL) {
	l = NAMLEN(de);

	if ((l == 1 && strncmp(de->d_name, ".", 1) == 0) || (l == 2 && strncmp(de->d_name, "..", 2) == 0)) {
	    continue;
	}
	if (l > INT_MAX) {
	    closedir(dir);
	    errno = ENAMETOOLONG;
	    parray_free(entries, free);
	    return NULL;
	}
	if (asprintf(&entry, "%.*s", (int)l, de->d_name) < 0) {
	    parray_free(entries, free);
	    closedir(dir);
	    return NULL;
	}

	parray_push(entries, entry);
    }

    if (closedir(dir) < 0) {
	parray_free(entries, free);
	return NULL;
    }

    parray_sort(entries, strcmp);

    d = xmalloc(sizeof(*d));
    d->name = xstrdup(name);
    d->len = strlen(name);
    d->entries = entries;
    d->index = 0;

    return d;
}
