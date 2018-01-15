/*
  result.c -- allocate/free result structure
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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


#include "result.h"
#include "globals.h"
#include "xmalloc.h"


void
result_free(result_t *res) {
    if (res == NULL)
	return;

    match_array_free(res->roms);
    file_status_array_free(res->files);
    match_disk_array_free(res->disks);
    file_status_array_free(res->images);
    free(res);
}


result_t *
result_new(const game_t *g, const archive_t *a, const images_t *im) {
    result_t *res;

    res = (result_t *)xmalloc(sizeof(*res));

    result_game(res) = GS_MISSING;
    result_roms(res) = NULL;
    result_files(res) = NULL;
    result_disks(res) = NULL;
    result_images(res) = NULL;

    if (g) {
	res->roms = match_array_new(game_num_files(g, file_type));

	if (game_num_disks(g) > 0 && file_type == TYPE_ROM)
	    result_disks(res) = match_disk_array_new(game_num_disks(g));
    }

    if (a)
	result_files(res) = file_status_array_new(archive_num_files(a));

    if (im)
	result_images(res) = file_status_array_new(images_length(im));

    return res;
}
