/*
  diagnostics.c -- display result of check
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include "diagnostics.h"

#include <sstream>

#include "fix.h"
#include "warn.h"

int diagnostics_options = 0;

static const std::string zname[] = {"", "cloneof", "grand-cloneof"};

static void diagnostics_game(filetype_t ft, const Game *game, const Result &result);


void diagnostics(const Game *game, const GameArchives &archives, const Result &result) {
    warn_set_info(WARN_TYPE_GAME, game->name);

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        diagnostics_game(static_cast<filetype_t>(ft), game, result);
        diagnostics_archive(static_cast<filetype_t>(ft), archives[ft], result);
    }
}


void diagnostics_archive(filetype_t ft, const Archive *a, const Result &result) {
    if (a == NULL) {
        return;
    }
    
    for (size_t i = 0; i < a->files.size(); i++) {
        auto f = &a->files[i];
            
        switch (result.archive_files[ft][i]) {
            case FS_UNKNOWN:
                if (diagnostics_options & WARN_UNKNOWN)
                    warn_archive_file(ft, f, "unknown");
                break;
                
            case FS_PARTUSED:
            case FS_USED:
            case FS_MISSING:
                /* handled in diagnostics_game */
                break;
                
            case FS_BROKEN:
                if (diagnostics_options & WARN_FILE_BROKEN)
                    warn_archive_file(ft, f, "broken");
                break;
                
            case FS_NEEDED:
                if (diagnostics_options & WARN_USED)
                    warn_archive_file(ft, f, "needed elsewhere");
                break;
                
            case FS_SUPERFLUOUS:
                if (diagnostics_options & WARN_SUPERFLUOUS)
                    warn_archive_file(ft, f, "not used");
                break;
            case FS_DUPLICATE:
                if ((diagnostics_options & WARN_SUPERFLUOUS) && (fix_options & FIX_DELETE_DUPLICATE))
                    warn_archive_file(ft, f, "duplicate");
                break;
        }
    }
}


static void
diagnostics_game(filetype_t ft, const Game *game, const Result &result) {
    switch (result.game) {
        case GS_CORRECT:
            if (ft == TYPE_ROM && (diagnostics_options & WARN_CORRECT)) {
                warn_game_file(ft, NULL, "correct");
            }
            return;
            
        case GS_OLD:
            if (!(diagnostics_options & WARN_CORRECT)) {
                return;
            }
            else {
                // TODO: move this to check and keep in result
                auto all_same = true;
                std::string old_name;
                for (size_t ft = 0; ft < TYPE_MAX; ft++) {
                    if (!result.game_files[ft].empty()) {
                        old_name = result.game_files[ft][0].old_game;
                        break;
                    }
                }

                if (!old_name.empty()) {
                    for (size_t ft = 0; ft < TYPE_MAX && all_same; ft++) {
                        for (size_t i = 1; i < game->files[ft].size(); i++) {
                            if (result.game_files[ft][i].old_game != old_name) {
                                all_same = false;
                                break;
                            }
                        }
                    }
                }
                if (all_same) {
                    if (ft == TYPE_ROM) {
                        warn_game_file(ft, NULL, "old in '" + old_name + "'");
                    }
                    return;
                }
            }
            break;

        case GS_MISSING:
            if (ft == TYPE_ROM && (diagnostics_options & WARN_MISSING)) {
                warn_game_file(ft, NULL, "not a single file found");
            }
            return;

        default:
            break;
    }
    
    if ((fix_options & FIX_COMPLETE_ONLY) && (diagnostics_options & WARN_BROKEN) == 0 && result.game != GS_FIXABLE) {
        return;
    }
    
    for (size_t i = 0; i < game->files[ft].size(); i++) {
        auto &match = result.game_files[ft][i];
        auto &rom = game->files[ft][i];
        FileData *file = NULL;
        
        if (!match.source_is_old() && match.archive) {
            file = &match.archive->files[match.index];
        }
        
        switch (match.quality) {
            case QU_MISSING:
                if (diagnostics_options & WARN_MISSING) {
                    if (rom.status != Rom::NO_DUMP || (diagnostics_options & WARN_NO_GOOD_DUMP)) {
                        warn_game_file(ft, &rom, "missing");
                    }
                }
                break;
                
            case QU_NAMEERR:
                if (diagnostics_options & WARN_WRONG_NAME) {
                    warn_game_file(ft, &rom, "wrong name (" + file->name + ")" + (rom.where != match.where ? ", should be in " : "") + zname[rom.where]);
                }
                break;
                
            case QU_LONG:
                if (diagnostics_options & WARN_LONGOK) {
                    std::ostringstream out;
                    out << "too long, valid subsection at byte " << match.offset << " (" << file->hashes.size << ")" << (rom.where != match.where ? ", should be in " : "") << zname[rom.where];
                    warn_game_file(ft, &rom, out.str());
                }
                break;
                
            case QU_OK:
                if (diagnostics_options & WARN_CORRECT) {
                    if (rom.status == Rom::OK) {
                        warn_game_file(ft, &rom, "correct");
                    }
                    else {
                        warn_game_file(ft, &rom, rom.status == Rom::BAD_DUMP ? "best bad dump" : "exists");
                    }
                }
                break;
                
            case QU_COPIED:
                if (diagnostics_options & WARN_ELSEWHERE) {
                    warn_game_file(ft, &rom, "is in '" + match.archive->name + "/" + match.archive->files[match.index].filename() + "'");
                }
                break;
                
            case QU_INZIP:
                if (diagnostics_options & WARN_WRONG_ZIP) {
                    warn_game_file(ft, &rom, "should be in " + zname[rom.where]);
                }
                break;
                
            case QU_OLD:
                if (diagnostics_options & WARN_CORRECT) {
                    warn_game_file(ft, &rom, "old in '" + match.old_game + "'");
                }
                break;
                
            case QU_NOHASH:
                /* only used for disks */
                break;
        }
    }
}
