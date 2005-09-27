/*
  $NiH: superfluous.c,v 1.3.2.7 2005/09/22 21:27:26 dillo Exp $

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


#include "dbh.h"
#include "dir.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"



parray_t *
find_superfluous(const char *dbname)
{
    dir_t *dir;
    char b[8192], dirname[8192], *p;
    parray_t *listf, *listd, *lst, *found;
    dir_status_t err;
    int i, len_dir, len_name;

    if (file_type == TYPE_ROM) {
	if ((listf=r_list(db, DDB_KEY_LIST_GAME)) == NULL) {
	    myerror(ERRDEF, "list of games not found in database `%s'",
		    dbname);
	    exit(1);
	}
	if ((listd=r_list(db, DDB_KEY_LIST_DISK)) == NULL) {
	    myerror(ERRDEF, "list of disks not found in database `%s'",
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
	if ((len_dir=snprintf(dirname, sizeof(dirname), "%s/%s", rompath[i],
			      file_type == TYPE_ROM ? "roms" : "samples"))
	    > sizeof(dirname)-1) {
	    myerror(ERRDEF, "ROMPATH entry too long, skipping: `%s'",
		    rompath[i]);
	    continue;
	}
	len_dir += 1; /* trailing '/' */
	if ((dir=dir_open(dirname, 0)) == NULL) {
	    myerror(ERRSTR, "can't open ROMPATH directory `%s'", dirname);
	    continue;
	}

	while ((err=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	    if (err == DIR_ERROR) {
		/* XXX: handle error */
		continue;
	    }

	    len_name = strlen(b+len_dir);

	    if (len_name > 4) {
		p = b+len_dir+len_name-4;
		if (strcmp(p, ".zip") == 0) {
		    *p = '\0';
		    lst = listf;
		}
		else if (strcmp(p, ".chd") == 0) {
		    *p = '\0';
		    lst = listd;
		}
		else {
		    p = NULL;
		    lst = listd;
		}
	    }
	    else {
		p = NULL;
		lst = listd;
	    }

	    if (lst == NULL
		|| parray_index_sorted(lst, b+len_dir, strcasecmp) == -1) {
		if (p)
		    *p = '.';
		
		parray_push(found, xstrdup(b));
	    }
	}
	dir_close(dir);
    }

    if (parray_length(found) > 0)
	parray_sort_unique(found, strcmp);

    return found;
}



void
print_superfluous(const parray_t *files)
{
    int i;

    if (parray_length(files) == 0)
	return;

    printf("Extra files found:\n");
    
    for (i=0; i<parray_length(files); i++)
	printf("%s\n", (char *)parray_get(files, i));
}
