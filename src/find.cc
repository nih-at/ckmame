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
#include "DeleteList.h"
#include "MemDB.h"
#include "RomDB.h"

static find_result_t check_for_file_in_archive(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *rom, const FileData *file, Match *m);
static find_result_t check_match_old(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *candidate, Match *match);
static find_result_t check_match_romset(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *candidate, Match *match);
static find_result_t find_in_db(RomDB *rdb, filetype_t filetype, size_t detector_id, const FileData *wanted_file, Archive *archive, const std::string &skip_game, const std::string &skip_file, Match *match, find_result_t (*)(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *candidate, Match *match));

static find_result_t find_in_archives_xxx(filetype_t filetype, size_t detector_id, const FileData *r, Match *m, bool needed_only);

static bool compute_all_detector_hashes(DeleteListPtr list);

bool compute_all_detector_hashes(bool needed_only) { // returns true if new hashes were computed
    if (db->detectors.empty()) {
        return false;
    }
    
    auto got_new_hashes = false;
    
    if (compute_all_detector_hashes(needed_delete_list)) {
        got_new_hashes = true;
    }

    if (!needed_only) {
        if (compute_all_detector_hashes(superfluous_delete_list)) {
            got_new_hashes = true;
        }
        if (compute_all_detector_hashes(extra_delete_list)) {
            got_new_hashes = true;
        }
    }
    
    return got_new_hashes;
}

static bool compute_all_detector_hashes(DeleteListPtr list) {
    auto got_new_hashes = false;
    for (auto entry : list->archives) {
        auto contents = ArchiveContents::by_name(entry.filetype, entry.name);
        if (contents == NULL || contents->has_all_detector_hashes(db->detectors)) {
            continue;
        }
        auto archive = Archive::open(contents);
        if (archive->compute_detector_hashes(db->detectors)) {
            got_new_hashes = true;
        }
    }
    return got_new_hashes;
}


find_result_t find_in_archives(filetype_t filetype, size_t detector_id, const FileData *rom, Match *m, bool needed_only) {
    auto result = find_in_archives_xxx(filetype, detector_id, rom, m, needed_only);
    if (result == FIND_UNKNOWN) {
        if (compute_all_detector_hashes(needed_only)) {
            result = find_in_archives_xxx(filetype, detector_id, rom, m, needed_only);
        }
    }
    
    return result;
}


static find_result_t find_in_archives_xxx(filetype_t filetype, size_t detector_id, const FileData *rom, Match *m, bool needed_only) {
    auto results = memdb->find(filetype, rom); // TODO: catch error, return FIND_ERROR
    
    for (auto result : results) {
        if (result.detector_id != 0 && result.detector_id != detector_id) {
            continue;
        }

        auto a = Archive::by_id(result.archive_id);
        if (!a) {
            return FIND_ERROR;
        }
	if (needed_only && a->where != FILE_NEEDED) {
	    continue;
	}

        auto &file = a->files[result.index];

        if (result.detector_id == 0 && !file.hashes.has_all_types(rom->hashes)) {
            a->file_ensure_hashes(result.index, rom->hashes.get_types() | db->hashtypes(filetype));
            memdb->update_file(a->contents.get(), result.index);
	}

	if (file.broken || file.get_hashes(result.detector_id).compare(rom->hashes) != Hashes::MATCH) {
	    continue;
	}

	if (m) {
            m->archive = a;
            m->index = result.index;
            m->where = result.location;
	    m->quality = Match::COPIED;
	}

	return FIND_EXISTS;
    }

    return FIND_UNKNOWN;
}


find_result_t find_in_old(filetype_t filetype, const FileData *file, Archive *archive, Match *match) {
    if (old_db == NULL) {
	return FIND_MISSING;
    }

    return find_in_db(old_db.get(), filetype, 0, file, archive, "", "", match, check_match_old);
}


find_result_t find_in_romset(filetype_t filetype, size_t detector_id, const FileData *file, Archive *archive, const std::string &skip_game, const std::string &skip_file, Match *match) {
    return find_in_db(db.get(), filetype, detector_id, file, archive, skip_game, skip_file, match, check_match_romset);
}


static find_result_t check_for_file_in_archive(filetype_t filetype, size_t detector_id, const std::string &name, const FileData *wanted_file, const FileData *candidate, Match *matches) {
    ArchivePtr a;

    auto full_name = findfile(filetype, name);
    if (full_name.empty() || !(a = Archive::open(full_name, filetype, FILE_ROMSET, 0))) {
	return FIND_MISSING;
    }

    auto idx = a->file_index_by_name(candidate->name);
    if (idx.has_value()) {
        a->file_ensure_hashes(idx.value(), wanted_file->hashes.get_types());
        if (a->compare_size_hashes(idx.value(), detector_id, wanted_file)) {
            auto index = idx.value();
            
            if (!a->files[index].broken) {
                if (matches) {
                    matches->archive = a;
                    matches->index = index;
                }
                return FIND_EXISTS;
            }
        }
    }

    return FIND_MISSING;
}


static find_result_t check_match_old(filetype_t, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *, Match *match) {
    if (match) {
	match->quality = Match::OLD;
	match->where = FILE_OLD;
        match->old_game = game_name;
        match->old_file = wanted_file->name;
    }

    return FIND_EXISTS;
}


static find_result_t check_match_romset(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *candidate, Match *match) {
    auto status = check_for_file_in_archive(filetype, detector_id, game_name, wanted_file, candidate, match);
    if (match && status == FIND_EXISTS) {
	match->quality = Match::COPIED;
	match->where = FILE_ROMSET;
    }

    return status;
}


static find_result_t find_in_db(RomDB *rdb, filetype_t filetype, size_t detector_id, const FileData *file, Archive *archive, const std::string &skip_game, const std::string &skip_file, Match *match, find_result_t (*check_match)(filetype_t filetype, size_t detector_id, const std::string &game_name, const FileData *wanted_file, const FileData *candidate, Match *match)) {
    auto locations = rdb->read_file_by_hash(filetype, file->hashes);
    
    if (locations.empty()) {
	return FIND_UNKNOWN;
    }

    find_result_t status = FIND_UNKNOWN;
    for (size_t i = 0; (status != FIND_ERROR && status != FIND_EXISTS) && i < locations.size(); i++) {
	auto &location = locations[i];

        if (location.game_name == skip_game && skip_file.empty()) {
	    continue;
        }

        // This is an optimization for ROM sets with many games that share many files, like ScummVM.
        if (check_match == check_match_romset && archive == NULL && findfile(filetype, location.game_name).empty()) {
            status = FIND_MISSING;
            continue;
        }
        
        auto &game_rom = location.rom;
        
        if (location.game_name == skip_game && game_rom.name == skip_file) {
            continue;
        }

        if (file->compare_size_hashes(game_rom)) {
	    bool ok = true;

            if (archive && !file->hashes.has_all_types(game_rom.hashes)) {
		auto idx = archive->file_index(file);
                if (idx.has_value()) {
                    if (!archive->file_ensure_hashes(idx.value(), game_rom.hashes.get_types())) {
			/* TODO: handle error (how?) */
			ok = false;
		    }
		}

                if (game_rom.hashes.compare(file->hashes) != Hashes::MATCH) {
		    ok = false;
		}
	    }

	    if (ok) {
                status = check_match(filetype, detector_id, location.game_name, file, &game_rom, match);
	    }
	}
    }

    return status;
}
