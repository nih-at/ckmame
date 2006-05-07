/*
  $NiH: check_files.c,v 1.10 2006/05/05 01:10:08 dillo Exp $

  check_files.c -- match files against ROMs
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



#include "archive.h"
#include "dbh.h"
#include "file_location.h"
#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "match.h"
#include "rom.h"
#include "util.h"



enum test {
    TEST_NSC,
    TEST_MSC,
    TEST_SCI,
    TEST_LONG
};

typedef enum test test_t;

enum test_result {
    TEST_NOTFOUND,
    TEST_UNUSABLE,
    TEST_USABLE
};

typedef enum test_result test_result_t;

static test_result_t match_files(archive_t *, test_t, const rom_t *,
				 match_t *);
static void update_game_status(const game_t *, result_t *);



void
check_files(game_t *g, archive_t *as[3], result_t *res)
{
    static const test_t tests[] = {
	TEST_NSC,
	TEST_SCI,
	TEST_LONG
    };
    static const int tests_count = sizeof(tests)/sizeof(tests[0]);
    
    int i, j;
    rom_t *r;
    match_t *m;
    archive_t *a;
    test_result_t result;
    int *user;

    if (result_game(res) == GS_OLD)
	return;

    for (i=0; i<game_num_files(g, file_type); i++) {
	r = game_file(g, file_type, i);
	m = result_rom(res, i);
	a = as[rom_where(r)];

	if (match_quality(m) == QU_OLD)
	    continue;

	match_quality(m) = QU_MISSING;

	/* check if it's in ancestor */
	if (rom_where(r) != ROM_INZIP
	    && a && (result=match_files(a, TEST_MSC, r, m)) != TEST_NOTFOUND) {
	    match_where(m) = rom_where(r);
	    if (result == TEST_USABLE)
		continue;
	}
	
	/* search for matching file in game's zip */
	if (as[0]) {
	    for (j=0; j<tests_count; j++) {
		if ((result=match_files(as[0],
					tests[j], r, m)) != TEST_NOTFOUND) {
		    match_where(m) = ROM_INZIP;
		    if (rom_where(r) != ROM_INZIP
			&& match_quality(m) == QU_OK) {
			match_quality(m) = QU_INZIP;
		    }
		    if (result == TEST_USABLE)
			break;
		}
	    }
	}

	if (rom_where(r) == ROM_INZIP
	    && (match_quality(m) == QU_MISSING
		|| match_quality(m) == QU_HASHERR)
	    && rom_size(r) > 0 && rom_status(r) != STATUS_NODUMP) {
	    /* search for matching file in other games (via db) */
	    if (find_in_romset(r, game_name(g), m) == FIND_EXISTS)
		continue;
	    
	    /* search for matching file in needed */
	    ensure_needed_maps();
	    if (find_in_archives(needed_file_map, r, m) == FIND_EXISTS)
		continue;

	    /* search for matching file in superfluous and update sets */
	    ensure_extra_maps(DO_MAP);
	    if (find_in_archives(extra_file_map, r, m) == FIND_EXISTS)
		continue;
	}
    }

    if (as[0] && archive_num_files(as[0]) > 0) {
	user = malloc(sizeof(int) * archive_num_files(as[0]));
	for (i=0; i<archive_num_files(as[0]); i++)
	    user[i] = -1;
    
	for (i=0; i<game_num_files(g, file_type); i++) {
	    m = result_rom(res, i);
	    if (match_where(m) == ROM_INZIP
		&& match_quality(m) != QU_HASHERR) {
		j = match_index(m);
		if (result_file(res, j) != FS_USED)
		    result_file(res, j)
			= match_quality(m) == QU_LONG ? FS_PARTUSED : FS_USED;
		
		if (match_quality(m) != QU_LONG
		    && match_quality(m) != QU_INZIP) {
		    if (user[j] == -1)
			user[j] = i;
		    else {
			if (match_quality(m) == QU_OK) {
			    match_quality(result_rom(res, user[j]))
				= QU_COPIED;
			    user[j] = i;
			}
			else
			    match_quality(m) = QU_COPIED;
		    }
		}
	    }
	}
    }

    update_game_status(g, res);
}



static test_result_t
match_files(archive_t *a, test_t t, const rom_t *r, match_t *m)
{
    int i;
    const rom_t *ra;
    test_result_t result;

    match_offset(m) = -1;

    result = TEST_NOTFOUND;
	
    for (i=0; result != TEST_USABLE && i<archive_num_files(a); i++) {
	ra = archive_file(a, i);

	if (rom_status(ra) != STATUS_OK)
	    continue;

	switch (t) {
	case TEST_NSC:
	case TEST_MSC:
	    if ((t == TEST_NSC
		 ? (rom_compare_n(r, ra) == 0)
		 : (rom_compare_m(r, ra) == 0))
		&& rom_compare_sc(r, ra) == 0) {
		if ((hashes_cmp(rom_hashes(r), rom_hashes(ra))
		     != HASHES_CMP_MATCH)) {
		    if (match_quality(m) == QU_HASHERR)
			break;
		    
		    match_quality(m) = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    match_quality(m) = QU_OK;
		    result = TEST_USABLE;
		}
		match_archive(m) = a;
		match_index(m) = i;
	    }
	    break;

	case TEST_SCI:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(rom_hashes(r)) == 0)
		break;
	    
	    if (rom_compare_sc(r, ra) == 0) {
		if (archive_file_compare_hashes(a, i, rom_hashes(r)) != 0) {
		    if (rom_status(archive_file(a, i)) != STATUS_OK)
			break;
		    if (match_quality(m) == QU_HASHERR)
			break;
		    
		    match_quality(m) = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    match_quality(m) = QU_NAMEERR;
		    result = TEST_USABLE;
		}
	    }
	    match_archive(m) = a;
	    match_index(m) = i;
	    break;

	case TEST_LONG:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(rom_hashes(r)) == 0)
		break;
	    
	    if (rom_compare_n(r, ra) == 0
		&& rom_size(ra) > rom_size(r)
		&& (match_offset(m)=archive_file_find_offset(a, i, rom_size(r),
							     rom_hashes(r)))
		!= -1) {
		match_archive(m) = a;
		match_index(m) = i;
		match_quality(m) = QU_LONG;
		return TEST_USABLE;
	    }
	    break;
	}
    }

    return result;
}



static void
update_game_status(const game_t *g, result_t *res)
{
    int i;
    int all_dead, all_own_dead, all_correct, all_fixable, has_own;
    const match_t *m;
    const rom_t *r;

    all_own_dead = all_dead = all_correct = all_fixable = 1;
    has_own = 0;

    for (i=0; i<game_num_files(g, file_type); i++) {
	m = result_rom(res, i);
	r = game_file(g, file_type, i);

	if (rom_where(r) == ROM_INZIP)
	    has_own = 1;
	if (match_quality(m) == QU_MISSING)
	    all_fixable = 0;
	if (match_quality(m) != QU_MISSING) {
	    all_dead = 0;
	    if (rom_where(r) == ROM_INZIP)
		all_own_dead = 0;
	}
	if (match_quality(m) != QU_OK)
	    all_correct = 0;
    }

    if (all_correct)
	result_game(res) = GS_CORRECT;
    else if (all_dead || (has_own && all_own_dead))
	result_game(res) = GS_MISSING;
    else if (all_fixable)
	result_game(res) = GS_FIXABLE;
    else
	result_game(res) = GS_PARTIAL;
}
