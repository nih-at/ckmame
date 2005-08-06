#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  $NiH: funcs.h,v 1.2.2.5 2005/07/31 21:13:02 dillo Exp $

  funcs.h -- tree functions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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



#include <zip.h>

#include "archive.h"
#include "dbl.h"
#include "file_status.h"
#include "game.h"
#include "match.h"
#include "match_disk.h"
#include "parray.h"
#include "tree.h"

file_status_array_t *check_archive(archive_t *, match_array_t *);
match_disk_array_t *check_disks(game_t *);
match_array_t *check_files(game_t *, archive_t *[]);
void diagnostics(const game_t *, const archive_t *, const match_array_t *,
		 const match_disk_array_t *, const file_status_array_t *);
void enter_archive_in_map(map_t *, const archive_t *, where_t);
void ensure_extra_file_map(void);
void ensure_needed_map(void);
int fix_game(game_t *, archive_t *, match_array_t *, match_disk_array_t *,
	     file_status_array_t *);
parray_t *find_extra_files(const char *);
char *make_needed_name(const rom_t *);
struct zip *my_zip_open(const char *, int);
void print_extra_files(const parray_t *);

#endif /* funcs.h */
