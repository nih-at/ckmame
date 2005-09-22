/*
  $NiH: check_files.c,v 1.1.2.8 2005/09/22 19:38:13 dillo Exp $

  check_files.c -- match files against ROMs
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



match_array_t *
check_files(game_t *g, archive_t *as[3])
{
    static const test_t tests[] = {
	TEST_NSC,
	TEST_SCI,
	TEST_LONG
    };
    static const int tests_count
	= sizeof(tests)/sizeof(tests[0]);
    
    match_array_t *ma;
    int i, j;
    rom_t *r;
    match_t *m;
    archive_t *a;
    test_result_t result;

    ma = match_array_new(game_num_files(g, file_type));

    for (i=0; i<game_num_files(g, file_type); i++) {
	r = game_file(g, file_type, i);
	m = match_array_get(ma, i);
	a = as[rom_where(r)];

	m->quality = QU_MISSING;

	/* check if it's in ancestor */
	if (rom_where(r) != ROM_INZIP
	    && a && (result=match_files(a, TEST_MSC, r, m)) != TEST_NOTFOUND) {
	    m->where = rom_where(r);
	    if (result == TEST_USABLE)
		continue;
	}
	
	/* search for matching file in game's zip */
	if (as[0]) {
	    for (j=0; j<tests_count; j++) {
		if ((result=match_files(as[0],
					tests[j], r, m)) != TEST_NOTFOUND) {
		    m->where = ROM_INZIP;
		    if (rom_where(r) != ROM_INZIP
			&& match_quality(m) == QU_OK) {
			m->quality = QU_INZIP;
		    }
		    if (result == TEST_USABLE)
			break;
		}
	    }
	}

	if (rom_where(r) == ROM_INZIP
	    && (m->quality == QU_MISSING || m->quality == QU_HASHERR)
	    && rom_size(r) > 0 && rom_status(r) != STATUS_NODUMP) {
	    /* search for matching file in other games (via db) */
	    if (find_in_romset(r, game_name(g), m) == FIND_EXISTS)
		continue;
	    
	    /* search for matching file in needed */
	    ensure_needed_map();
	    if (find_in_archives(needed_map, r, m) == FIND_EXISTS)
		continue;

	    /* search for matching file in superfluous and update sets */
	    ensure_extra_file_map();
	    if (find_in_archives(extra_file_map, r, m) == FIND_EXISTS)
		continue;
	}
    }

    return ma;
}



static test_result_t
match_files(archive_t *a, test_t t, const rom_t *r, match_t *m)
{
    int i;
    const rom_t *ra;
    test_result_t result;

    m->offset = -1;

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
		    if (m->quality == QU_HASHERR)
			break;
		    
		    m->quality = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    m->quality = QU_OK;
		    result = TEST_USABLE;
		}
	    }
	    m->archive = a;
	    m->index = i;
	    break;

	case TEST_SCI:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(rom_hashes(r)) == 0)
		break;
	    
	    if (rom_compare_sc(r, ra) == 0) {
		if (archive_file_compare_hashes(a, i, rom_hashes(r)) != 0) {
		    if (rom_status(archive_file(a, i)) != STATUS_OK)
			break;
		    if (m->quality == QU_HASHERR)
			break;
		    
		    m->quality = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    m->quality = QU_NAMEERR;
		    result = TEST_USABLE;
		}
	    }
	    m->archive = a;
	    m->index = i;
	    break;

	case TEST_LONG:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(rom_hashes(r)) == 0)
		break;
	    
	    if (rom_compare_n(r, ra) == 0
		&& rom_size(ra) > rom_size(r)
		&& (m->offset=archive_file_find_offset(a, i, rom_size(r),
						       rom_hashes(r))) != -1) {
		m->archive = a;
		m->index = i;
		m->quality = QU_LONG;
		return TEST_USABLE;
	    }
	    break;
	}
    }

    return result;
}
