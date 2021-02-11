/*
  find.c -- find ROM in ROM set or archives
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

#include "find.h"

#include "check_util.h"
#include "dbh.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "memdb.h"
#include "sq_util.h"

#define QUERY_FILE "select game_id, file_idx, file_sh, location from file where file_type = ?"
#define QUERY_FILE_HASH_FMT " and (%s = ? or %s is null)"
#define QUERY_FILE_TAIL " order by location"
#define QUERY_FILE_GAME_ID 0
#define QUERY_FILE_FILE_IDX 1
#define QUERY_FILE_FILE_SH 2
#define QUERY_FILE_LOCATION 3


static find_result_t check_for_file_in_archive(filetype_t filetype, const std::string &game_name, const File *rom, const File *file, Match *m);
static find_result_t check_match_old(filetype_t filetype, const Game *game, const File *wanted_file, const File *candidate, Match *match);
static find_result_t check_match_romset(filetype_t filetype, const Game *game, const File *wanted_file, const File *candidate, Match *match);
static find_result_t find_in_db(RomDB *rdb, filetype_t filetype, const File *wanted_file, Archive *archive, const std::string &skip, Match *match, find_result_t (*)(filetype_t filetype, const Game *game, const File *wanted_file, const File *candidate, Match *match));


#if 0
find_result_t
find_disk(const Disk *disk, MatchDisk *match_disk) {
    sqlite3_stmt *stmt;
    int ret;
    
    if (memdb == NULL) {
        return FIND_UNKNOWN;
    }

    if ((stmt = memdb->get_statement(DBH_STMT_MEM_QUERY_FILE, &disk->hashes, 0)) == NULL)
	return FIND_ERROR;

    if (sqlite3_bind_int(stmt, 1, TYPE_DISK) != SQLITE_OK || sq3_set_hashes(stmt, 2, &disk->hashes, 0) != SQLITE_OK) {
	return FIND_ERROR;
    }

    switch (sqlite3_step(stmt)) {
        case SQLITE_ROW: {
            auto dm = Disk::by_id(sqlite3_column_int(stmt, QUERY_FILE_GAME_ID));
            if (!dm) {
                ret = FIND_ERROR;
                break;
            }

            if (match_disk) {
                match_disk->name = dm->name;
                match_disk->hashes = dm->hashes;
                match_disk->quality = QU_COPIED;
                match_disk->where = static_cast<where_t>(sqlite3_column_int(stmt, QUERY_FILE_LOCATION));
            }
            ret = FIND_EXISTS;
            break;
        }
            
    case SQLITE_DONE:
	ret = FIND_UNKNOWN;
	break;

    default:
	ret = FIND_ERROR;
	break;
    }

    return static_cast<find_result_t>(ret);
}
#endif

find_result_t
find_in_archives(const File *rom, Match *m, bool needed_only) {
    sqlite3_stmt *stmt;
    int i, ret, hcol, sh;

    if (memdb_ensure() < 0) {
        return FIND_ERROR;
    }
    
    if ((stmt = memdb->get_statement(DBH_STMT_MEM_QUERY_FILE, &rom->hashes, rom->size != SIZE_UNKNOWN)) == NULL) {
	return FIND_ERROR;
    }

    hcol = 2;
    if (rom->size != SIZE_UNKNOWN) {
	hcol++;
        if (sqlite3_bind_int64(stmt, 2, rom->size) != SQLITE_OK) {
	    return FIND_ERROR;
        }
    }

    if (sqlite3_bind_int(stmt, 1, TYPE_ROM) != SQLITE_OK || sq3_set_hashes(stmt, hcol, &rom->hashes, 0) != SQLITE_OK) {
	return FIND_ERROR;
    }


    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto a = Archive::by_id(sqlite3_column_int(stmt, QUERY_FILE_GAME_ID));
        if (!a) {
	    ret = SQLITE_ERROR;
	    break;
        }
	if (needed_only && a->where != FILE_NEEDED) {
	    continue;
	}

	i = sqlite3_column_int(stmt, QUERY_FILE_FILE_IDX);
	sh = sqlite3_column_int(stmt, QUERY_FILE_FILE_SH);
        auto &file = a->files[i];

        if (sh == 0 && (rom->hashes.types & file.hashes.types) != rom->hashes.types) {
            a->file_compute_hashes(i, rom->hashes.types | db->hashtypes(TYPE_ROM));
            memdb_update_file(a->contents.get(), i);
	}

	if (file.status != STATUS_OK || file.get_hashes(sh != 0).compare(rom->hashes) != Hashes::MATCH) {
	    continue;
	}

	if (m) {
            m->archive = a;
            m->index = i;
            m->where = static_cast<where_t>(sqlite3_column_int(stmt, QUERY_FILE_LOCATION));
	    m->quality = QU_COPIED;
	}

	return FIND_EXISTS;
    }

    return ret == SQLITE_DONE ? FIND_UNKNOWN : FIND_ERROR;
}


find_result_t find_in_old(filetype_t filetype, const File *file, Archive *archive, Match *match) {
    if (old_db == NULL) {
	return FIND_MISSING;
    }

    return find_in_db(old_db.get(), filetype, file, archive, "", match, check_match_old);
}


find_result_t find_in_romset(filetype_t filetype, const File *file, Archive *archive, const std::string &skip, Match *match) {
    return find_in_db(db.get(), filetype, file, archive, skip, match, check_match_romset);
}


static find_result_t check_for_file_in_archive(filetype_t filetype, const std::string &name, const File *wanted_file, const File *candidate, Match *matches) {
    ArchivePtr a;

    auto full_name = findfile(name, TYPE_ROM, "");
    if (full_name == "" || !(a = Archive::open(full_name, TYPE_ROM, FILE_ROMSET, 0))) {
	return FIND_MISSING;
    }

    auto idx = a->file_index_by_name(wanted_file->name);
    
    if (idx.has_value() && a->file_compare_hashes(idx.value(), &candidate->hashes) == Hashes::MATCH) {
        auto index = idx.value();
        File *af = &a->files[index];
        
        if (!af->hashes.has_all_types(candidate->hashes)) {
            if (!a->file_compute_hashes(index, Hashes::TYPE_ALL)) { /* TODO: only needed hash types */
                return FIND_MISSING;
	    }
	    if (a->file_compare_hashes(index, &candidate->hashes) != Hashes::MATCH) {
		return FIND_MISSING;
	    }
	}
	if (matches) {
            matches->archive = a;
            matches->index = index;
	}
	return FIND_EXISTS;
    }

    return FIND_MISSING;
}


