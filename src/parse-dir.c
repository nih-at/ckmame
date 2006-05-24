/*
  $NiH$

  parse-dir.c -- read info from zip archives
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "dir.h"
#include "error.h"
#include "funcs.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"



static int parse_archive(parser_context_t *, archive_t *);




int
parse_dir(const char *dname, parser_context_t *ctx)
{
    dir_t *dir;
    archive_t *a;
    char b[8192];
    dir_status_t ds;
    name_type_t nt;

    ctx->lineno = 0;
    
    if ((dir=dir_open(dname, DIR_RECURSE)) == NULL)
	return -1;

    while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	if (ds == DIR_ERROR) {
	    /* XXX: handle error */
	    continue;
	}
	switch ((nt=name_type(b))) {
	case NAME_ZIP:
	    if ((a=archive_new(b, 0)) != NULL) {
		parse_archive(ctx, a);
		archive_free(a);
	    }
	    break;
	    
	case NAME_CHD:
	case NAME_NOEXT:
	case NAME_UNKNOWN:
	    /* ignore all but zip archives */
	    break;
	}
    }
    return 0;
}



static int
parse_archive(parser_context_t *ctx, archive_t *a)
{
    char *name;
    int i;
    rom_t *r;
    char hstr[HASHES_SIZE_MAX*2+1];

    parse_game_start(ctx, 0);

    name = xstrdup(mybasename(archive_name(a)));
    if (strlen(name) > 4 && strcmp(name+strlen(name)-4, ".zip") == 0)
	name[strlen(name)-4] = '\0';
    parse_game_name(ctx, 0, 0, name);
    free(name);

    for (i=0; i<archive_num_files(a); i++) {
	if (archive_file_compute_hashes(a, i,
		   HASHES_TYPE_CRC|HASHES_TYPE_MD5|HASHES_TYPE_SHA1) < 0)
	    continue;
	r = archive_file(a, i);

	parse_file_start(ctx, TYPE_ROM);
	parse_file_name(ctx, TYPE_ROM, 0, rom_name(r));
	sprintf(hstr, "%lu", rom_size(r));
	parse_file_size(ctx, TYPE_ROM, 0, hstr);
	parse_file_hash(ctx, TYPE_ROM, HASHES_TYPE_CRC,
			hash_to_string(hstr, HASHES_TYPE_CRC, rom_hashes(r)));
	parse_file_hash(ctx, TYPE_ROM, HASHES_TYPE_MD5,
			hash_to_string(hstr, HASHES_TYPE_MD5, rom_hashes(r)));
	parse_file_hash(ctx, TYPE_ROM, HASHES_TYPE_SHA1,
			hash_to_string(hstr, HASHES_TYPE_SHA1, rom_hashes(r)));
	parse_file_end(ctx,TYPE_ROM);
    }

    parse_game_end(ctx, 0);

    return 0;
}
