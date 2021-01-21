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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "archive.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "match.h"
#include "match_disk.h"
#include "types.h"
#include "warn.h"

static const std::string zname[] = {"", "cloneof", "grand-cloneof"};

static void diagnostics_disks(const Game *, const Result *);
static void diagnostics_files(const Game *, const Result *);


void
diagnostics(const Game *game, const ArchivePtr a, const Images *im, const Result *result) {
    warn_set_info(WARN_TYPE_GAME, game->name);

    diagnostics_files(game, result);
    diagnostics_archive(a, result);
    diagnostics_disks(game, result);
    diagnostics_images(im, result);
}


void
diagnostics_archive(const ArchivePtr a, const Result *result) {
    if (!a) {
	return;
    }

    for (size_t i = 0; i < a->files.size(); i++) {
        auto f = &a->files[i];

	switch (result->files[i]) {
	case FS_UNKNOWN:
	    if (output_options & WARN_UNKNOWN)
		warn_file(f, "unknown");
	    break;

	case FS_PARTUSED:
	case FS_USED:
	case FS_MISSING:
	    /* handled in diagnostics_files */
	    break;

	case FS_BROKEN:
	    if (output_options & WARN_FILE_BROKEN)
		warn_file(f, "broken");
	    break;

	case FS_NEEDED:
	    if (output_options & WARN_USED)
		warn_file(f, "needed elsewhere");
	    break;

	case FS_SUPERFLUOUS:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_file(f, "not used");
	    break;
	case FS_DUPLICATE:
	    if ((output_options & WARN_SUPERFLUOUS) && (fix_options & FIX_DELETE_DUPLICATE))
		warn_file(f, "duplicate");
	    break;
	}
    }
}


static void
diagnostics_disks(const Game *game, const Result *result) {
    if (game->disks.empty()) {
	return;
    }

    for (size_t i = 0; i < game->disks.size(); i++) {
        auto match_disk = result->disks[i];
        auto d = &game->disks[i];

	switch (match_disk.quality) {
            case QU_MISSING:
                if ((d->status != STATUS_NODUMP && (output_options & WARN_MISSING)) || (output_options & WARN_NO_GOOD_DUMP))
                    warn_disk(d, "missing");
                break;
                
            case QU_HASHERR:
                if (output_options & WARN_WRONG_CRC) {
                    /* TODO: display checksum(s) */
                    warn_disk(d, "wrong checksum");
                }
                break;
                
            case QU_NOHASH:
                if (d->status == STATUS_NODUMP && (output_options & WARN_NO_GOOD_DUMP))
                    warn_disk(d, "exists");
                else if (output_options & WARN_WRONG_CRC) {
                    /* TODO: display checksum(s) */
                    warn_disk(d, "no common checksum types");
                }
                break;
                
            case QU_OK:
                if (output_options & WARN_CORRECT)
                    warn_disk(d, d->status == STATUS_OK ? "correct" : "exists");
                break;
                
            case QU_OLD:
                if (output_options & WARN_CORRECT)
                    warn_disk(d, "old");
                break;
                
            case QU_COPIED:
                if (output_options & WARN_ELSEWHERE)
                    warn_disk(d, "is at '" + match_disk.name + "'");
                break;
                
            case QU_NAMEERR:
                if (output_options & WARN_WRONG_NAME)
                    warn_disk(d, "wrong name (" + match_disk.name + ")");
                break;
                
            default:
                break;
        }
    }
}


static void
diagnostics_files(const Game *game, const Result *result) {
    switch (result->game) {
    case GS_CORRECT:
	if (output_options & WARN_CORRECT)
	    warn_rom(NULL, "correct");
	return;
    case GS_OLD:
	if (!(output_options & WARN_CORRECT))
	    return;
	else {
	    auto all_same = true;
	    for (size_t i = 1; i < game->roms.size(); i++) {
                if (result->roms[0].old_game != result->roms[i].old_game) {
		    all_same = false;
		    break;
		}
	    }
	    if (all_same) {
		warn_rom(NULL, "old in '" + result->roms[0].old_game + "'");
		return;
	    }
	}
	break;
    case GS_MISSING:
	if (output_options & WARN_MISSING)
	    warn_rom(NULL, "not a single rom found");
	return;
    default:
	break;
    }
    
    if ((fix_options & FIX_COMPLETE_ONLY) && (output_options & WARN_BROKEN) == 0 && result->game != GS_FIXABLE) {
        return;
    }

    for (size_t i = 0; i < game->roms.size(); i++) {
        auto &match = result->roms[i];
        auto &rom = game->roms[i];
        File *file = NULL;
        
        if (!match.source_is_old() && match.archive) {
            file = &match.archive->files[match.index];
        }

        switch (match.quality) {
            case QU_MISSING:
                if (output_options & WARN_MISSING) {
                    if (rom.status != STATUS_NODUMP || (output_options & WARN_NO_GOOD_DUMP)) {
                        warn_rom(&rom, "missing");
                    }
                }
                break;

            case QU_NAMEERR:
                if (output_options & WARN_WRONG_NAME) {
                    warn_rom(&rom, "wrong name (" + file->name + ")" + (rom.where != match.where ? ", should be in " : "") + zname[rom.where]);
                }
                break;

            case QU_LONG:
                if (output_options & WARN_LONGOK) {
		    std::ostringstream out;
		    out << "too long, valid subsection at byte " << match.offset << " (" << file->size << ")" << (rom.where != match.where ? ", should be in " : "") << zname[rom.where];
                    warn_rom(&rom, out.str());
                }
                break;

            case QU_OK:
                if (output_options & WARN_CORRECT) {
                    if (rom.status == STATUS_OK) {
                        warn_rom(&rom, "correct");
                    }
                    else {
                        warn_rom(&rom, rom.status == STATUS_BADDUMP ? "best bad dump" : "exists");
                    }
                }
                break;

            case QU_COPIED:
                if (output_options & WARN_ELSEWHERE) {
                    warn_rom(&rom, "is in '" + match.archive->name + "/" + match.archive->files[match.index].name + "'");
                }
                break;

            case QU_INZIP:
                if (output_options & WARN_WRONG_ZIP) {
                    warn_rom(&rom, "should be in " + zname[rom.where]);
                }
                break;

            case QU_HASHERR:
                if (output_options & WARN_MISSING) {
                    warn_rom(&rom, "checksum mismatch" + (rom.where != match.where ? ", should be in " + zname[rom.where] : ""));
                }
                break;

            case QU_OLD:
                if (output_options & WARN_CORRECT) {
                    warn_rom(&rom, "old in '" + match.old_game + "'");
                }
                break;
                
            case QU_NOHASH:
                /* only used for disks */
                break;
        }
    }
}


void
diagnostics_images(const Images *im, const Result *result) {
    if (im == NULL)
	return;

    for (size_t i = 0; i < im->disks.size(); i++) {
	switch (result->images[i]) {
	case FS_UNKNOWN:
	    if (output_options & WARN_UNKNOWN)
		warn_image(images_name(im, i), "unknown");
	    break;
	case FS_BROKEN:
	    if (output_options & WARN_FILE_BROKEN)
		warn_image(images_name(im, i), "broken");
	    break;
	case FS_SUPERFLUOUS:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_image(images_name(im, i), "not used");
	    break;
	case FS_DUPLICATE:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_image(images_name(im, i), "duplicate");
	    break;
	case FS_NEEDED:
	    if (output_options & WARN_USED)
		warn_image(images_name(im, i), "needed elsewhere");
	    break;

	case FS_MISSING:
	case FS_PARTUSED:
	case FS_USED:
	    break;
	}
    }
}
