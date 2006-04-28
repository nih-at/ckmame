#ifndef HAD_GARBAGE_H
#define HAD_GARBAGE_H

/*
  $NiH$

  garbage.h -- move files to garbage directory
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



#include <zip.h>

#include "archive.h"

struct garbage {
    archive_t *a;
    char *zname;
    struct zip *za;
};

typedef struct garbage garbage_t;



int garbage_add(garbage_t *, int);
int garbage_close(garbage_t *);
garbage_t *garbage_new(archive_t *);

#endif /* garbage.h */
