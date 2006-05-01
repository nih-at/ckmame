/*
  $NiH: check_old.c,v 1.1 2006/04/24 11:38:38 dillo Exp $

  check_old.c -- search ROMs in old db
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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



#include "find.h"
#include "funcs.h"
#include "globals.h"



void
check_old(game_t *g, result_t *res)
{
    int i;
    int all_old;

    if (old_db == NULL)
	return;

    all_old = 1;
    for (i=0; i<game_num_files(g, file_type); i++) {
	if (find_in_old(game_file(g, file_type, i), result_rom(res, i))
	    != FIND_EXISTS)
	    all_old = 0;
    }

    if (all_old)
	result_game(res) = GS_OLD;

    for (i=0; i<game_num_disks(g); i++)
	find_disk_in_old(game_disk(g, i), result_disk(res, i));
}
