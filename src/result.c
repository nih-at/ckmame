/*
  $NiH: result.c,v 1.3 2006/05/01 21:09:11 dillo Exp $

  result.c -- allocate/free result structure
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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
    file_status_array_free(res->images);
    free(res);
}




result_t *
result_new(const game_t *g, const archive_t *a, const images_t *im)
{
    result_t *res;

    res = (result_t *)xmalloc(sizeof(*res));

    result_game(res) = GS_MISSING;
    result_roms(res) = NULL;
    result_files(res) = NULL;
    result_disks(res) = NULL;
    result_images(res) = NULL;
    
    if (g) {
	res->roms = match_array_new(game_num_files(g, file_type));

	if (game_num_disks(g) > 0 && file_type == TYPE_ROM)
	    result_disks(res) = match_disk_array_new(game_num_disks(g));
    }
    
    if (a)
	result_files(res) = file_status_array_new(archive_num_files(a));

    if (im)
	result_images(res) = file_status_array_new(images_length(im));

    return res;
}
