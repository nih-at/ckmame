/*
 stats.c -- store stats of the ROM set
 Copyright (C) 2018 Dieter Baron and Thomas Klausner
 
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

#include "Stats.h"

#include <cinttypes>

#include "util.h"
#include "globals.h"

void Stats::add_game(GameStatus status) {
    switch (status) {
        case GS_OLD:
        case GS_CORRECT:
        case GS_FIXABLE:
            games_good++;
            break;
            
        case GS_PARTIAL:
            games_partial++;
            break;
            
        case GS_MISSING:
            break;
    }
    
    games_total++;
}


void Stats::add_rom(enum filetype type, const FileData *rom, Match::Quality status) {
    // TODO: only own ROMs? (what does dumpgame /stats count?)
    
    add_file(type, rom->hashes.size, status);
}


void Stats::print(FILE *f, bool total_only) {
    static const char *ft_name[] = {"ROMs: ", "Disks:"};

    std::string message = "Games: ";
    if (!total_only) {
        if (games_good > 0 || games_partial == 0) {
	    message += std::to_string(games_good);
        }
        if (games_partial > 0) {
            if (games_good > 0) {
                message += " complete, ";
            }
            message += std::to_string(games_partial) + " partial";
        }
	message += " / ";
    }
    message += std::to_string(games_total);

    output.message(message);

    for (int type = 0; type < TYPE_MAX; type++) {
        if (files[type].files_total > 0) {
	    message = ft_name[type];
	    message += " ";
            if (!total_only) {
		message += std::to_string(files[type].files_good) + " / ";
            }
	    message += std::to_string(files[type].files_total);

            if (files[type].bytes_total > 0) {
                message += " (";
                if (!total_only) {
		    message += human_number(files[type].bytes_good) + " / ";
                }
		message += human_number(files[type].bytes_total) + ")";
            }
	    output.message(message);
        }
    }
}


void Stats::add_file(enum filetype type, uint64_t size, Match::Quality status) {
    if (status != Match::MISSING) {
        if (size != Hashes::SIZE_UNKNOWN) {
            files[type].bytes_good += size;
        }
        files[type].files_good++;
    }
    
    if (size != Hashes::SIZE_UNKNOWN) {
        files[type].bytes_total += size;
    }
    files[type].files_total++;
}
