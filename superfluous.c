/*
  $NiH: superfluous.c,v 1.2 2004/02/26 02:26:12 wiz Exp $

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
handle_extra_files(DB *db, char *dbname, int sample)
{
    FILE *fin;
    char b[8192], *p, bo[8192], *po, **list, **lists, **listx;
    int i, l, nfound;
    int nlist, nlists, nlistx;

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

    p = b;
    po = bo;
    for (i=0; rompath[i]; i++) {
	/* XXX: use opendir */
	sprintf(b, "ls %s/%s", rompath[i], sample ? "samples" : "roms");
	if ((fin=popen(b, "r")) == NULL) {
	    /* XXX: error */
	    continue;
	}

	while (fgets(b, 8192, fin)) {
	    l = strlen(b);
	    if (b[l-1] == '\n')
		b[--l] = '\0';
	    strncpy(bo, b, sizeof(bo));
	    if (l > 4 && (strcmp(b+l-4, ".zip") == 0
			  || strcmp(b+l-4, ".chd") == 0)) {
		b[l-4] = '\0';
	    }
	    if (bsearch(&p, sample ? lists : list,
			sample ? nlists : nlist,
			sizeof(char *),
			(cmpfunc)strpcasecmp) == NULL
		&& bsearch(&po, listx, nlistx, sizeof(char *),
			   (cmpfunc)strpcasecmp) == NULL) {
		if (nfound++ == 0)
		    printf("Extra files found:\n");
		printf("%s/%s/%s\n", rompath[i],
		       sample ? "samples" : "roms", bo);
	    }
	}
	pclose(fin);
    }

    return 0;
}
