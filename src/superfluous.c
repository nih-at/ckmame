/*
  $NiH: superfluous.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

  superfluous.c -- check for unknown file in rom directories
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "types.h"
#include "dbh.h"
#include "util.h"
#include "funcs.h"
#include "error.h"
#include "xmalloc.h"

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



static int
docmp(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}



int
handle_extra_files(DB *db, const char *dbname, int sample)
{
    DIR *dir;
    struct dirent *de;
    char b[8192], *p;
    parray_t *list, *lists, *listx, *lst, *found;
    int i, j, l, first;

    first = 1;

    if ((list=r_list(db, DDB_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "list of games not found in database `%s'", dbname);
	exit(1);
    }
    if ((lists=r_list(db, DDB_KEY_LIST_SAMPLE)) == NULL) {
	myerror(ERRDEF, "list of samples not found in database `%s'", dbname);
	exit(1);
    }
    if ((listx=r_list(db, DDB_KEY_LIST_DISK)) == NULL) {
	myerror(ERRDEF, "list of extra files not found in database `%s'",
		dbname);
	exit(1);
    }

    /* XXX: why this? */
    if (parray_length(list) < 1 || (sample && parray_length(lists) < 1))
	return 0;

    init_rompath();

    for (i=0; rompath[i]; i++) {
	sprintf(b, "%s/%s", rompath[i], sample ? "samples" : "roms");
	if ((dir=opendir(b)) == NULL) {
	    /* XXX: error */
	    continue;
	}

	found = parray_new();

	while ((de=readdir(dir))) {
	    l = (NAMLEN(de) < sizeof(b)-1 ? NAMLEN(de) : sizeof(b)-1);
	    strncpy(b, de->d_name, l);
	    b[l] = '\0';

	    if (strcmp(b, ".") == 0 || strcmp(b, "..") == 0)
		continue;

	    if (l > 4 && strcmp(b+l-4, ".zip") == 0) {
		b[l-4] = '\0';
		lst = sample ? lists : list;
	    }
	    else if (l > 4 && strcmp(b+l-4, ".chd") == 0) {
		b[l-4] = '\0';
		lst = listx;
	    }
	    else
		lst = listx;

	    p = b;
	    if (parray_bsearch(lst, &p, (cmpfunc)strpcasecmp) == NULL) {
		p = xmalloc(l+1);
		strncpy(p, de->d_name, l);
		p[l] = '\0';
		parray_push(found, p);
	    }
	}
	closedir(dir);

	if (parray_length(found) > 0) {
	    if (first) {
		printf("Extra files found:\n");
		first = 0;
	    }

	    parray_sort(found, docmp);

	    for (j=0; j<parray_length(found); j++)
		printf("%s/%s/%s\n", rompath[i],
		       sample ? "samples" : "roms",
		       (char *)parray_get(found, j));

	    parray_free(found, free);
	}
    }

    return 0;
}
