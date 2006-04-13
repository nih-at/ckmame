#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  $NiH: funcs.h,v 1.8 2006/04/05 22:36:03 dillo Exp $

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
#include "game.h"
#include "parray.h"
#include "result.h"
#include "tree.h"

void check_archive(archive_t *, const char *, result_t *);
void check_disks(game_t *, result_t *);
void check_files(game_t *, archive_t *[], result_t *);
void diagnostics(const game_t *, const archive_t *, const result_t *);
void ensure_extra_maps(void);
void ensure_needed_maps(void);
int fix_game(game_t *, archive_t *, result_t *);
parray_t *find_superfluous(const char *);
char *make_needed_name(const rom_t *);
char *make_needed_name_disk(const disk_t *);
char *make_unique_name(const char *, const char *, ...);
struct zip *my_zip_open(const char *, int);
int my_zip_rename(struct zip *, int, const char *);
void print_superfluous(const parray_t *);
int rename_or_move(const char *, const char *);

#endif /* funcs.h */
