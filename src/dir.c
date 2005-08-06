/*
  $NiH: dir.c,v 1.1.2.1 2005/07/31 11:36:34 dillo Exp $

  dir.h -- reading a directory
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* copied from autoconf manual (AC_HEADER_DIRENT) */

#include "config.h"

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include "dir.h"
#include "xmalloc.h"

struct dir_one {
    DIR *dir;
    char *name;
    int len;
};

typedef struct dir_one dir_one_t;

static int dir_one_free(dir_one_t *);
static dir_one_t *dir_one_new(const char *);



int
dir_close(dir_t *dir)
{
    dir_one_t *d;
    int ret;

    ret = 0;
    while ((d=parray_pop(dir->stack)) != NULL)
	ret |= dir_one_free(d);
    
    parray_free(dir->stack, (void (*)())dir_one_free);
    free(dir);

    return ret;
}



int
dir_next(dir_t *dir, char *name, int len)
{
    struct dirent *de;
    dir_one_t *d;
    int l;
    struct stat st;

    d = parray_get_last(dir->stack);
    for (;;) {
	if ((de=readdir(d->dir)) == NULL) {
	    dir_one_free(d);
	    parray_pop(dir->stack);
	    if ((d=parray_get_last(dir->stack)) == NULL)
		return DIR_EOD;
	    continue;
	}
	
	l = NAMLEN(de);

	if ((l == 1 && strncmp(de->d_name, ".", 1) == 0)
	    || (l == 2 && strncmp(de->d_name, "..", 2) == 0))
	    continue;
	    
	if (d->len + l + 2 > len) {
	    errno = ENAMETOOLONG;
	    return DIR_ERROR;
	}
	
	sprintf(name, "%s/%*s", d->name, l, de->d_name);

	if (dir->flags & DIR_RECURSE) {

	    if (stat(name, &st) < 0)
		return DIR_ERROR;

	    if ((st.st_mode & S_IFMT) == S_IFDIR) {
		if ((d=dir_one_new(name)) == NULL)
		    return DIR_ERROR;

		parray_push(dir->stack, d);
		continue;
	    }
	}
	return DIR_OK;
    }
}



dir_t *
dir_open(const char *name, int flags)
{
    dir_one_t *d;
    dir_t *dir;

    if ((d=dir_one_new(name)) == NULL)
	return NULL;

    dir = xmalloc(sizeof(*dir));
    dir->stack = parray_new();
    parray_push(dir->stack, d);
    dir->flags = flags;

    return dir;
}



static int
dir_one_free(dir_one_t *d)
{
    int ret;

    if (d == NULL)
	return 0;
    
    ret = closedir(d->dir);
    free(d->name);
    free(d);

    return ret;
}



static dir_one_t *
dir_one_new(const char *name)
{
    DIR *dir;
    dir_one_t *d;

    if ((dir=opendir(name)) == NULL)
	return NULL;

    d = xmalloc(sizeof(*d));
    d->dir = dir;
    d->name = xstrdup(name);
    d->len = strlen(name);

    return d;
}
