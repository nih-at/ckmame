/*
  fixdat.c -- write fixdat
  Copyright (C) 2012 Dieter Baron and Thomas Klausner

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



#include "funcs.h"
#include "globals.h"
#include "match.h"
#include "match_disk.h"
#include "output.h"



void
write_fixdat_entry(const game_t *game, const archive_t *a, const images_t *im,
		   const result_t *res)
{
    game_t *gm;
    int i;
    
    if (result_game(res) != GS_MISSING)
	return;

    gm = game_new();
    game_name(gm) = strdup(game_name(game));

    
    for (i=0; i<game_num_files(game, TYPE_ROM); i++) {
	match_t *m = result_rom(res, i);
	file_t *r = game_file(game, TYPE_ROM, i);

	if (match_quality(m) != QU_MISSING || file_status(r) == STATUS_NODUMP)
	    continue;

	file_t *rm = array_push(game_files(gm, TYPE_ROM), r);
	file_name(rm) = strdup(file_name(r));
	if (file_merge(r))
	    file_merge(rm) = strdup(file_merge(r));
    }

    for (i=0; i<game_num_disks(game); i++) {
	match_disk_t *m = result_disk(res, i);
	disk_t *d = game_disk(game, i);

	if (match_disk_quality(m) != QU_MISSING || disk_status(d) == STATUS_NODUMP)
	    continue;

	disk_t *dm = array_push(game_disks(gm), d);
	disk_name(dm) = strdup(disk_name(d));
	if (disk_merge(d))
	    disk_merge(dm) = strdup(disk_merge(d));
    }

    if (game_num_files(gm, TYPE_ROM) > 0 || game_num_disks(gm) > 0)
	output_game(fixdat, gm);

    game_free(gm);
}
