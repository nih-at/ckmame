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
#include "globals.h"

#include <sstream>

#include "fix.h"
#include "Stats.h"
#include "warn.h"

static const std::string zname[] = {"", "cloneof", "grand-cloneof"};

static void diagnostics_game(filetype_t ft, const Game *game, const Result &result);


void diagnostics(const Game *game, const GameArchives &archives, const Result &result) {
    warn_set_info(WARN_TYPE_GAME, game->name);

    stats.add_game(result.game);

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        auto filetype = static_cast<filetype_t>(ft);
        
        for (size_t i = 0; i < game->files[filetype].size(); i++) {
            stats.add_rom(filetype, &game->files[filetype][i], result.game_files[filetype][i].quality);
        }
        diagnostics_game(static_cast<filetype_t>(filetype), game, result);
        diagnostics_archive(static_cast<filetype_t>(filetype), archives[filetype], result);
    }
}


void diagnostics_archive(filetype_t ft, const Archive *a, const Result &result, bool is_in_needed) {
    if (a == NULL) {
        return;
    }
    
    for (size_t i = 0; i < a->files.size(); i++) {
        auto f = &a->files[i];
            
        switch (result.archive_files[ft][i]) {
            case FS_UNKNOWN:
                if (configuration.warn_file_unknown) {
                    warn_archive_file(ft, f, "unknown");
                }
                break;
                
            case FS_PARTUSED:
            case FS_USED:
            case FS_MISSING:
                /* handled in diagnostics_game */
                break;
                
            case FS_BROKEN:
                warn_archive_file(ft, f, "broken");
                break;
                
            case FS_NEEDED:
                if (!is_in_needed && configuration.warn_file_known) {
                    warn_archive_file(ft, f, "needed elsewhere");
                }
                break;
                
            case FS_SUPERFLUOUS:
                if (configuration.warn_file_known) {
                    warn_archive_file(ft, f, "not used");
                }
                break;
            case FS_DUPLICATE:
                if (configuration.warn_file_known) {
                    warn_archive_file(ft, f, "duplicate");
                }
                break;
        }
    }
}


static void
diagnostics_game(filetype_t ft, const Game *game, const Result &result) {
    if (!configuration.report_detailed) {
        switch (result.game) {
            case GS_CORRECT:
                if (ft == TYPE_ROM && configuration.report_correct) {
                    warn_game_file(ft, NULL, "correct");
                }
                return;
            
            case GS_OLD: {
                // TODO: does not work if whole game is duplicate from old.
                if (!configuration.report_correct) {
                    return;
                }
                
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
                break;
            }

            case GS_MISSING:
                if (ft == TYPE_ROM && configuration.report_missing) {
                    warn_game_file(ft, NULL, "not a single file found");
                }
                return;
                
            default:
                break;
        }
    }
    
    if (configuration.complete_games_only && !(configuration.report_missing || configuration.report_fixable) && result.game != GS_FIXABLE) {
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
            case Match::MISSING:
                if ((configuration.report_missing && rom.status != Rom::NO_DUMP) || configuration.report_detailed) {
                    warn_game_file(ft, &rom, "missing");
                }
                break;
                
            case Match::NAME_ERROR:
                if (configuration.report_fixable) {
                    warn_game_file(ft, &rom, "wrong name (" + file->name + ")" + (rom.where != match.where ? ", should be in " : "") + zname[rom.where]);
                }
                break;
                
            case Match::LONG:
                if (configuration.report_fixable) {
                    std::ostringstream out;
                    out << "too long, valid subsection at byte " << match.offset << " (" << file->hashes.size << ")" << (rom.where != match.where ? ", should be in " : "") << zname[rom.where];
                    warn_game_file(ft, &rom, out.str());
                }
                break;
                
            case Match::OK:
                if (rom.status == Rom::OK) {
                    if (configuration.report_correct || configuration.report_detailed) {
                        warn_game_file(ft, &rom, "correct");
                    }
                }
                else {
                    if (configuration.report_detailed) {
                        warn_game_file(ft, &rom, rom.status == Rom::BAD_DUMP ? "best bad dump" : "exists");
                    }
                }
                break;
                
            case Match::OK_AND_OLD:
                if (configuration.report_correct || configuration.report_detailed) {
                    warn_game_file(ft, &rom, "duplicate (also in old '" + match.old_game + "')");
                }
                break;
                
            case Match::COPIED:
                if (configuration.report_fixable) {
                    warn_game_file(ft, &rom, "is in '" + match.archive->name + "/" + match.archive->files[match.index].filename() + "'");
                }
                break;
                
            case Match::IN_ZIP:
                if (configuration.report_fixable) {
                    warn_game_file(ft, &rom, "should be in " + zname[rom.where]);
                }
                break;
                
            case Match::OLD:
                if (configuration.report_correct || configuration.report_detailed) {
                    warn_game_file(ft, &rom, "is in old '" + match.old_game + "'");
                }
                break;
                
            case Match::NO_HASH:
                /* only used for disks */
                break;
        }
    }
}
