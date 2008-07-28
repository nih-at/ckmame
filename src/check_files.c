/*
  check_files.c -- match files against ROMs
  Copyright (C) 2005-2008 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "archive.h"
#include "dbh.h"
#include "file.h"
#include "file_location.h"
#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "match.h"
#include "util.h"
#include "warn.h"



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

static test_result_t match_files(archive_t *, test_t, const file_t *,
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
    file_t *r;
    match_t *m;
    archive_t *a;
    test_result_t result;
    int *user;

    if (result_game(res) == GS_OLD)
	return;

    for (i=0; i<game_num_files(g, file_type); i++) {
	r = game_file(g, file_type, i);
	m = result_rom(res, i);
	a = as[file_where(r)];

	if (match_quality(m) == QU_OLD)
	    continue;

	match_quality(m) = QU_MISSING;

	/* check if it's in ancestor */
	if (file_where(r) != FILE_INZIP
	    && a && (result=match_files(a, TEST_MSC, r, m)) != TEST_NOTFOUND) {
	    match_where(m) = file_where(r);
	    if (result == TEST_USABLE)
		continue;
	}
	
	/* search for matching file in game's zip */
	if (as[0]) {
	    for (j=0; j<tests_count; j++) {
		if ((result=match_files(as[0],
					tests[j], r, m)) != TEST_NOTFOUND) {
		    match_where(m) = FILE_INZIP;
		    if (file_where(r) != FILE_INZIP
			&& match_quality(m) == QU_OK) {
			match_quality(m) = QU_INZIP;
		    }
		    if (result == TEST_USABLE)
			break;
		}
	    }
	}

	if (file_where(r) == FILE_INZIP
	    && (match_quality(m) == QU_MISSING
		|| match_quality(m) == QU_HASHERR)
	    && file_size(r) > 0 && file_status(r) != STATUS_NODUMP) {
	    /* search for matching file in other games (via db) */
	    if (find_in_romset(r, game_name(g), m) == FIND_EXISTS)
		continue;
	    
	    /* search in needed, superfluous and update sets */
	    ensure_needed_maps();
	    ensure_extra_maps(DO_MAP);
	    if (find_in_archives(r, m) == FIND_EXISTS)
		continue;
	}
    }

    if (as[0] && archive_num_files(as[0]) > 0) {
	user = malloc(sizeof(int) * archive_num_files(as[0]));
	for (i=0; i<archive_num_files(as[0]); i++)
	    user[i] = -1;
    
	for (i=0; i<game_num_files(g, file_type); i++) {
	    m = result_rom(res, i);
	    if (match_where(m) == FILE_INZIP
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
match_files(archive_t *a, test_t t, const file_t *r, match_t *m)
{
    int i;
    const file_t *ra;
    test_result_t result;

    match_offset(m) = -1;

    result = TEST_NOTFOUND;
	
    for (i=0; result != TEST_USABLE && i<archive_num_files(a); i++) {
	ra = archive_file(a, i);

	if (file_status(ra) != STATUS_OK)
	    continue;

	switch (t) {
	case TEST_NSC:
	case TEST_MSC:
	    if ((t == TEST_NSC
		 ? (file_compare_n(r, ra))
		 : (file_compare_m(r, ra)))
		&& file_compare_sc(r, ra)) {
		/* XXX: this is exceedingly ugly */
		if ((hashes_cmp(file_hashes(r), file_hashes(ra))
		     != HASHES_CMP_MATCH)
		    && (!file_sh_is_set(ra, FILE_SH_DETECTOR)
			|| (hashes_cmp(file_hashes(r),
				      file_hashes_xxx(ra, FILE_SH_DETECTOR))
			    != HASHES_CMP_MATCH))) {
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
	    if (hashes_types(file_hashes(r)) == 0)
		break;
	    
	    if (file_compare_sc(r, ra)) {
		if (archive_file_compare_hashes(a, i, file_hashes(r)) != 0) {
		    if (file_status(archive_file(a, i)) != STATUS_OK)
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
	    if (hashes_types(file_hashes(r)) == 0)
		break;
	    
	    if (file_compare_n(r, ra)
		&& file_size(ra) > file_size(r)
		&& (match_offset(m)=archive_file_find_offset(a, i, file_size(r),
							     file_hashes(r)))
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
    const file_t *r;

    all_own_dead = all_dead = all_correct = all_fixable = 1;
    has_own = 0;

    for (i=0; i<game_num_files(g, file_type); i++) {
	m = result_rom(res, i);
	r = game_file(g, file_type, i);

	if (file_where(r) == FILE_INZIP)
	    has_own = 1;
	if (match_quality(m) == QU_MISSING)
	    all_fixable = 0;
	else {
	    all_dead = 0;
	    if (file_where(r) == FILE_INZIP)
		all_own_dead = 0;
	}
	/* XXX: using output_options here is a bit of a hack,
	   but so is all of the result_game processing */
	if (match_quality(m) != QU_OK
	    && (file_status(r) == STATUS_OK
		|| (output_options & WARN_NO_GOOD_DUMP)))
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
