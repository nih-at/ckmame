#ifndef HAD_FILE_LOCATION_H
#define HAD_FILE_LOCATION_H

/*
  $NiH: file_location.h,v 1.2 2005/09/27 21:33:02 dillo Exp $

  file_location.h -- location of a file
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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



#include "hashes.h"
#include "types.h"



struct file_location {
    char *name;
    int index;
};

typedef struct file_location file_location_t;

struct file_location_ext {
    char *name;
    int index;
    where_t where;
};

typedef struct file_location_ext file_location_ext_t;



#define file_location_name(a)	((a)->name)
#define file_location_index(a)	((a)->index)

const char *file_location_make_key(filetype_t, const hashes_t *);
int file_location_default_hashtype(filetype_t);

int file_location_cmp(const file_location_t *, const file_location_t *);
void file_location_free(file_location_t *);
void file_location_finalize(file_location_t *);
file_location_t *file_location_new(const char *, int);

#define file_location_ext_name(a)	((a)->name)
#define file_location_ext_index(a)	((a)->index)
#define file_location_ext_where(a)	((a)->where)

void file_location_ext_free(file_location_ext_t *);
file_location_ext_t *file_location_ext_new(const char *, int, where_t);

#endif /* file_location.h */
