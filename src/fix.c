/*
  $NiH: fix.c,v 1.1 2005/07/04 21:54:50 dillo Exp $

  fix.c -- fix ROM sets
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <zip.h>

#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "game.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

extern char *prg;

static int fix_file(rom_t *rom, match_t *m, archive_t **zip);
static int fix_add_garbage(archive_t *zip, int idx);
static char *mkgarbage_name(const char *name);

static struct zip *zf_garbage;
static char *zf_garbage_name = NULL;



int
fix_game(game_t *g, filetype_t ft, archive_t **zip, match_array_t *ma)
{
    int i;
    char *s;
    struct stat st;
    match_t *m;
    rom_t *r;

    zf_garbage = NULL;

    if (zip[0] == NULL) {
	if (zip[1] == NULL && zip[2] == NULL)
	    return 0;
	
	zip[0] = archive_new(g->name, ft,
			     archive_name(zip[1] ? zip[1] : zip[2]));
    }
    
    if (fix_do) {
	archive_ensure_zip(zip[0], 1);
	/* XXX: handle error */
    }

    for (i=0; i<game_num_files(g, ft); i++) {
	m = match_array_get(ma, i, 0);
	r = game_file(g, ft, i);

	if (match_quality(m) < ROM_NAMERR)
	    continue;
	
	if (rom_where(r) == ROM_INZIP && match_zno(m) != ROM_INZIP) {
	    fix_file(r, m, zip);
	    rom_state(archive_file(zip[match_zno(m)],
				   match_fno(m))) = ROM_NAMERR;
	}
	if (match_zno(m) == ROM_INZIP && match_quality(m) < ROM_BESTBADDUMP)
	    fix_file(r, m, zip);
    }

    for (i=0; i<archive_num_files(zip[0]); i++) {
	r = archive_file(zip[0], i);
	
	if (((rom_state(r) == ROM_UNKNOWN
	      || (rom_state(r) < ROM_NAMERR
		  && rom_where(r) != ROM_INZIP)))) {
	    if (fix_print)
		printf("%s: %s unknown file %s\n",
		       archive_name(zip[0]),
		       (fix_keep_unknown ? "mv" : "rm"),
		       rom_name(r));
	    if (fix_do) {
		if (fix_keep_unknown)
		    fix_add_garbage(zip[0], i);
		zip_delete(archive_zip(zip[0]), i);
	    }
	}
	else if (rom_state(r) < ROM_TAKEN) {
	    if (fix_print)
		printf("%s: %s unused file %s\n",
		       archive_name(zip[0]),
		       (fix_keep_unused ? "mv" : "rm"),
		       rom_name(r));
	    if (fix_do) {
		if (fix_keep_unused)
		    fix_add_garbage(zip[0], i);
		zip_delete(archive_zip(zip[0]), i);
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
fix_file(rom_t *r, match_t *m, archive_t **zip)
{
    struct zip_source *source;
    archive_t *afrom;
    struct zip *zfrom, *zto;

    afrom = zip[match_zno(m)];
    zfrom = archive_zip(afrom);
    zto = archive_zip(zip[0]);

    if (match_zno(m) != 0) {
	if (match_quality(m) == ROM_LONGOK) {
	    if (fix_do) {
		if ((source=zip_source_zip(zto,
					   zfrom,
					   match_fno(m), 0, match_offset(m),
					   rom_size(r))) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    fprintf(stderr, "%s: error adding `%s' to `%s': %s\n", prg,
			    rom_name(r), archive_name(zip[0]),
			    zip_strerror(zto));
		}
		if (fix_keep_long)
		    fix_add_garbage(afrom, match_fno(m));
	    }
	    if (fix_print)
		printf("%s: add `%s/%s' as %s, shrinking to %d/%ld\n",
		       archive_name(zip[0]), archive_name(afrom),
		       rom_name(archive_file(afrom, match_fno(m))),
		       rom_name(r), (int)match_offset(m), rom_size(r));
	}
	else {
	    if (fix_do) {
		if ((source=zip_source_zip(zto,
					   zfrom,
					   match_fno(m), 0, 0, 0)) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    fprintf(stderr, "%s: error adding `%s' to `%s': %s\n", prg,
			    rom_name(r), archive_name(zip[0]),
			    zip_strerror(zto));
		}
	    }
	    if (fix_print)
		printf("%s: add `%s/%s' as %s\n",
		       archive_name(zip[0]), archive_name(afrom),
		       rom_name(archive_file(afrom, match_fno(m))),
		       rom_name(r));
	}
    }
    else {
	switch (m->quality) {
	case ROM_NAMERR:
	    if (fix_do)
		zip_rename(zto, match_fno(m), rom_name(r));
	    if (fix_print)
		printf("%s: rename `%s' to %s\n",
		       archive_name(zip[0]),
		       rom_name(archive_file(zip[0], match_fno(m))),
		       rom_name(r));
	    break;

	case ROM_LONGOK:
	    if (fix_do) {
		if ((source=zip_source_zip(zto, zto, match_fno(m),
					   0, match_offset(m),
					   rom_size(r))) == NULL
		    || zip_replace(zto, match_fno(m), source) < 0) {
		    zip_source_free(source);
		    fprintf(stderr, "%s: error shortening `%s' in `%s': %s\n",
			    prg, rom_name(r), archive_name(zip[0]),
			    zip_strerror(archive_zip(zip[0])));
		}
#if 0
		/* XXX: rename? */
		zip_rename(archive_zip(zip[0]), match_fno(m), rom_name(r));
#endif
		if (fix_keep_long)
		    fix_add_garbage(afrom, match_fno(m));
	    }
	    if (fix_print)
		printf("%s: shrink `%s' as %s to %d/%ld\n",
		       archive_name(zip[0]),
		       rom_name(archive_file(zip[0], match_fno(m))),
		       rom_name(r), (int)match_offset(m), rom_size(r));
	    break;

	default:
	    break;
	}
    }

    return 0;
}



static int
fix_add_garbage(archive_t *zip, int idx)
{
    struct zip_source *source;

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
    if (zf_garbage) {
	if ((source=zip_source_zip(zf_garbage,
				   archive_zip(zip), idx,
				   ZIP_FL_UNCHANGED, 0, 0)) == NULL
	    || zip_add(zf_garbage, rom_name(archive_file(zip, idx)),
		       source) < 0) {
	    zip_source_free(source);
	    fprintf(stderr, "%s: error adding `%s' to garbage file `%s': %s\n",
		    prg, rom_name(archive_file(zip, idx)), zf_garbage_name,
		    zip_strerror(zf_garbage));
	}
    }

    return 0;
}



static char *
mkgarbage_name(const char *name)
{
    const char *s;
    char *t;

    if ((s=strrchr(name, '/')) == NULL)
	s = name;
    else
	s++;

    t = (char *)xmalloc(strlen(name)+strlen("garbage/")+1);

    sprintf(t, "%.*sgarbage/%s", (int)(s-name), name, s);

    return t;
}
