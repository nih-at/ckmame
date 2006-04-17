#ifndef HAD_DISK_H
#define HAD_DISK_H

/*
  $NiH: disk.h,v 1.3 2005/10/02 11:28:10 dillo Exp $

  disk.h -- information about one disk
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

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



#include "hashes.h"
#include "types.h"

struct disk {
    int refcount;
    char *name;
    char *merge;
    hashes_t hashes;
    status_t status;
};

typedef struct disk disk_t;



#define disk_hashes(d)	(&(d)->hashes)
#define disk_status(d)	((d)->status)
#define disk_merge(d)	((d)->merge)
#define disk_name(d)	((d)->name)

void disk_finalize(disk_t *);
void disk_free(disk_t *);
void disk_init(disk_t *);
disk_t *disk_new(const char *, int);
void disk_real_free(disk_t *);

#endif /* disk.h */
