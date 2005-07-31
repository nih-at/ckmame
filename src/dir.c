/*
  $NiH$

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



int
dir_close(dir_t *dir)
{
    int ret;

    ret = closedir((DIR *)dir->dir);
    
    free(dir->name);
    free(dir);

    return ret;
}



int
dir_next(dir_t *dir, char *name, int len)
{
    struct dirent *de;
    int l;
    
    for (;;) {
	if ((de=readdir((DIR *)dir->dir)) == NULL)
	    return DIR_EOD;
	
	l = NAMLEN(de);

	if ((l == 1 && strncmp(de->d_name, ".", 1) == 0)
	    || (l == 2 && strncmp(de->d_name, "..", 2) == 0))
	    continue;
	    
	if (dir->len + l + 2 > len) {
	    errno = ENAMETOOLONG;
	    return DIR_ERROR;
	}
	
	sprintf(name, "%s/%*s", dir->name, l, de->d_name);
	return DIR_OK;
    }
}



dir_t *
dir_open(const char *name)
{
    DIR *d;
    dir_t *dir;

    if ((d=opendir(name)) == NULL)
	return NULL;

    dir = xmalloc(sizeof(*dir));
    dir->dir = d;
    dir->name = xstrdup(name);
    dir->len = strlen(name);

    return dir;
}
