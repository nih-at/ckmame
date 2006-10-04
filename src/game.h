#ifndef _HAD_GAME_H
#define _HAD_GAME_H

/*
  $NiH: game.h,v 1.3 2006/05/05 10:38:51 dillo Exp $

  game.h -- information about one game
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "array.h"
#include "disk.h"
#include "parray.h"
#include "rom.h"

struct rs {
    char *cloneof[2];
    parray_t *clones;
    array_t *files;
};

#define GAME_RS_MAX	2

struct game {
    char *name;
    char *description;
    int dat_no;
    struct rs rs[GAME_RS_MAX];
    array_t *disks;
};

typedef struct game game_t;



#define game_clone(g, ft, i)		\
	((char *)parray_get(game_clones((g), (ft)), (i)))

#define game_clone_index(g, ft, n)	\
	parray_index_sorted(game_clones((g), (ft)), (n), (int (*)())strcmp)

#define game_cloneof(g, ft, i)		((g)->rs[ft].cloneof[i])
#define game_clones(g, ft)		((g)->rs[ft].clones)
#define game_dat_no(g)			((g)->dat_no)
#define game_description(g)		((g)->description)

#define game_disk(g, i)			\
	((disk_t *)array_get(game_disks(g), (i)))

#define game_disks(g)			((g)->disks)

#define game_file(g, ft, i)   		\
	((rom_t *)array_get(game_files((g), (ft)), (i)))

#define game_files(g, ft)		((g)->rs[ft].files)

#define game_num_clones(g, ft)		(array_length(game_clones((g), (ft))))
#define game_num_disks(g)		(array_length(game_disks(g)))
#define game_num_files(g, ft)		(array_length(game_files((g), (ft))))
#define game_name(g)			((g)->name)

void game_add_clone(game_t *, filetype_t, const char *);
void game_free(game_t *);
game_t *game_new(void);

#endif /* game.h */
