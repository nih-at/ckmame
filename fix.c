/*
  $NiH: fix.c,v 1.16 2003/12/28 00:56:06 wiz Exp $

  fix.c -- fix romsets
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "types.h"
#include "dbl.h"
#include "error.h"
#include "util.h"
#include "romutil.h"
#include "xmalloc.h"

extern char *prg;

static int fix_file(struct rom *rom, struct match *m, struct zfile **zip);
static int fix_add_garbage(struct zfile *zip, int idx);
static char *mkgarbage_name(char *name);

static struct zip *zf_garbage;
static char *zf_garbage_name = NULL;



int
fix_game(struct game *g, struct zfile **zip, struct match *m)
{
    int i;
    char *s;
    struct stat st;

    zf_garbage = NULL;

    if (zip[0] == NULL) {
	if (zip[1] == NULL && zip[2] == NULL)
	    return 0;
	
	zip[0] = zfile_new(g->name, 0, (zip[1] ? zip[1] : zip[2])->name);
    }
    
    if (fix_do) 
	if (zip[0]->zf == NULL) {
	    int zerr;

	    zerr = 0;
	    zip[0]->zf = zip_open(zip[0]->name, ZIP_CREATE, &zerr);
	    if (zip[0]->zf == NULL) {
		char errstr[1024];

		(void)zip_error_str(errstr, sizeof(errstr),
				    zerr, errno);
		fprintf(stderr, "%s: error opening '%s': %s\n", prg,
			zip[0]->name, errstr);
	    }
	}

    for (i=0; i<g->nrom; i++) {
	if (m[i].quality < ROM_NAMERR)
	    continue;
	
	if (g->rom[i].where == ROM_INZIP && m[i].zno != ROM_INZIP) {
	    fix_file(g->rom+i, m+i, zip);
	    zip[m[i].zno]->rom[m[i].fno].state = ROM_NAMERR;
	}
	if (m[i].zno == ROM_INZIP && m[i].quality < ROM_BESTBADDUMP) {
	    fix_file(g->rom+i, m+i, zip);
	}
    }

    for (i=0; i<zip[0]->nrom; i++) {
	if (((zip[0]->rom[i].state == ROM_UNKNOWN
	      || (zip[0]->rom[i].state < ROM_NAMERR
		  && zip[0]->rom[i].where != ROM_INZIP)))) {
	    if (fix_print)
		printf("%s: %s unknown file %s\n",
		       zip[0]->name,
		       (fix_keep_unknown ? "mv" : "rm"),
		       zip[0]->rom[i].name);
	    if (fix_do) {
		if (fix_keep_unknown)
		    fix_add_garbage(zip[0], i);
		zip_delete(zip[0]->zf, i);
	    }
	}
	else if (zip[0]->rom[i].state < ROM_TAKEN) {
	    if (fix_print)
		printf("%s: %s unused file %s\n",
		       zip[0]->name,
		       (fix_keep_unused ? "mv" : "rm"),
		       zip[0]->rom[i].name);
	    if (fix_do) {
		if (fix_keep_unused)
		    fix_add_garbage(zip[0], i);
		zip_delete(zip[0]->zf, i);
	    }
	}
    }

    if (zf_garbage) {
	if (zip_get_num_files(zf_garbage) > 0) {
	    s = strrchr(zf_garbage_name, '/');
	    if (s) {
		*s = 0;
		if (stat(zf_garbage_name, &st) < 0) {
		    if (mkdir(zf_garbage_name, 0777) < 0) {
			fprintf(stderr, "%s: mkdir `%s' error failed: %s\n", prg,
				zf_garbage_name, strerror(errno));
			/* XXX: problem */
		    }
		} else {
		    if (!(st.st_mode & S_IFDIR)) {
			fprintf(stderr, "%s: `%s' is not a directory\n", prg,
				zf_garbage_name);
			/* XXX: problem */
		    }
		}
		*s = '/';
	    } else {
		/* XXX: internal error */
		fprintf(stderr, "%s: internal error: no slash in "
			"zf_garbage_name: `%s'\n",
			prg, zf_garbage_name);
	    }
	}		    
	zip_close(zf_garbage);
    }

    return 0;
}



static int
fix_file(struct rom *rom, struct match *m, struct zfile **zip)
{
    if (m->zno != 0) {
	if (m->quality == ROM_LONGOK) {
	    if (fix_do) {
		zip_add_zip(zip[0]->zf, rom->name, NULL,
			    zip[m->zno]->zf, m->fno, m->offset, rom->size);
		if (fix_keep_long)
		    fix_add_garbage(zip[m->zno], m->fno);
	    }
	    if (fix_print)
		printf("%s: add `%s/%s' as %s, shrinking to %d/%ld\n",
		       zip[0]->name,
		       zip[m->zno]->name, zip[m->zno]->rom[m->fno].name,
		       rom->name, m->offset, rom->size);
	}
	else {
	    if (fix_do)
		zip_add_zip(zip[0]->zf, rom->name, NULL,
			    zip[m->zno]->zf, m->fno, 0, 0);
	    if (fix_print)
		printf("%s: add `%s/%s' as %s\n",
		       zip[0]->name,
		       zip[m->zno]->name, zip[m->zno]->rom[m->fno].name,
		       rom->name);
	}
    }
    else {
	switch (m->quality) {
	case ROM_NAMERR:
	    if (fix_do)
		zip_rename(zip[0]->zf, m->fno, rom->name);
	    if (fix_print)
		printf("%s: rename `%s' to %s\n",
		       zip[0]->name,
		       zip[0]->rom[m->fno].name,
		       rom->name);
	    break;

	case ROM_LONGOK:
	    if (fix_do) {
		zip_replace_zip(zip[0]->zf, m->fno, rom->name, NULL,
				zip[0]->zf, m->fno, m->offset, rom->size);
		if (fix_keep_long)
		    fix_add_garbage(zip[m->zno], m->fno);
	    }
	    if (fix_print)
		printf("%s: shrink `%s' as %s to %d/%ld\n",
		       zip[0]->name,
		       zip[0]->rom[m->fno].name,
		       rom->name, m->offset, rom->size);
	    break;

	default:
	    break;
	}
    }

    return 0;
}



static int
fix_add_garbage(struct zfile *zip, int idx)
{
    if (!fix_do)
	return 0;

    if (zf_garbage == NULL) {
	if (zf_garbage_name != NULL) {
	    free(zf_garbage_name);
	    zf_garbage_name = NULL;
	}
	zf_garbage_name = mkgarbage_name(zip->name);
	zf_garbage = zip_open(zf_garbage_name, ZIP_CREATE, NULL);
    }
    if (zf_garbage)
	zip_add_zip(zf_garbage, NULL, NULL, zip->zf, idx, 0, 0);

    return 0;
}



static char *
mkgarbage_name(char *name)
{
    char *s;
    char *t;

    if ((s=strrchr(name, '/')) == NULL)
	s = name;
    else
	s++;

    t = (char *)xmalloc(strlen(name)+strlen("garbage/")+1);

    sprintf(t, "%.*sgarbage/%s", (int)(s-name), name, s);

    return t;
}
