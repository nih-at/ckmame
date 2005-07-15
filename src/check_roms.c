/*
  $NiH: check_roms.c,v 1.1.2.1 2005/07/14 15:16:18 wiz Exp $

  check_roms.c -- match files against ROMs
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



#include "archive.h"
#include "game.h"
#include "hashes.h"
#include "match.h"
#include "rom.h"



/* XXX: move elsewhere */
int
archive_file_compare_hashes(archive_t *a, int i, const hashes_t *h)
{
    hashes_t *rh;

    rh = rom_hashes(archive_file(a, i));

    if ((hashes_types(rh) & hashes_types(h)) != hashes_types(h))
	archive_file_compute_hashes(a, i, hashes_types(h)|romhashtypes);
    
    return hashes_cmp(rh, h);
}



/* XXX: move elsewhere */
int
archive_find(archive_t *a, test_t t, const rom_t *r, match_t *m)
{
    int i;
    const rom_t *ra;

    m->offset = -1;

    for (i=0; i<archive_num_files(a); i++) {
	ra = archive_file(a, i);

	switch (t) {
	case TEST_NSC:
	    if (rom_compare_nsc(r, ra) == 0) {
		m->index = i;
		return 0;
	    }
	    break;

	case TEST_SCI:
	    if (rom_compare_sc(r, ra) == 0
		&& archive_file_compare_hashes(a, i, rom_hashes(r)) == 0) {
		m->index = i;
		return 0;
	    }
	    break;

	case TEST_LONG:
	    if (rom_compare_n(r, ra) == 0
		&& (m->offset=archive_find_offset(a, i, rom_size(r),
						  rom_hashes(r))) != -1) {
		m->index = i;
		return 0;
	    }
	    break;
	}
    }

    return 1;
}



struct test_result {
    test_t test;
    quality_t quality;
};

typedef struct test_result test_result_t;

test_result_t test_results[] = {
    { TEST_NSC, QU_OK },
    { TEST_SCI, QU_NAMEERR },
    { TEST_LONG, QU_LONG },
};

static int test_result_count = sizeof(test_results)/sizeof(test_results[0]);

int
check_roms(game_t *g, filetype_t ft, archive_t a[3])
{
    match_array_t *ma;
    int i, j;
    rom_t *r;
    match_t *m;

    ma = match_array_new(game_num_files(g, ft));

    for (i=0; i<game_num_files(g, ft); i++) {
	r = game_file(g, ft, i);
	/* XXX: one argument missing */
	m = match_array_get(ma, i);

	m->quality = QU_MISSING;

	if (rom_where(r) != ROM_INZIP
	    && archive_find(&a[rom_where(r)], TEST_NSC, r, m) == 0)
	    m->quality = QU_ANCESTOR_OK;
	else {
	    for (j=0; j<test_result_count; j++)
		if (archive_find(&a[0], test_results[j].test, r, m) == 0) {
		    m->quality = test_results[j].quality;
		    break;
		}
	}
	if (rom_where(r) == ROM_INZIP && m->quality == QU_MISSING) {
	    /* XXX: find elsewhere */
	}
    }
}
