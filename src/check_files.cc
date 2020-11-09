/*
  check_files.c -- match files against ROMs
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

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
#include "xmalloc.h"


enum test { TEST_NAME_SIZE_CHECKSUM, TEST_MERGENAME_SIZE_CHECKSUM, TEST_SIZE_CHECKSUM, TEST_LONG };

typedef enum test test_t;

enum test_result { TEST_NOTFOUND, TEST_UNUSABLE, TEST_USABLE };

typedef enum test_result test_result_t;

static test_result_t match_files(Archive *, test_t, const file_t *, match_t *);
static void update_game_status(const game_t *, result_t *);


void
check_files(game_t *g, ArchivePtr archives[3], result_t *res) {
    static const test_t tests[] = {TEST_NAME_SIZE_CHECKSUM, TEST_SIZE_CHECKSUM, TEST_LONG};
    static const int tests_count = sizeof(tests) / sizeof(tests[0]);
    
    int i, j;
    test_result_t result;
    
    if (result_game(res) == GS_OLD)
        return;
    
    for (i = 0; i < game_num_roms(g); i++) {
        file_t *rom = game_rom(g, i);
        match_t *match = result_rom(res, i);
        Archive *expected_archive = archives[file_where(rom)].get();
        
        if (match_quality(match) == QU_OLD) {
            continue;
        }
        
        match_quality(match) = QU_MISSING;
        
        /* check if it's in ancestor */
        if (file_where(rom) != FILE_INGAME && expected_archive != NULL && (result = match_files(expected_archive, TEST_MERGENAME_SIZE_CHECKSUM, rom, match)) != TEST_NOTFOUND) {
            match_where(match) = file_where(rom);
            if (result == TEST_USABLE) {
                continue;
            }
        }
        
        /* search for matching file in game's zip */
        if (archives[0]) {
            for (j = 0; j < tests_count; j++) {
                if ((result = match_files(archives[0].get(), tests[j], rom, match)) != TEST_NOTFOUND) {
                    match_where(match) = FILE_INGAME;
                    if (file_where(rom) != FILE_INGAME && match_quality(match) == QU_OK) {
                        match_quality(match) = QU_INZIP;
                    }
                    if (result == TEST_USABLE) {
                        break;
                    }
                }
            }
        }
        
        if (file_where(rom) == FILE_INGAME && (match_quality(match) == QU_MISSING || match_quality(match) == QU_HASHERR) && file_size_(rom) > 0 && file_status_(rom) != STATUS_NODUMP) {
            /* search for matching file in other games (via db) */
            if (find_in_romset(rom, NULL, game_name(g), match) == FIND_EXISTS) {
                continue;
            }
            
            /* search in needed, superfluous and update sets */
            ensure_needed_maps();
            ensure_extra_maps(DO_MAP);
            if (find_in_archives(rom, match, false) == FIND_EXISTS) {
                continue;
            }
        }
    }
    
    Archive *archive = archives[0].get();
    if (archive && !archive->files.empty()) {
        int user[archive->files.size()];
        for (size_t i = 0; i < archive->files.size(); i++) {
            user[i] = -1;
        }
        
        for (i = 0; i < game_num_roms(g); i++) {
            match_t *match = result_rom(res, i);
            if (match_where(match) == FILE_INGAME && match_quality(match) != QU_HASHERR) {
                j = match_index(match);
                if (result_file(res, j) != FS_USED) {
                    result_file(res, j) = match_quality(match) == QU_LONG ? FS_PARTUSED : FS_USED;
                }
                
                if (match_quality(match) != QU_LONG && match_quality(match) != QU_INZIP) {
                    if (user[j] == -1) {
                        user[j] = i;
                    }
                    else {
                        if (match_quality(match) == QU_OK) {
                            match_quality(result_rom(res, user[j])) = QU_COPIED;
                            user[j] = i;
                        }
                        else {
                            match_quality(match) = QU_COPIED;
                        }
                    }
                }
            }
        }
    }
    
    update_game_status(g, res);
    
    stats_add_game(stats, result_game(res));
    for (i = 0; i < game_num_roms(g); i++) {
        stats_add_rom(stats, TYPE_ROM, game_rom(g, i), match_quality(result_rom(res, i)));
    }

}


