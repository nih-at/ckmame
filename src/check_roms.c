/*
  $NiH: check_roms.c,v 1.1 2005/07/13 17:42:19 dillo Exp $

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

enum quality {
    QU_MISSING,		/* ROM is missing */
    QU_LONG,		/* long ROM with valid subsection */
    QU_NAMEERR,		/* wrong name */
    QU_COPIED,		/* copied from elsewhere */
    QU_INZIP,		/* is in zip, should be in ancestor */
    QU_OK,		/* name/size/crc match */
    QU_ANCESTOR_OK	/* ancestor ROM found in ancestor */
};

#define match new_match
#define match_t new_match_t

typedef enum quality quality_t;

struct match {
    quality_t quality;
    int index;
    off_t offset;
};

typedef struct match match_t;

enum test {
    TEST_NSC,
    TEST_SCI,
    TEST_LONG
};

typedef enum test test_t;

int rom_compare_sc(const rom_t *, const rom_t *);



int
rom_compare_n(const rom_t *r1, const rom_t *r2)
{
    return strcasecmp(rom_name(r1), rom_name(r2));
}



int
rom_compare_nsc(const rom_t *r1, const rom_t *r2)
{
    if (rom_compare_n(r1, r2) == 0
	&& rom_compare_sc(r1, r2) == 0)
	return 0;

    return 1;
}



int
rom_compare_sc(const rom_t *r1, const rom_t *r2)
{
    if (rom_size(r1) == rom_size(r2)
	&& hashes_cmp(rom_hashes(r1), rom_hashes(r2)) == 0)
	return 0;

    return 1;
}


int
archive_file_compare_hashes(const archive_t *a, int i, const hashes_t *h)
{
    return hashes_cmp(rom_hashes(archive_file(a, i)), h);
}



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
