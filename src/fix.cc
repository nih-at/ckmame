/*
  fix.cc -- fix ROM sets
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include "fix.h"
#include "globals.h"
#include "compat.h"

#include <cinttypes>

#include "DeleteList.h"
#include "fix_util.h"
#include "Garbage.h"
#include "Tree.h"
#include "CkmameCache.h"
#include "warn.h"

// Fix archive in ROM set as best we can.
static int fix_files(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage);

// Clear and remove incomplete archive in ROM set, keeping needed files in needed/.
static int clear_incomplete(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage);


int fix_game(Game *game, const GameArchives archives, Result *result) {
    int ret = 0;

    for (auto filetype: db->filetypes()) {
        Archive *archive = archives[filetype];
        GarbagePtr garbage;
        DeleteList::Mark extra_mark, needed_mark, superfluous_mark;

        if (configuration.fix_romset) {
            garbage = std::make_shared<Garbage>(archive);

            extra_mark = DeleteList::Mark(ckmame_cache->extra_delete_list);
            needed_mark = DeleteList::Mark(ckmame_cache->needed_delete_list);
            superfluous_mark = DeleteList::Mark(ckmame_cache->superfluous_delete_list);
        }

        for (uint64_t i = 0; i < archive->files.size(); i++) {
            switch (result->archive_files[filetype][i]) {
                case FS_UNKNOWN: {
                    if (configuration.delete_unknown_pattern.length() > 0 &&
                        fnmatch(configuration.delete_unknown_pattern.c_str(), archive->files[i].filename().c_str(), 0) == 0) {
                        output.message_verbose("delete unknown file '%s' (matching delete-unknown-pattern)", archive->files[i].filename().c_str());

                        /* TODO: handle error (how?) */
                        archive->file_delete(i);
                    } else {
                        output.message_verbose("move unknown file '%s'", archive->files[i].filename().c_str());

                        if (configuration.fix_romset) {
                            garbage->add(i, false); /* TODO: check return value */
                        }
                    }
                    break;
                }

                case FS_DUPLICATE:
                    if (!configuration.keep_old_duplicate && archive->is_writable()) {
			output.message_verbose("delete duplicate file '%s'", archive->files[i].filename().c_str());

                        /* TODO: handle error (how?) */
                        archive->file_delete(i);
                    }
                    break;

                case FS_SUPERFLUOUS:
                    if (archive->is_writable()) {
			output.message_verbose("delete unused file '%s'", archive->files[i].filename().c_str());

                        /* TODO: handle error (how?) */
                        archive->file_delete(i);
                    }
                    break;

                case FS_NEEDED:
                    /* TODO: handle error (how?) */
                    if (save_needed(archive, i, game->name)) {
                        check_tree.recheck_games_needing(filetype, archive->files[i].hashes.size, &archive->files[i].hashes);
                    }
                    break;

                case FS_BROKEN:
                case FS_USED:
                case FS_MISSING:
                case FS_PARTUSED:
                    /* nothing to be done */
                    break;
            }
        }

        if (configuration.fix_romset) {
            if (!garbage->commit()) {
                garbage->rollback();
                archive->rollback();
                output.archive_error("committing garbage failed");
                return -1;
            }
        }

        if (!configuration.complete_games_only || result->game == GS_CORRECT || result->game == GS_CORRECT_MIA || result->game == GS_FIXABLE) {
            ret |= fix_files(game, filetype, archive, result, garbage.get());
        }
        else {
            ret |= clear_incomplete(game, filetype, archive, result, garbage.get());
        }

        if (configuration.fix_romset) {
            if (!garbage->close()) {
                archive->rollback();
                output.archive_error("closing garbage failed");
                return -1;
            }
        }

        if (archive->commit()) {
            extra_mark.commit();
            needed_mark.commit();
            superfluous_mark.commit();
        }
        else {
            archive->rollback();
        }
    }

    return ret;
}

int fix_save_needed_from_unknown(Game *game, const GameArchives archives, Result *result) {
    auto needs_recheck = false;

    if (configuration.fix_romset) {
        for (auto filetype: db->filetypes()) {
	    auto garbage_name = make_garbage_name(archives[filetype]->name, 0);

	    warn_set_info(WARN_TYPE_ARCHIVE, garbage_name);
	    for (size_t i = 0; i < game->files[filetype].size(); i++) {
		Match *match = &result->game_files[filetype][i];
		if (match->quality != Match::COPIED || match->source_is_old() || match->archive->name != garbage_name) {
		    continue;
		}

		/* we can't copy from our own garbage archive, since we're copying to it, and libzip doesn't support cross copying */

		/* TODO: handle error (how?) */
		if (save_needed(match->archive.get(), match->index, game->name)) {
		    needs_recheck = true;
		}

		break;
	    }
	    warn_unset_info();
	}
    }

    return needs_recheck ? 1 : 0;
}


static int
make_space(Archive *a, const std::string &name, std::vector<std::string> *original_names, size_t num_names) {
    auto idx = a->file_index_by_name(name);

    if (!idx.has_value()) {
	return 0;
    }

    auto index = idx.value();

    if (index < num_names && (*original_names)[index].empty()) {
	(*original_names)[index] = a->files[index].filename();
    }

    if (a->files[index].broken) {
	output.message_verbose("delete broken '%s'", name.c_str());
        return a->file_delete(index) ? 0 : -1;
    }

    return a->file_rename_to_unique(index) ? 0 : -1;
}


#define REAL_NAME(aa, ii) ((aa) == archive && (ii) < num_names && !original_names[(ii)].empty() ? original_names[(ii)].c_str() : (aa)->files[ii].filename().c_str())

