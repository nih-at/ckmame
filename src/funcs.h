#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  $NiH: funcs.h,v 1.2 2005/07/13 17:42:20 dillo Exp $

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



#include "archive.h"
#include "dbl.h"
#include "game.h"
#include "match.h"
#include "match_disk.h"
#include "tree.h"

match_disk_array_t *check_disks(game_t *);
int check_roms(game_t *, filetype_t, archive_t []);
void diagnostics(const game_t *, filetype_t, const match_array_t *,
		 const match_disk_array_t *, const archive_t **);
int fix_game(game_t *, filetype_t, archive_t **, match_array_t *);
int handle_extra_files(DB *, const char *, int);
void marry(match_array_t *, const int *);

#endif /* funcs.h */
