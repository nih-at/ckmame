#ifndef HAD_RESULT_H
#define HAD_RESULT_H

/*
  $NiH: result.h,v 1.1 2006/04/05 22:39:14 dillo Exp $

  result.h -- result of game check
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_status.h"
#include "game.h"
#include "match.h"
#include "match_disk.h"
#include "parray.h"

struct result {
    game_status_t game;
    match_array_t *roms;
    file_status_array_t *files;

    match_disk_array_t *disks;
    file_status_array_t *disk_files;
    parray_t *disk_names;
};

typedef struct result result_t;


#define result_disk(res, i)	(match_disk_array_get(result_disks(res), (i)))
#define result_disks(res)	((res)->disks)
#define result_disk_file(res, i)	\
	(file_status_array_get(result_disk_files(res), (i)))
#define result_disk_files(res)	((res)->disk_files)
#define result_disk_name(res, i)	\
	(parray_get(result_disk_names(res), (i)))
#define result_disk_names(res)	((res)->disk_names)
#define result_file(res, i)	(file_status_array_get(result_files(res), (i)))
#define result_files(res)	((res)->files)
#define result_game(res) 	((res)->game)
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
result_t *result_new(const game_t *, const archive_t *);

#endif /* result.h */
