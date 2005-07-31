/*
  $NiH: superfluous.c,v 1.3.2.2 2005/07/30 23:24:51 wiz Exp $

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

#include "dbh.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"



parray_t *
find_extra_files(const char *dbname)
{
    DIR *dir;
    struct dirent *de;
    char b[8192], dirname[8192], *p;
    parray_t *listf, *listd, *lst, *found;
    int i, l, first;

    first = 1;

    if (file_type == TYPE_ROM) {
	if ((listf=r_list(db, DDB_KEY_LIST_GAME)) == NULL) {
	    myerror(ERRDEF, "list of games not found in database `%s'",
		    dbname);
	    exit(1);
	}
	if ((listd=r_list(db, DDB_KEY_LIST_DISK)) == NULL) {
	    myerror(ERRDEF, "list of extra files not found in database `%s'",
		    dbname);
	    exit(1);
	}
    }
    else {
	if ((listf=r_list(db, DDB_KEY_LIST_SAMPLE)) == NULL) {
	    myerror(ERRDEF, "list of samples not found in database `%s'",
		    dbname);
	    exit(1);
	}
	listd = NULL;
    }

    init_rompath();

    found = parray_new();

    for (i=0; rompath[i]; i++) {
	if (snprintf(dirname, sizeof(dirname), "%s/%s", rompath[i],
		     file_type == TYPE_ROM ? "roms" : "samples")
	    > sizeof(dirname)-1) {
	    myerror(ERRDEF, "ROMPATH entry too long, skipping: `%s'",
		    rompath[i]);
	    continue;
	}
	if ((dir=opendir(dirname)) == NULL) {
	    myerror(ERRSTR, "can't open ROMPATH directory `%s'", dirname);
	    continue;
	}

	while ((de=readdir(dir))) {
	    l = (NAMLEN(de) < sizeof(b)-1 ? NAMLEN(de) : sizeof(b)-1);
	    strncpy(b, de->d_name, l);
	    b[l] = '\0';

	    if (strcmp(b, ".") == 0 || strcmp(b, "..") == 0)
		continue;

	    if (l > 4 && strcmp(b+l-4, ".zip") == 0) {
		b[l-4] = '\0';
		lst = listf;
	    }
	    else if (l > 4 && strcmp(b+l-4, ".chd") == 0) {
		b[l-4] = '\0';
		lst = listd;
	    }
	    else
		lst = listd;

	    if (lst == NULL || parray_index_sorted(lst, b, strcasecmp) == -1) {
		p = xmalloc(strlen(dirname)+1+l+1);
		snprintf(p, strlen(dirname)+1+l+1, "%s/%s", dirname,
			 de->d_name);
		parray_push(found, p);
	    }
	}
	closedir(dir);
    }

    if (parray_length(found) > 0)
	parray_sort_unique(found, strcmp);

    return found;
}
