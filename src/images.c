/*
  $NiH$

  images.c -- array of disk images
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



#include "images.h"
#include "funcs.h"



const char *
images_name(const images_t *im, int i)
{
    disk_t *d;
    
    d = images_get(im, i);

    return d ? disk_name(d) : NULL;
}



images_t *
images_new(const game_t *g)
{
    images_t *im;
    char *fname;
    int i;

    if (game_num_disks(g) == 0)
	return NULL;

    im = parray_new_sized(game_num_disks(g));

    for (i=0; i<game_num_disks(g); i++) {
	fname = findfile(disk_name(game_disk(g, i)), TYPE_DISK);
	if (fname == NULL)
	    parray_push(im, NULL);
	else {
	    parray_push(im, disk_new(fname, 0));
	    free(fname);
	}
    }

    return im;
}



images_t *
images_new_name(const char *name, int quiet)
{
    images_t *im;

    im = parray_new_sized(1);

    parray_push(im, disk_new(name, quiet));

    return im;
}
