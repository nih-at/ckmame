#ifndef _HAD_IMAGES_H
#define _HAD_IMAGES_H

/*
  $NiH$

  images.h -- array of disk images
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



#include "disk.h"
#include "game.h"
#include "parray.h"

typedef parray_t images_t;

#define images_free(im)		(parray_free((im), disk_free))
#define images_get(im, i)	((disk_t *)parray_get((im), (i)))
#define images_length(im)	(parray_length(im))



const char *images_name(const images_t *, int);
images_t *images_new(const game_t *);
images_t *images_new_name(const char  *, int);

#endif /* images.h */
