#ifndef HAD_MATCH_DISK_H
#define HAD_MATCH_DISK_H

/*
  $NiH: match_disk.h,v 1.3 2006/05/01 21:09:11 dillo Exp $

  match_disk.h -- matching files with disks
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

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



#include "disk.h"
#include "parray.h"
#include "types.h"

struct match_disk {
    char *name;
    hashes_t hashes;
    quality_t quality;
    where_t where;
};

typedef struct match_disk match_disk_t;

typedef array_t match_disk_array_t;

#define match_disk_array_free(ma)	\
	(array_free((ma), (void (*)())match_disk_finalize))
#define match_disk_array_get(ma, i)	\
	((match_disk_t *)array_get((ma), (i)))
#define match_disk_array_new(n)		\
	(array_new_length(sizeof(match_disk_t), n,	\
			  (void (*)())match_disk_init))

#define match_disk_array_length	array_length

#define match_disk_hashes(m)	(&(m)->hashes)
#define match_disk_name(m)	((m)->name)
#define match_disk_quality(m)	((m)->quality)
#define match_disk_where(m)	((m)->where)



void match_disk_finalize(match_disk_t *);
void match_disk_init(match_disk_t *);
void match_disk_set_source(match_disk_t *, const disk_t *);

#endif /* match_disk.h */
