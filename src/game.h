#ifndef _HAD_GAME_H
#define _HAD_GAME_H

/*
  game.h -- information about one game
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


#include "array.h"
#include "disk.h"
#include "file.h"
#include "parray.h"

struct game {
    uint64_t id;
    char *name;
    char *description;
    int dat_no;
    char *cloneof[2];
    array_t *roms;
    array_t *disks;
};

typedef struct game game_t;


#define game_cloneof(g, i) ((g)->cloneof[i])
#define game_dat_no(g) ((g)->dat_no)
#define game_description(g) ((g)->description)

#define game_disk(g, i) ((disk_t *)array_get(game_disks(g), (i)))

#define game_disks(g) ((g)->disks)

#define game_file(g, i) ((file_t *)array_get(game_files(g), (i)))

#define game_files(g) ((g)->roms)

#define game_id(g) ((g)->id)
#define game_num_clones(g) (array_length(game_clones(g)))
#define game_num_disks(g) (array_length(game_disks(g)))
#define game_num_files(g) (array_length(game_files(g)))
#define game_name(g) ((g)->name)

void game_add_clone(game_t *, filetype_t, const char *);
void game_free(game_t *);
game_t *game_new(void);

#endif /* game.h */
