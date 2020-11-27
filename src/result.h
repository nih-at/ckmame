#ifndef HAD_RESULT_H
#define HAD_RESULT_H

/*
  result.h -- result of game check
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_status.h"
#include "game.h"
#include "images.h"
#include "match.h"
#include "match_disk.h"
#include "parray.h"

class Result {
public:
    Result(const Game *g, const Archive *a, const images_t *im);
    ~Result();
    game_status_t game;

    std::vector<Match> roms;
    std::vector<file_status_> files;

    match_disk_array_t *disks;
    std::vector<file_status_> images;
};

typedef Result result_t;


#define result_disk(res, i) (match_disk_array_get((res)->disks, (i)))
#define result_disks(res) ((res)->disks)
#define result_file(res, i) ((res)->files[i])
#define result_files(res) ((res)->files)
#define result_game(res) ((res)->game)
#define result_image(res, i) ((res)->images[i])
#define result_images(res) ((res)->images)
#define result_num_disks(res) ((res)->disks != NULL ? match_disk_array_length((res)->disks) : 0)
#define result_num_files(res) ((res)->files.size())
#define result_num_roms(res) ((res)->roms.size())
#define result_rom(res, i) (&(res)->roms[i])
#define result_roms(res) ((res)->roms)

#endif /* result.h */
