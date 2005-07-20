/*
  $NiH: check_roms.c,v 1.1.2.3 2005/07/19 22:46:48 dillo Exp $

  check_roms.c -- match files against ROMs
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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



#include "archive.h"
#include "dbh.h"
#include "file_by_hash.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "match.h"
#include "rom.h"
#include "util.h"



enum test {
    TEST_NSC,
    TEST_SCI,
    TEST_LONG
};

typedef enum test test_t;

enum test_result {
    TEST_ERROR = -1,
    TEST_SUCCESS,
    TEST_FAILURE
};

typedef enum test_result test_result_t;

static test_result_t check_for_file_in_zip(const char *, const rom_t *,
					   match_t *);
static test_result_t match_files(archive_t *, test_t, const rom_t *,
				 match_t *);
static test_result_t find_in_archives(map_t *, const rom_t *, match_t *);
static test_result_t find_in_romset(const rom_t *, const char *, match_t *);




int
check_roms(game_t *g, filetype_t ft, archive_t as[3])
{
    static const struct test_result {
	test_t test;
	quality_t quality;
    } test_results[] = {
	{ TEST_NSC, QU_OK },
	{ TEST_SCI, QU_NAMEERR },
	{ TEST_LONG, QU_LONG },
    };
    static const int test_result_count
	= sizeof(test_results)/sizeof(test_results[0]);
    
    match_array_t *ma;
    int i, j;
    rom_t *r;
    match_t *m;
    archive_t *a;

    ma = match_array_new(game_num_files(g, ft));

    for (i=0; i<game_num_files(g, ft); i++) {
	r = game_file(g, ft, i);
	m = match_array_get(ma, i);
	a = &as[rom_where(r)];

	m->quality = QU_MISSING;

	/* check if it's in ancestor */
	if (rom_where(r) != ROM_INZIP
	    && match_files(a, TEST_NSC, r, m) == TEST_SUCCESS) {
	    m->quality = QU_ANCESTOR_OK;
	    continue;
	}
	
	/* search for matching file in game's zip */
	for (j=0; j<test_result_count; j++) {
	    if (match_files(as, test_results[j].test, r, m) == TEST_SUCCESS) {
		m->quality = test_results[j].quality;
		break;
	    }
	}

	if (rom_where(r) == ROM_INZIP && m->quality == QU_MISSING
	    && rom_size(r) > 0 && rom_status(r) != FLAGS_NODUMP) {
	    /* search for matching file in family (not yet) */

	    /* search for matching file in other games (via db) */
	    if (find_in_romset(r, game_name(g), m) == TEST_SUCCESS)
		continue;
	    
	    /* search for matching file in needed */
	    if (find_in_archives(needed_map, r, m) == TEST_SUCCESS)
		continue;

	    /* search for matching file in superfluous and update sets */
	    if (find_in_archives(extra_file_map, r, m) == TEST_SUCCESS)
		continue;
	}
    }

    return 0;
}



static test_result_t
check_for_file_in_zip(const char *name, const rom_t *r, match_t *m)
{
    archive_t *a;
    int idx;

    if ((a=archive_new(name, TYPE_ROM, NULL)) == NULL)
	return TEST_FAILURE;
    
    if ((idx=archive_file_index_by_name(a, rom_name(r))) >= 0
	&& archive_file_compare_hashes(a, idx,
				       rom_hashes(r)) == HASHES_CMP_MATCH) {
	m->archive = a;
	m->index = idx;
	return TEST_SUCCESS;
    }

    archive_free(a);

    return TEST_FAILURE;
}



static test_result_t
find_in_archives(map_t *map, const rom_t *r, match_t *m)
{
    parray_t *pa;
    file_by_hash_t *fbh;
    archive_t *a;
    int i;

    if ((pa=map_get(map, file_by_hash_default_hashtype(TYPE_ROM),
		    rom_hashes(r))) == NULL)
	return TEST_FAILURE;

    for (i=0; i<parray_length(pa); i++) {
	fbh = parray_get(pa, i);

	if ((a=archive_new(file_by_hash_name(fbh),
			   TYPE_FULL_PATH, NULL)) == NULL) {
	    /* XXX: internal error */
	    return TEST_ERROR;
	}

	switch (archive_file_compare_hashes(a, file_by_hash_index(fbh),
					    rom_hashes(r))) {
	case HASHES_CMP_MATCH:
	    m->archive = a;
	    m->index = file_by_hash_index(fbh);
	    m->quality = QU_COPIED;
	    return TEST_SUCCESS;

	case HASHES_CMP_NOCOMMON:
	    archive_free(a);
	    return TEST_ERROR;

	default:
	    archive_free(a);
	    break;
	}
    }

    return TEST_FAILURE;
}



static test_result_t
find_in_romset(const rom_t *r, const char *skip, match_t *m)
{
    array_t *a;
    file_by_hash_t *fbh;
    game_t *g;
    const rom_t *gr;
    int i;
    int status;

    if ((a=r_file_by_hash(db, TYPE_ROM, rom_hashes(r))) == NULL) {
	/* XXX: internal error: db inconsistency */
	return -1;
    }

    status = TEST_FAILURE;
    for (i=0; status==TEST_FAILURE && i<array_length(a); i++) {
	fbh = array_get(a, i);

	if (strcmp(file_by_hash_name(fbh), skip) == 0)
	    continue;

	if ((g=r_game(db, file_by_hash_name(fbh))) == NULL) {
	    /* XXX: internal error: db inconsistency */
	    status = TEST_ERROR;
	    break;
	}

	gr = game_file(g, TYPE_ROM, file_by_hash_index(fbh));

	if (hashes_cmp(rom_hashes(r), rom_hashes(gr)) == HASHES_CMP_MATCH) {
	    status = check_for_file_in_zip(game_name(g), gr, m);
	    if (status == TEST_SUCCESS)
		m->quality = QU_COPIED;
	}

	game_free(g);
    }

    array_free(a, file_by_hash_finalize);

    return status;
}



static test_result_t
match_files(archive_t *a, test_t t, const rom_t *r, match_t *m)
{
    int i;
    const rom_t *ra;

    m->offset = -1;

    for (i=0; i<archive_num_files(a); i++) {
	ra = archive_file(a, i);

	if (rom_status(ra) != FLAGS_OK)
	    continue;

	switch (t) {
	case TEST_NSC:
	    if (rom_compare_nsc(r, ra) == 0) {
		m->archive = a;
		m->index = i;
		return 0;
	    }
	    break;

	case TEST_SCI:
	    if (rom_compare_sc(r, ra) == 0
		&& archive_file_compare_hashes(a, i, rom_hashes(r)) == 0) {
		m->archive = a;
		m->index = i;
		return 0;
	    }
	    break;

	case TEST_LONG:
	    if (rom_compare_n(r, ra) == 0
		&& (m->offset=archive_file_find_offset(a, i, rom_size(r),
						       rom_hashes(r))) != -1) {
		m->archive = a;
		m->index = i;
		return 0;
	    }
	    break;
	}
    }

    return 1;
}
