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
check_disks(game_t *game, images_t *im[], result_t *res) {
    disk_t *disk;
    match_disk_t *md;
    int i, j;

    if (game_num_disks(game) == 0)
	return;
    
    for (i = 0; i < game_num_disks(game); i++) {
	md = result_disk(res, i);
	disk = game_disk(game, i);

	if (match_disk_quality(md) == QU_OLD)
	    continue;

        j = images_find(im[disk_where(disk)], disk_merged_name(disk));

        if (j != -1) {
            disk_t *expected_image = images_get(im[disk_where(disk)], j);
            match_disk_where(md) = disk_where(disk);
	    match_disk_set_source(md, expected_image);

	    switch (hashes_cmp(disk_hashes(disk), disk_hashes(expected_image))) {
	    case HASHES_CMP_MATCH:
		match_disk_quality(md) = QU_OK;
                if (disk_where(disk) == FILE_INGAME) {
                    result_image(res, j) = FS_USED;
                }
		break;
	    case HASHES_CMP_MISMATCH:
		match_disk_quality(md) = QU_HASHERR;
		break;
                
            default:
                break;
	    }
	}

        if (disk_where(disk) == FILE_INGAME && match_disk_quality(md) != QU_OK && im[0] != NULL) {
            for (i = 0; i < images_length(im[0]); i++) {
                disk_t *image = images_get(im[0], i);
                
                if (hashes_cmp(disk_hashes(disk), disk_hashes(image)) == HASHES_CMP_MATCH) {
                    match_disk_where(md) = FILE_INGAME;
                    match_disk_set_source(md, image);
                    match_disk_quality(md) = QU_NAMEERR;
                    result_image(res, i) = FS_USED;
                }
            }
        }

	if (hashes_types(disk_hashes(disk)) == 0) {
	    continue;
	}

	if (match_disk_quality(md) != QU_OK && match_disk_quality(md) != QU_OLD) {
	    /* search in needed, superfluous and extra dirs */
	    ensure_extra_maps(DO_MAP);
	    ensure_needed_maps();
	    if (find_disk(disk, md) == FIND_EXISTS)
		continue;
	}
    }
    
    for (i = 0; i < game_num_disks(game); i++) {
        stats_add_disk(stats, game_disk(game, i), match_quality(result_disk(res, i)));
    }
}
