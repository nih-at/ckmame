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


#include "check.h"

#include "check_util.h"
#include "find.h"
#include "globals.h"
#include "RomDB.h"
#include "warn.h"
#include "CkmameCache.h"

enum test { TEST_NAME_SIZE_CHECKSUM, TEST_MERGENAME_SIZE_CHECKSUM, TEST_SIZE_CHECKSUM, TEST_LONG };

typedef enum test test_t;

enum test_result { TEST_NOTFOUND, TEST_UNUSABLE, TEST_USABLE };

typedef enum test_result test_result_t;

static test_result_t match_files(const ArchivePtr&, test_t, const Game *game, const Rom *, Match *);


void check_game_files(Game *game, filetype_t filetype, GameArchives *archives, Result *res) {
    static std::vector<test_t> tests = {
        TEST_NAME_SIZE_CHECKSUM,
        TEST_SIZE_CHECKSUM,
        TEST_LONG
    };
    
    test_result_t result;
    
    size_t detector_id = filetype == TYPE_ROM ?  db->get_detector_id_for_dat(game->dat_no) : 0;

    auto missing_roms = false;
    
    for (size_t i = 0; i < game->files[filetype].size(); i++) {
        auto &rom = game->files[filetype][i];
        Match *match = &res->game_files[filetype][i];
        auto expected_archive = archives[rom.where].archive[filetype];
        
        if (match->quality == Match::OLD) {
            Match ingame_match;
            if (rom.where == FILE_INGAME && match_files(archives[0].archive[filetype], TEST_NAME_SIZE_CHECKSUM, game, &rom, &ingame_match) == TEST_USABLE) {
                if (ingame_match.quality == Match::OK) {
                    match->quality = Match::OK_AND_OLD;
                    res->game = GameStatus::GS_FIXABLE;
                }
            }
            continue;
        }
        
        match->quality = Match::MISSING;
        
        /* check if it's in ancestor */
        if (rom.where != FILE_INGAME && expected_archive && (result = match_files(expected_archive, TEST_MERGENAME_SIZE_CHECKSUM, game, &rom, match)) != TEST_NOTFOUND) {
            match->where = rom.where;
            if (result == TEST_USABLE) {
                continue;
            }
        }
        
        /* search for matching file in game's zip */
        if (archives[0].archive[filetype]) {
            for (const auto &test : tests) {
                if ((result = match_files(archives[0].archive[filetype], test, game, &rom, match)) != TEST_NOTFOUND) {
                    match->where = FILE_INGAME;
                    if (rom.where != FILE_INGAME && match->quality == Match::OK) {
                        match->quality = Match::IN_ZIP;
                    }
                    if (result == TEST_USABLE) {
                        break;
                    }
                }
            }
        }

        if (rom.where == FILE_INGAME && match->quality == Match::MISSING && rom.hashes.size > 0 && !rom.hashes.empty() && rom.status != Rom::NO_DUMP) {
            if (configuration.complete_games_only && missing_roms) {
                match->quality = Match::UNCHECKED;
                continue;
            }

            /* search for matching file in other games (via db) */
            if (find_in_romset(filetype, detector_id, &rom, nullptr, game->name, "", match) == FIND_EXISTS) {
                continue;
            }

            /* search in needed, superfluous and update sets */
            ckmame_cache->ensure_needed_maps();
	    ckmame_cache->ensure_extra_maps();

            if (find_in_archives(filetype, detector_id, &rom, match, false) == FIND_EXISTS) {
                continue;
            }

            missing_roms = true;
        }
    }
    
    Archive *archive = archives[0][filetype];
    if (archive && !archive->files.empty()) {
        uint64_t user[archive->files.size()];
        for (size_t i = 0; i < archive->files.size(); i++) {
            user[i] = std::numeric_limits<uint64_t>::max();
        }
        
        for (size_t i = 0; i < game->files[filetype].size(); i++) {
            Match *match = &res->game_files[filetype][i];
            if (match->where == FILE_INGAME) {
                size_t j = match->index;
                if (res->archive_files[filetype][j] != FS_USED) {
                    res->archive_files[filetype][j] = match->quality == Match::LONG ? FS_PARTUSED : FS_USED;
                }
                
                if (match->quality != Match::LONG && match->quality != Match::IN_ZIP) {
                    if (user[j] == std::numeric_limits<uint64_t>::max()) {
                        user[j] = i;
                    }
                    else {
                        if (match->quality == Match::OK) {
                            res->game_files[filetype][user[j]].quality = Match::COPIED;
                            user[j] = i;
                        }
                        else {
                            match->quality = Match::COPIED;
                        }
                    }
                }
            }
        }
    }
}


