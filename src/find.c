/*
  $NiH: find.c,v 1.6 2006/04/13 21:41:37 dillo Exp $

  find.c -- find ROM in ROM set or archives
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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



#include "dbh.h"
#include "file_location.h"
#include "find.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "util.h"
#include "xmalloc.h"

static find_result_t check_for_file_in_zip(const char *, const rom_t *,
					   match_t *);




find_result_t
find_disk(map_t *map, const disk_t *d, match_disk_t *m)
{
    parray_t *pa;
    file_location_ext_t *fbh;
    disk_t *dm;
    int i;

    if ((pa=map_get(map, file_location_default_hashtype(TYPE_DISK),
		    disk_hashes(d))) == NULL)
	return FIND_UNKNOWN;

    for (i=0; i<parray_length(pa); i++) {
	fbh = parray_get(pa, i);

	if ((dm=disk_new(file_location_ext_name(fbh), 0)) == NULL) {
	    /* XXX: internal error */
	    return FIND_ERROR;
	}

	switch (hashes_cmp(disk_hashes(d), disk_hashes(dm))) {
	case HASHES_CMP_MATCH:
	    m->name = xstrdup(disk_name(dm));
	    hashes_copy(match_disk_hashes(m), disk_hashes(dm));
	    m->quality = QU_COPIED;
	    disk_free(dm);
	    return FIND_EXISTS;

	case HASHES_CMP_NOCOMMON:
	    disk_free(dm);
	    return FIND_ERROR;

	default:
	    disk_free(dm);
	    break;
	}
    }

    return FIND_UNKNOWN;
}



find_result_t
find_disk_in_romset(DB *db, const disk_t *d, const char *skip,
		    match_disk_t *md)
{
    array_t *a;
    file_location_t *fbh;
    game_t *g;
    const disk_t *gd;
    disk_t *f;
    int i;
    find_result_t status;
    char *file_name;

    if ((a=r_file_by_hash(db, TYPE_DISK, disk_hashes(d))) == NULL) {
	/* XXX: internal error: db inconsistency */
	return FIND_ERROR;
    }

    status = FIND_UNKNOWN;
    for (i=0;
	 (status != FIND_ERROR && status != FIND_EXISTS) && i<array_length(a);
	 i++) {
	fbh = array_get(a, i);

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g=r_game(db, file_location_name(fbh))) == NULL) {
	    /* XXX: internal error: db inconsistency */
	    status = FIND_ERROR;
	    break;
	}

	gd = game_disk(g, file_location_index(fbh));

	if (hashes_cmp(disk_hashes(d), disk_hashes(gd)) == HASHES_CMP_MATCH) {
	    status = FIND_MISSING;
	    
	    file_name = findfile(disk_name(gd), TYPE_DISK);
	    if (file_name != NULL) {
		f = disk_new(file_name, 1);
		if (f) {
		    if (hashes_cmp(disk_hashes(d), disk_hashes(f))
			== HASHES_CMP_MATCH) {
			status = FIND_EXISTS;
			md->quality = QU_COPIED;
			md->name = file_name;
			hashes_copy(&md->hashes, disk_hashes(f));
		    }
		    else
			free(file_name);
		    disk_free(f);
		}
		else
		    free(file_name);
	    }
	}

	game_free(g);
    }

    array_free(a, file_location_finalize);

    return status;
}



find_result_t
find_in_archives(map_t *map, const rom_t *r, match_t *m)
{
    parray_t *pa;
    file_location_ext_t *fbh;
    archive_t *a;
    int i;

    if ((pa=map_get(map, file_location_default_hashtype(TYPE_ROM),
		    rom_hashes(r))) == NULL)
	return FIND_UNKNOWN;

    for (i=0; i<parray_length(pa); i++) {
	fbh = parray_get(pa, i);

	if ((a=archive_new(file_location_ext_name(fbh),
			   TYPE_FULL_PATH, 0)) == NULL
	    || archive_num_files(a) < file_location_ext_index(fbh)) {
	    /* XXX: internal error */
	    return FIND_ERROR;
	}

	switch (archive_file_compare_hashes(a, file_location_ext_index(fbh),
					    rom_hashes(r))) {
	case HASHES_CMP_MATCH:
	    m->archive = a;
	    m->index = file_location_ext_index(fbh);
	    m->where = file_location_ext_where(fbh);
	    m->quality = QU_COPIED;
	    return FIND_EXISTS;

	case HASHES_CMP_NOCOMMON:
	    archive_free(a);
	    return FIND_ERROR;

	default:
	    archive_free(a);
	    break;
	}
    }

    return FIND_UNKNOWN;
}



find_result_t
find_in_romset(DB *db, const rom_t *r, const char *skip, match_t *m)
{
    array_t *a;
    file_location_t *fbh;
    game_t *g;
    const rom_t *gr;
    int i;
    find_result_t status;

    if ((a=r_file_by_hash(db, TYPE_ROM, rom_hashes(r))) == NULL) {
	/* XXX: internal error: db inconsistency */
	return FIND_ERROR;
    }

    status = FIND_UNKNOWN;
    for (i=0;
	 (status != FIND_ERROR && status != FIND_EXISTS) && i<array_length(a);
	 i++) {
	fbh = array_get(a, i);

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g=r_game(db, file_location_name(fbh))) == NULL) {
	    /* XXX: internal error: db inconsistency */
	    status = FIND_ERROR;
	    break;
	}

	gr = game_file(g, TYPE_ROM, file_location_index(fbh));

	if (hashes_cmp(rom_hashes(r), rom_hashes(gr)) == HASHES_CMP_MATCH) {
	    status = check_for_file_in_zip(game_name(g), gr, m);
	    if (status == FIND_EXISTS) {
		m->quality = QU_COPIED;
		m->where = ROM_ROMSET;
	    }
	}

	game_free(g);
    }

    array_free(a, file_location_finalize);

    return status;
}



static find_result_t
check_for_file_in_zip(const char *name, const rom_t *r, match_t *m)
{
    archive_t *a;
    int idx;

    if ((a=archive_new(name, TYPE_ROM, 0)) == NULL)
	return FIND_MISSING;
    
    if ((idx=archive_file_index_by_name(a, rom_name(r))) >= 0
	&& archive_file_compare_hashes(a, idx,
				       rom_hashes(r)) == HASHES_CMP_MATCH) {
	m->archive = a;
	m->index = idx;
	return FIND_EXISTS;
    }

    archive_free(a);

    return FIND_MISSING;
}
