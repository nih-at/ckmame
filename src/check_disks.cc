/*
  check_disks.c -- match files against disks
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


#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "match_disk.h"
#include "util.h"
#include "xmalloc.h"


void
check_disks(Game *game, ImagesPtr im[], Result *res) {
    if (game->disks.empty()) {
	return;
    }
    
    for (size_t i = 0; i < game->disks.size(); i++) {
        auto &match_disk = res->disks[i];
        auto &disk = game->disks[i];

        if (match_disk.quality == QU_OLD) {
	    continue;
        }

        int j = images_find(im[disk.where].get(), disk.merged_name());

        if (j != -1) {
            auto expected_image = im[disk.where]->disks[j];
            match_disk.where = disk.where;
	    match_disk.set_source(expected_image.get());

            switch (disk.hashes.compare(*disk_hashes(expected_image))) {
                case Hashes::MATCH:
                    match_disk.quality = QU_OK;
                    if (disk.where == FILE_INGAME) {
                        result_image(res, j) = FS_USED;
                    }
                    break;
                    
                case Hashes::MISMATCH:
                    match_disk.quality = QU_HASHERR;
                    break;
                
                default:
                    break;
            }
	}

	if (disk.where != FILE_INGAME || disk.hashes.empty()) {
            /* stop searching if disk is not supposed to be in this location, or we have no checksums */
	    continue;
	}

	if (match_disk.quality != QU_OK && im[0] != NULL) {
            for (size_t k = 0; k < im[0]->disks.size(); k++) {
                auto image = im[0]->disks[k];
                
                if (disk.hashes.compare(*disk_hashes(image)) == Hashes::MATCH) {
                    match_disk.where = FILE_INGAME;
                    match_disk.set_source(image.get());
                    match_disk.quality = QU_NAMEERR;
                    result_image(res, k) = FS_USED;
                }
            }
        }

	if (match_disk.quality != QU_OK && match_disk.quality != QU_OLD) {
            MatchDisk disk_match;
            
            if (find_disk_in_romset(&disk, game->name.c_str(), &disk_match) == FIND_EXISTS) {
                match_disk = disk_match;
                match_disk.where = FILE_ROMSET;
                continue;
            }
	    /* search in needed, superfluous and extra dirs */
	    ensure_extra_maps(DO_MAP);
	    ensure_needed_maps();
	    if (find_disk(&disk, &match_disk) == FIND_EXISTS)
		continue;
	}
    }
    
    for (size_t i = 0; i < game->disks.size(); i++) {
        stats_add_disk(stats, &game->disks[i], res->disks[i].quality);
    }
}
