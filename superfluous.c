/*
  $NiH: superfluous.c,v 1.5 2004/04/26 09:23:46 dillo Exp $

  superfluous.c -- check for unknown file in rom directories
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

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



int
handle_extra_files(DB *db, const char *dbname, int sample)
{
    DIR *dir;
    struct dirent *de;
    char b[8192], **list, **lists, **listx, *p, **lst;
    int i, l, nfound;
    int nlist, nlists, nlistx, nlst;

    nfound = 0;

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
		if (nfound++ == 0)
		    printf("Extra files found:\n");
		printf("%s/%s/%s\n", rompath[i],
		       sample ? "samples" : "roms", de->d_name);
	    }
	}
	closedir(dir);
    }

    return 0;
}