static int fix_files(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage) {

    bool needs_recheck = false;

    output.set_error_archive(archive->name);

    size_t num_names = archive->files.size();
    std::vector<std::string> original_names;
    original_names.resize(num_names);

    for (size_t i = 0; i < game->files[filetype].size(); i++) {
        Archive *archive_from = nullptr;
        Match *match = &result->game_files[filetype][i];

        if (!match->source_is_old()) {
	    archive_from = match->archive.get();
        }
        auto &game_file = game->files[filetype][i];
	output.set_error_archive(archive->name, game_file.name);

	switch (match->quality) {
            case Match::MISSING:
            case Match::UNCHECKED:
                if (game_file.hashes.size == 0) {
                    /* create missing empty file */
		    output.message_verbose("create empty file '%s'", game_file.filename(filetype).c_str());

                    /* TODO: handle error (how?) */
                    archive->file_add_empty(game_file.name);
                }
                break;

            case Match::LONG: {
                if (archive == archive_from && !archive_from->is_file_deleted(match->index)) {
		    output.message_verbose("move long file '%s'", REAL_NAME(archive_from, match->index));
                    if (!garbage->add(match->index, true)) {
                        break;
                    }
                }

		output.message_verbose("extract (offset %" PRIu64 ", size %" PRIu64 ") from '%s' to '%s'", match->offset, game_file.hashes.size, REAL_NAME(archive_from, match->index), game_file.filename(filetype).c_str());

                if (make_space(archive, game_file.name, &original_names, num_names) < 0) {
                    break;
                }
                if (!archive->file_copy_part(archive_from, match->index, game_file.name, match->offset, game_file.hashes.size, &game_file.hashes)) {
                    break;
                }
                // TODO: is this still necessary since we always move long files?
                if (archive == archive_from && !archive_from->is_file_deleted(match->index)) {
                     archive_from->file_delete(match->index);
                }
                break;
            }

            case Match::NAME_ERROR:
                if (game_file.where == FILE_IN_CLONEOF || game_file.where == FILE_IN_GRAND_CLONEOF) {
                    if (check_tree.recheck(game->cloneof[game_file.where - 1])) {
                        /* fall-through to rename in case save_needed fails */
                        if (save_needed(archive, match->index, game->name)) {
                            check_tree.recheck_games_needing(filetype, game_file.hashes.size, &game_file.hashes);
                            break;
                        }
                    }
                }

		output.message_verbose("rename '%s' to '%s'", REAL_NAME(archive, match->index), game_file.filename(filetype).c_str());

                /* TODO: handle errors (how?) */
                if (make_space(archive, game_file.name, &original_names, num_names) < 0)
                    break;
                archive->file_rename(match->index, game_file.name);

                break;

            case Match::COPIED:
                if (garbage && archive_from == garbage->da.get()) {
                    /* we can't copy from our own garbage archive, since we're copying to it, and libzip doesn't support cross copying */
                    break;
                }
		output.message_verbose("add '%s/%s' as '%s'", archive_from->name.c_str(), REAL_NAME(archive_from, match->index), game_file.filename(filetype).c_str());

                if (make_space(archive, game_file.name, &original_names, num_names) < 0) {
                    /* TODO: if (idx >= 0) undo deletion of broken file */
                    break;
                }

                if (archive->file_copy(archive_from, match->index, game_file.name)) {
                    ckmame_cache->used(archive_from, match->index);
                }
                else {
                    output.error("copying '%s' from '%s' to '%s' failed, not deleting", game_file.filename(filetype).c_str(), archive_from->name.c_str(), archive->name.c_str());
                    /* TODO: if (idx >= 0) undo deletion of broken file */
                }
                break;

            case Match::IN_ZIP:
                /* TODO: save to needed */
                break;

            case Match::OK:
                /* all is well */
                break;

            case Match::OK_AND_OLD:
                /* already deleted by fix_game() */
                break;

            case Match::NO_HASH:
                /* only used for disks */
                break;

            case Match::OLD:
                /* nothing to be done */
                break;
        }
    }

    return needs_recheck ? 1 : 0;
}


static int clear_incomplete(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage) {

    output.set_error_archive(archive->name);

    for (size_t i = 0; i < game->files[filetype].size(); i++) {
        Archive *archive_from = nullptr;
        auto match = &result->game_files[filetype][i];
        auto game_file = &game->files[filetype][i];

        if (!match->source_is_old()) {
	    archive_from = match->archive.get();
        }

        output.set_error_archive(archive->name, game_file->name);

        switch (match->quality) {
            case Match::MISSING:
            case Match::OLD:
            case Match::UNCHECKED:
                /* nothing to do */
                break;

            case Match::NO_HASH:
                /* only used for disks */
                // TODO: handle disks
                break;

            case Match::LONG:
                save_needed_part(archive_from, match->index, game->name, match->offset, game_file->hashes.size, game_file); /* TODO: handle error */
                break;

            case Match::COPIED:
                switch(match->where) {
                    case FILE_INGAME:
                    case FILE_SUPERFLUOUS:
                        /* TODO: handle error (how?) */
                        save_needed(archive_from, match->index, game->name);
                        archive_from->commit();
                        break;

                    case FILE_EXTRA:
                    case FILE_IN_CLONEOF:
                    case FILE_IN_GRAND_CLONEOF:
                    case FILE_ROMSET:
                    case FILE_NEEDED:
                    case FILE_OLD:
                    case FILE_NOWHERE:
                        /* file is already where we will find it later */
                        break;
                }
                break;

            case Match::OK_AND_OLD:
                if (configuration.keep_old_duplicate) {
                    save_needed(archive, match->index, game->name); /* TODO: handle error */
                }

            case Match::NAME_ERROR:
            case Match::OK:
            case Match::IN_ZIP:
                save_needed(archive, match->index, game->name); /* TODO: handle error */
                break;
        }
    }

    return 0;
}
