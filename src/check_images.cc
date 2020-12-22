/*
  check_images.c -- match files against disks
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
check_images(Images *im, const char *gamename, Result *res) {
    if (im == NULL) {
	return;
    }

    for (size_t i = 0; i < im->disks.size(); i++) {
        auto disk = im->disks[i];

	if (disk == NULL) {
	    result_image(res, i) = FS_MISSING;
	    continue;
	}

	if (disk_status(disk) != STATUS_OK) {
	    result_image(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_image(res, i) == FS_USED)
	    continue;

        if ((disk_hashes(disk)->types & romdb_hashtypes(db, TYPE_DISK)) != romdb_hashtypes(db, TYPE_DISK)) {
	    /* TODO: compute missing hashes */
	}

	if (find_disk_in_old(disk.get(), NULL) == FIND_EXISTS) {
	    result_image(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_disk_in_romset(disk.get(), gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_image(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING: {
	    MatchDisk md;

            ensure_needed_maps();
	    if (find_disk(disk.get(), &md) != FIND_EXISTS)
		result_image(res, i) = FS_NEEDED;
	    else {
		if (match_disk_where(&md) == FILE_NEEDED)
		    result_image(res, i) = FS_SUPERFLUOUS;
		else
		    result_image(res, i) = FS_NEEDED;
	    }
	} break;

	case FIND_ERROR:
	    /* TODO: how to handle? */
	    break;
	}
    }
}