static test_result_t match_files(const ArchivePtr& archive, test_t test, const Game *game, const Rom *rom, Match *match) {
    test_result_t result;

    match->offset = 0;

    result = TEST_NOTFOUND;

    for (size_t i = 0; result != TEST_USABLE && i < archive->files.size(); i++) {
        auto &file = archive->files[i];

	if (file.broken) {
	    continue;
	}

        switch (test) {
            case TEST_NAME_SIZE_CHECKSUM:
            case TEST_MERGENAME_SIZE_CHECKSUM: {
                if ((test == TEST_NAME_SIZE_CHECKSUM ? rom->compare_name(file) : rom->compare_merged(file))) {
                    // TODO: no detectors for disks
                    size_t detector_id = db->get_detector_id_for_dat(game->dat_no);
                    if (archive->compare_size_hashes(i, detector_id, rom)) {
                        match->quality = Match::OK;
                        result = TEST_USABLE;
                        match->archive = archive;
                        match->index = i;
                    }
                }
                break;
            }
                
            case TEST_SIZE_CHECKSUM: {
                /* roms without hashes are only matched with correct name */
                if (rom->hashes.empty()) {
                    break;
                }
                
                // TODO: no detectors for disks
                size_t detector_id = db->get_detector_id_for_dat(game->dat_no);
                if (archive->compare_size_hashes(i, detector_id, rom)) {
                    match->quality = Match::NAME_ERROR;
                    result = TEST_USABLE;
                    match->archive = archive;
                    match->index = i;
                }
                break;
            }
                
            case TEST_LONG:
                /* roms without hashes are only matched with correct name */
                if (rom->hashes.empty() || rom->hashes.size == 0) {
                    break;
                }
                
                if (rom->compare_name(file) && file.hashes.size > rom->hashes.size) {
                    auto offset = archive->file_find_offset(i, rom->hashes.size, &rom->hashes);
                    if (offset.has_value()) {
                        match->offset = offset.value();
                        match->archive = archive;
                        match->index = i;
                        match->quality = Match::LONG;
                        return TEST_USABLE;
                    }
                }
                break;
        }
    }

    return result;
}


void update_game_status(const Game *game, Result *result) {
    bool all_dead, all_own_dead, all_correct, all_fixable;

    all_own_dead = all_dead = all_correct = all_fixable = true;
    auto has_own = false;

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        auto filetype = static_cast<filetype_t>(ft);

        for (size_t i = 0; i < game->files[filetype].size(); i++) {
            auto match = &result->game_files[filetype][i];
            auto &rom = game->files[filetype][i];

            if (rom.where == FILE_INGAME) {
                has_own = true;
            }
            if (match->quality == Match::MISSING) {
                all_fixable = false;
            }
            else {
                all_dead = false;
                if (rom.where == FILE_INGAME) {
                    all_own_dead = false;
                }
            }
	    if (match->quality != Match::OK && (rom.status == Rom::OK || configuration.report_no_good_dump)) {
                all_correct = false;
            }
        }
    }
        
    if (all_correct) {
        result->game = GS_CORRECT;
    }
    else if (all_dead || (has_own && all_own_dead)) {
	result->game = GS_MISSING;
    }
    else if (all_fixable) {
	result->game = GS_FIXABLE;
    }
    else {
	result->game = GS_PARTIAL;
    }
}
