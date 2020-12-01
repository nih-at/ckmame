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
check_images(images_t *im, const char *gamename, result_t *res) {
    disk_t *d;
    int i;

    if (im == NULL) {
	return;
    }

    for (i = 0; i < images_length(im); i++) {
	d = images_get(im, i);

	if (d == NULL) {
	    result_image(res, i) = FS_MISSING;
	    continue;
	}

	if (disk_status(d) != STATUS_OK) {
	    result_image(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_image(res, i) == FS_USED)
	    continue;

        if ((disk_hashes(d)->types & romdb_hashtypes(db, TYPE_DISK)) != romdb_hashtypes(db, TYPE_DISK)) {
	    /* TODO: compute missing hashes */
	}

	if (find_disk_in_old(d, NULL) == FIND_EXISTS) {
	    result_image(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_disk_in_romset(d, gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_image(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING: {
	    match_disk_t md;
	    match_disk_init(&md);
	    ensure_needed_maps();
	    if (find_disk(d, &md) != FIND_EXISTS)
		result_image(res, i) = FS_NEEDED;
	    else {
		if (match_disk_where(&md) == FILE_NEEDED)
		    result_image(res, i) = FS_SUPERFLUOUS;
		else
		    result_image(res, i) = FS_NEEDED;
	    }
	    match_disk_finalize(&md);
	} break;

	case FIND_ERROR:
	    /* TODO: how to handle? */
	    break;
	}
    }
}
