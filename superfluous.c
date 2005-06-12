/*
  $NiH: superfluous.c,v 1.7 2005/06/12 17:18:59 dillo Exp $

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



#include <dirent.h>
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
    char b[8192], **list, **lists, **listx, *p, **lst;
    char **found;
    int i, j, l, nfound, nalloced, first;
    int nlist, nlists, nlistx, nlst;

    first = 1;

    if ((nlist=r_list(db, "/list", &list)) < 0) {
	myerror(ERRDEF, "list of games not found in database `%s'", dbname);
	exit(1);
    }
    if ((nlists=r_list(db, "/sample_list", &lists)) < 0) {
	myerror(ERRDEF, "list of samples not found in database `%s'", dbname);
	exit(1);
    }
    if ((nlistx=r_list(db, "/extra_list", &listx)) < 0) {
	myerror(ERRDEF, "list of extra files not found in database `%s'",
		dbname);
	exit(1);
    }

    if (nlist < 1 || (sample && nlists < 1))
	return 0;

    init_rompath();

    for (i=0; rompath[i]; i++) {
	sprintf(b, "%s/%s", rompath[i], sample ? "samples" : "roms");
	if ((dir=opendir(b)) == NULL) {
	    /* XXX: error */
	    continue;
	}

	found = NULL;
	nalloced = nfound = 0;

	while ((de=readdir(dir))) {
	    l = de->d_namlen;
	    p = de->d_name;

	    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
		continue;

	    if (l > 4 && strcmp(p+l-4, ".zip") == 0) {
		strncpy(b, p, sizeof(b));
		b[l-4] = '\0';
		p = b;
		lst = sample ? lists : list;
		nlst = sample ? nlists : nlist;
	    }
	    else if (l > 4 && strcmp(p+l-4, ".chd") == 0) {
		strncpy(b, p, sizeof(b));
		b[l-4] = '\0';
		p = b;
		lst = listx;
		nlst = nlistx;
	    }
	    else {
		lst = listx;
		nlst = nlistx;
	    }

	    if (bsearch(&p, lst, nlst, sizeof(char *),
			(cmpfunc)strpcasecmp) == NULL) {
		if (nfound >= nalloced) {
		    if (nfound == 0)
			nalloced = 16;
		    else
			nalloced *= 2;
		    found = xrealloc(found, sizeof(*found)*nalloced);
		}
		found[nfound++] = xstrdup(de->d_name);
	    }
	}
	closedir(dir);

	if (nfound) {
	    if (first) {
		printf("Extra files found:\n");
		first = 0;
	    }

	    qsort(found, nfound, sizeof(*found), docmp);

	    for (j=0; j<nfound; j++) {
		printf("%s/%s/%s\n", rompath[i],
		       sample ? "samples" : "roms", found[j]);
		free(found[i]);
	    }

	    free(found);
	}
    }

    return 0;
}