#if 0
/*ARGSUSED1*/
static find_result_t check_match_disk_romset(const Game *game, const Disk *disk, MatchDisk *match_disk) {
    auto file_name = findfile(disk->name, TYPE_DISK, game->name);
    if (file_name == "") {
	return FIND_MISSING;
    }

    auto f = Disk::from_file(file_name, DISK_FL_QUIET);
    if (!f) {
	return FIND_MISSING;
    }

    if (f->status == STATUS_OK && disk->hashes.compare(f->hashes) == Hashes::MATCH) {
	if (match_disk) {
	    match_disk->quality = QU_COPIED;
	    match_disk->name = file_name;
            match_disk->hashes = f->hashes;
	}
        printf("found disk '%s': %d\n", file_name.c_str(), f->status);
	return FIND_EXISTS;
    }

    return FIND_MISSING;
}
#endif

static find_result_t check_match_old(filetype_t, const Game *game, const File *wanted_file, const File *, Match *match) {
    if (match) {
	match->quality = QU_OLD;
	match->where = FILE_OLD;
        match->old_game = game->name;
        match->old_file = wanted_file->name;
    }

    return FIND_EXISTS;
}


static find_result_t check_match_romset(filetype_t filetype, const Game *game, const File *wanted_file, const File *candidate, Match *match) {
    auto status = check_for_file_in_archive(filetype, game->name, wanted_file, candidate, match);
    if (match && status == FIND_EXISTS) {
	match->quality = QU_COPIED;
	match->where = FILE_ROMSET;
    }

    return status;
}


static find_result_t find_in_db(RomDB *rdb, filetype_t filetype, const File *file, Archive *archive, const std::string &skip, Match *match, find_result_t (*check_match)(filetype_t filetype, const Game *game, const File *wanted_file, const File *candidate, Match *match)) {
    auto roms = rdb->read_file_by_hash(filetype, &file->hashes);

    if (roms.empty()) {
	return FIND_UNKNOWN;
    }

    find_result_t status = FIND_UNKNOWN;
    for (size_t i = 0; (status != FIND_ERROR && status != FIND_EXISTS) && i < roms.size(); i++) {
	auto rom = roms[i];

        if (rom.name == skip) {
	    continue;
        }

        GamePtr game = rdb->read_game(rom.name);
        if (!game || game->files[filetype].size() <= rom.index) {
	    /* TODO: internal error: database inconsistency */
	    status = FIND_ERROR;
	    break;
	}

        auto &game_rom = game->files[filetype][rom.index];

        if (file->size == game_rom.size && file->hashes.compare(game_rom.hashes) == Hashes::MATCH) {
	    bool ok = true;

            if (archive && !file->hashes.has_all_types(game_rom.hashes)) {
		auto idx = archive->file_index(file);
                if (idx.has_value()) {
                    if (!archive->file_compute_hashes(idx.value(), game_rom.hashes.types)) {
			/* TODO: handle error (how?) */
			ok = false;
		    }
		}

                if (game_rom.hashes.compare(file->hashes) != Hashes::MATCH) {
		    ok = false;
		}
	    }

	    if (ok) {
                status = check_match(filetype, game.get(), &game_rom, file, match);
	    }
	}
    }

    return status;
}
