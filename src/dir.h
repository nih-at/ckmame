#ifndef HAD_DIR_H
#define HAD_DIR_H

/*
  $NiH: dir.h,v 1.2 2005/09/27 21:33:02 dillo Exp $

  dir.h -- reading a directory
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



#include "parray.h"

#define DIR_RECURSE	1

struct dir {
    int flags;
    parray_t *stack;
};

typedef struct dir dir_t;

enum dir_status {
    DIR_ERROR = -1,
    DIR_OK,
    DIR_EOD
};

typedef enum dir_status dir_status_t;

int dir_close(dir_t *);
dir_status_t dir_next(dir_t *, char *, int);
dir_t *dir_open(const char *, int);

#endif /* dir.h */
