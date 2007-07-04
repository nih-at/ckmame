#ifndef HAD_RESULT_H
#define HAD_RESULT_H

/*
  $NiH: result.h,v 1.3 2006/05/01 21:09:11 dillo Exp $

  result.h -- result of game check
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_status.h"
#include "game.h"
#include "images.h"
#include "match.h"
#include "match_disk.h"
#include "parray.h"

struct result {
    game_status_t game;
    match_array_t *roms;
    file_status_array_t *files;

    match_disk_array_t *disks;
    file_status_array_t *images;
};

typedef struct result result_t;


#define result_disk(res, i)	(match_disk_array_get(result_disks(res), (i)))
#define result_disks(res)	((res)->disks)
#define result_file(res, i)	(file_status_array_get(result_files(res), (i)))
#define result_files(res)	((res)->files)
#define result_game(res) 	((res)->game)
#define result_image(res, i)	\
	(file_status_array_get(result_images(res), (i)))
#define result_images(res)	((res)->images)
#define result_num_disks(res)					\
	(result_disks(res)					\
	 ? match_disk_array_length(result_disks(res)) : 0)
#define result_num_files(res)					\
	(result_files(res)					\
	 ? file_status_array_length(result_files(res)) : 0)
#define result_num_roms(res)	\
	(result_roms(res) ? match_array_length(result_roms(res)) : 0)
#define result_rom(res, i)	(match_array_get(result_roms(res), (i)))
#define result_roms(res)	((res)->roms)



void result_free(result_t *);
result_t *result_new(const game_t *, const archive_t *, const images_t *);

#endif /* result.h */
