/*
  $NiH$

  result.c -- allocate/free result structure
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



#include "globals.h"
#include "result.h"
#include "xmalloc.h"



void
result_free(result_t *res)
{
    if (res == NULL)
	return;

    match_array_free(res->roms);
    file_status_array_free(res->files);
    match_disk_array_free(res->disks);
    file_status_array_free(res->disk_files);
    parray_free(res->disk_names, free);
    free(res);
}




result_t *
result_new(const game_t *g, const archive_t *a)
{
    result_t *res;
    int i;

    res = (result_t *)xmalloc(sizeof(*res));

    res->roms = match_array_new(game_num_files(g, file_type));
    res->files = file_status_array_new(archive_num_files(a));

    if (game_num_disks(g) > 0 && file_type == TYPE_ROM) {
	res->disks = match_disk_array_new(game_num_disks(g));
	res->disk_files = file_status_array_new(game_num_disks(g));
	res->disk_names = parray_new_sized(game_num_disks(g));
	for (i=0; i<game_num_disks(g); i++)
	    parray_push(res->disk_names, NULL);
    }
    else {
	res->disks = NULL;
	res->disk_files = NULL;
	res->disk_names = NULL;
    }

    return res;
}
