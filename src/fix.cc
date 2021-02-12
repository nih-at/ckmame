/*
  fix.c -- fix ROM sets
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

#include <algorithm>
#include <filesystem>
#include <string>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zip.h>

#include "archive.h"
#include "check_util.h"
#include "error.h"
#include "file_util.h"
#include "fix_util.h"
#include "game.h"
#include "garbage.h"
#include "globals.h"
#include "match.h"
#include "tree.h"
#include "types.h"
#include "util.h"

// Fix archive in ROM set as best we can.
static int fix_files(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage);

// Clear and remove incomplete archive in ROM set, keeping needed files in needed/.
static int clear_incomplete(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage);


int
fix_game(Game *game, const GameArchives archives, Result *result) {
    int ret;

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        auto filetype = static_cast<filetype_t>(ft);
        Archive *archive = archives[filetype];
        GarbagePtr garbage;
        DeleteList::Mark extra_mark, needed_mark, superfluous_mark;

        if (fix_options & FIX_DO) {
            garbage = std::make_shared<Garbage>(archive);

            archive->ensure_valid_archive();

            extra_mark = DeleteList::Mark(extra_delete_list);
            needed_mark = DeleteList::Mark(needed_delete_list);
            superfluous_mark = DeleteList::Mark(superfluous_delete_list);
        }

        for (uint64_t i = 0; i < archive->files.size(); i++) {
            switch (result->archive_files[filetype][i]) {
                case FS_UNKNOWN: {
                    if (fix_options & FIX_IGNORE_UNKNOWN) {
                        break;
                    }
                    auto move = (fix_options & FIX_MOVE_UNKNOWN);
                    if (fix_options & FIX_PRINT) {
                        printf("%s: %s unknown file '%s'\n", archive->name.c_str(), (move ? "move" : "delete"), archive->filename(i).c_str());
                    }
                    
                    if (fix_options & FIX_DO) {
                        if (move) {
                            garbage->add(i, false); /* TODO: check return value */
                        }
                        else {
                            archive->file_delete(i); /* TODO: check return value */
                        }
                    }
                    break;
                }
                    
                case FS_DUPLICATE:
                    if (!(fix_options & FIX_DELETE_DUPLICATE)) {
                        break;
                    }
                    /* fallthrough */
                case FS_SUPERFLUOUS:
                    if (fix_options & FIX_PRINT) {
                        printf("%s: delete %s file '%s'\n", archive->name.c_str(), (result->archive_files[filetype][i] == FS_SUPERFLUOUS ? "unused" : "duplicate"), archive->filename(i).c_str());
                    }
                    
                    /* TODO: handle error (how?) */
                    archive->file_delete(i);
                    break;
                    
                case FS_NEEDED:
                    /* TODO: handle error (how?) */
                    if (save_needed(archive, i, game->name)) {
                        check_tree.recheck_games_needing(filetype, archive->files[i].size, &archive->files[i].hashes);
                    }
                    break;
                    
                case FS_BROKEN:
                    // TODO: move broken disks aside?
                case FS_USED:
                case FS_MISSING:
                case FS_PARTUSED:
                    /* nothing to be done */
                    break;
            }
        }
        
        if (fix_options & FIX_DO) {
            if (!garbage->commit()) {
                /* TODO: error message? (or is message from archive_close enough?) */
                /* TODO: handle error (how?) */
                archive->rollback();
                myerror(ERRZIP, "committing garbage failed");
                return -1;
            }
        }
        
        if ((fix_options & FIX_COMPLETE_ONLY) == 0 || result->game == GS_CORRECT || result->game == GS_FIXABLE) {
            ret = fix_files(game, filetype, archive, result, garbage.get());
        }
        else {
            ret = clear_incomplete(game, filetype, archive, result, garbage.get());
        }

        if (fix_options & FIX_DO) {
            if (!garbage->close()) {
                archive->rollback();
                myerror(ERRZIP, "closing garbage failed");
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


static int
make_space(Archive *a, const std::string &name, std::vector<std::string> *original_names, size_t num_names) {
    auto idx = a->file_index_by_name(name);

    if (!idx.has_value()) {
	return 0;
    }

    auto index = idx.value();

    if (index < num_names && (*original_names)[index].empty()) {
	(*original_names)[index] = name;
    }

    if (a->files[index].status == STATUS_BADDUMP) {
        if (fix_options & FIX_PRINT) {
	    printf("%s: delete broken '%s'\n", a->name.c_str(), name.c_str());
        }
        return a->file_delete(index) ? 0 : -1;
    }

    return a->file_rename_to_unique(index) ? 0 : -1;
}


#define REAL_NAME(aa, ii) ((aa) == archive && (ii) < static_cast<int64_t>(num_names) && !original_names[(ii)].empty() ? original_names[(ii)].c_str() : (aa)->filename(ii).c_str())

static int fix_files(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage) {

    bool needs_recheck = false;

    seterrinfo("", archive->name);

    size_t num_names = archive->files.size();
    std::vector<std::string> original_names;
    original_names.resize(num_names);

    for (size_t i = 0; i < game->files[filetype].size(); i++) {
        Archive *archive_from = NULL;
        Match *match = &result->game_files[filetype][i];

        if (!match->source_is_old()) {
	    archive_from = match->archive.get();
        }
        File *game_file = &game->files[filetype][i];
	seterrinfo(game_file->name, archive->name);

	switch (match->quality) {
            case QU_MISSING:
                if (game_file->size == 0) {
                    /* create missing empty file */
                    if (fix_options & FIX_PRINT) {
                        printf("%s: create empty file '%s'\n", archive->name.c_str(), game_file->name.c_str());
                    }
                    
                    /* TODO: handle error (how?) */
                    archive->file_add_empty(game_file->name);
                }
                break;
                
            case QU_HASHERR:
                /* all is lost */
                break;
                
            case QU_LONG: {
                if (archive == archive_from && (fix_options & FIX_MOVE_LONG) && archive_from->files[match->index].where != FILE_DELETED) {
                    if (fix_options & FIX_PRINT) {
                        printf("%s: move long file '%s'\n", archive_from->name.c_str(), REAL_NAME(archive_from, match->index));
                    }
                    if (!garbage->add(match->index, true)) {
                        break;
                    }
                }
                
                if (fix_options & FIX_PRINT) {
                    printf("%s: extract (offset %jd, size %" PRIu64 ") from '%s' to '%s'\n", archive->name.c_str(), match->offset, game_file->size, REAL_NAME(archive_from, match->index), game_file->name.c_str());
                }
                
                bool replacing_ourselves = (archive == archive_from && match->index == archive_from->file_index_by_name(game_file->name));
                if (make_space(archive, game_file->name, &original_names, num_names) < 0) {
                    break;
                }
                if (!archive->file_copy_part(archive_from, match->index, game_file->name, match->offset, game_file->size, game_file)) {
                    break;
                }
                if (archive == archive_from && archive_from->files[match->index].where != FILE_DELETED) {
                    if (!replacing_ourselves && !(fix_options & FIX_MOVE_LONG) && (fix_options & FIX_PRINT)) {
                        printf("%s: delete long file '%s'\n", archive_from->name.c_str(), game_file->name.c_str());
                    }
                    archive_from->file_delete(match->index);
                }
                break;
            }
                
            case QU_NAMEERR:
                if (game_file->where == FILE_INCO || game_file->where == FILE_INGCO) {
                    if (check_tree.recheck(game->cloneof[game_file->where - 1])) {
                        /* fall-through to rename in case save_needed fails */
                        if (save_needed(archive, match->index, game->name)) {
                            check_tree.recheck_games_needing(filetype, game_file->size, &game_file->hashes);
                            break;
                        }
                    }
                }
                
                if (fix_options & FIX_PRINT)
                    printf("%s: rename '%s' to '%s'\n", archive->name.c_str(), REAL_NAME(archive, match->index), game_file->name.c_str());
                
                /* TODO: handle errors (how?) */
                if (make_space(archive, game_file->name, &original_names, num_names) < 0)
                    break;
                archive->file_rename(match->index, game_file->name);
                
                break;
                
            case QU_COPIED:
                if (garbage && archive_from == garbage->da.get()) {
                    /* we can't copy from our own garbage archive, since we're copying to it, and libzip doesn't support cross copying */
                    
                    /* TODO: handle error (how?) */
                    if (save_needed(archive_from, match->index, game->name)) {
                        needs_recheck = true;
                    }
                    
                    break;
                }
                if (fix_options & FIX_PRINT) {
                    printf("%s: add '%s/%s' as '%s'\n", archive->name.c_str(), archive_from->name.c_str(), REAL_NAME(archive_from, match->index), game_file->name.c_str());
                }
                
                if (make_space(archive, game_file->name, &original_names, num_names) < 0) {
                    /* TODO: if (idx >= 0) undo deletion of broken file */
                    break;
                }
                
                if (!archive->file_copy(archive_from, match->index, game_file->name)) {
                    myerror(ERRDEF, "copying '%s' from '%s' to '%s' failed, not deleting", game_file->name.c_str(), archive_from->name.c_str(), archive->name.c_str());
                    /* TODO: if (idx >= 0) undo deletion of broken file */
                }
                else {
                    DeleteList::used(archive_from, match->index);
                }
                break;
                
            case QU_INZIP:
                /* TODO: save to needed */
                break;
                
            case QU_OK:
                /* all is well */
                break;
                
            case QU_NOHASH:
                /* only used for disks */
                break;
                
            case QU_OLD:
                /* nothing to be done */
                break;
        }
    }
    
    return needs_recheck ? 1 : 0;
}


static int clear_incomplete(Game *game, filetype_t filetype, Archive *archive, Result *result, Garbage *garbage) {

    seterrinfo("", archive->name);

    for (size_t i = 0; i < game->files[filetype].size(); i++) {
        Archive *archive_from = NULL;
        auto match = &result->game_files[filetype][i];
        auto game_file = &game->files[filetype][i];

        if (!match->source_is_old()) {
	    archive_from = match->archive.get();
        }
        
        seterrinfo(game_file->name, archive->name);

        switch (match->quality) {
            case QU_MISSING:
            case QU_OLD:
                /* nothing to do */
                break;
                
            case QU_NOHASH:
                /* only used for disks */
                // TODO: handle disks
                break;
                
            case QU_HASHERR:
                /* all is lost */
                break;
                
            case QU_LONG:
                save_needed_part(archive_from, match->index, game->name, match->offset, game_file->size, game_file); /* TODO: handle error */
                break;
                
            case QU_COPIED:
                switch(match->where) {
                    case FILE_INGAME:
                    case FILE_SUPERFLUOUS:
                    case FILE_EXTRA:
                        /* TODO: handle error (how?) */
                        save_needed(archive_from, match->index, game->name);
                        archive_from->commit();
                        break;
                        
                    case FILE_INCO:
                    case FILE_INGCO:
                    case FILE_ROMSET:
                    case FILE_NEEDED:
                    case FILE_OLD:
                    case FILE_NOWHERE:
                    case FILE_ADDED:
                    case FILE_DELETED:
                        /* file is already where we will find it later */
                        break;
                }
                break;
                
            case QU_NAMEERR:
            case QU_OK:
            case QU_INZIP:
                save_needed(archive, i, game->name); /* TODO: handle error */
                break;
        }
    }

    return 0;
}