static test_result_t
match_files(Archive *archive, test_t test, const file_t *rom, match_t *match) {
    const file_t *file;
    test_result_t result;

    match_offset(match) = -1;

    result = TEST_NOTFOUND;

    for (size_t i = 0; result != TEST_USABLE && i < archive->files.size(); i++) {
        file = &archive->files[i];

	if (file_status_(file) != STATUS_OK) {
	    continue;
	}

	switch (test) {
	case TEST_NAME_SIZE_CHECKSUM:
	case TEST_MERGENAME_SIZE_CHECKSUM:
	    if ((test == TEST_NAME_SIZE_CHECKSUM ? (file_compare_n(rom, file)) : (file_compare_m(rom, file))) && file_compare_sc(rom, file)) {
		/* TODO: this is exceedingly ugly */
		if ((hashes_cmp(file_hashes(rom), file_hashes(file)) != HASHES_CMP_MATCH) && (!file_sh_is_set(file, FILE_SH_DETECTOR) || (hashes_cmp(file_hashes(rom), file_hashes_xxx(file, FILE_SH_DETECTOR)) != HASHES_CMP_MATCH))) {
		    if (match_quality(match) == QU_HASHERR)
			break;

		    match_quality(match) = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    match_quality(match) = QU_OK;
		    result = TEST_USABLE;
		}
		match_archive(match) = archive;
		match_index(match) = i;
	    }
	    break;

	case TEST_SIZE_CHECKSUM:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(file_hashes(rom)) == 0)
		break;

	    if (file_compare_sc(rom, file)) {
		if (archive->file_compare_hashes(i, file_hashes(rom)) != 0) {
		    if (file_status_(&archive->files[i]) != STATUS_OK)
			break;
		    if (match_quality(match) == QU_HASHERR)
			break;

		    match_quality(match) = QU_HASHERR;
		    result = TEST_UNUSABLE;
		}
		else {
		    match_quality(match) = QU_NAMEERR;
		    result = TEST_USABLE;
		}
	    }
	    match_archive(match) = archive;
	    match_index(match) = i;
	    break;

	case TEST_LONG:
	    /* roms without hashes are only matched with correct name */
	    if (hashes_types(file_hashes(rom)) == 0 || file_size_(rom) == 0)
		break;

                if (file_compare_n(rom, file) && file_size_(file) > file_size_(rom)) {
                    auto offset = archive->file_find_offset(i, file_size_(rom), file_hashes(rom));
                    if (offset.has_value()) {
                        match_offset(match) = offset.value();
                        match_archive(match) = archive;
                        match_index(match) = i;
                        match_quality(match) = QU_LONG;
                        return TEST_USABLE;
                    }
                }
	    break;
	}
    }

    return result;
}


static void
update_game_status(const game_t *g, result_t *res) {
    int i;
    int all_dead, all_own_dead, all_correct, all_fixable, has_own;
    const match_t *m;
    const file_t *r;

    all_own_dead = all_dead = all_correct = all_fixable = 1;
    has_own = 0;

    for (i = 0; i < game_num_roms(g); i++) {
	m = result_rom(res, i);
	r = game_rom(g, i);

	if (file_where(r) == FILE_INGAME)
	    has_own = 1;
	if (match_quality(m) == QU_MISSING)
	    all_fixable = 0;
	else {
	    all_dead = 0;
	    if (file_where(r) == FILE_INGAME)
		all_own_dead = 0;
	}
	/* TODO: using output_options here is a bit of a hack,
	   but so is all of the result_game processing */
	if (match_quality(m) != QU_OK && (file_status_(r) != STATUS_NODUMP || (output_options & WARN_NO_GOOD_DUMP)))
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
