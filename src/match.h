#ifndef HAD_MATCH_H
#define HAD_MATCH_H

/*
  $NiH: match.h,v 1.1 2005/07/13 17:42:20 dillo Exp $

  match.h -- matching files with ROMs
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "parray.h"
#include "types.h"

struct match {
    quality_t quality;
    int index;
    off_t offset;              /* offset of correct part if QU_LONG */
};

typedef struct match match_t;

typedef array_t match_array_t;

#define match_array_get(ma, i)	\
	((match_t *)array_get((ma), (i))

#define match_array_length		array_length

#define match_copy(m1, m2)		(memcpy(m1, m2, sizeof(match_t)))
#define match_fno(m)			((m)->fno)
#define match_free			free
#define match_offset(m)			((m)->offset)
#define match_quality(m)		((m)->quality)
#define match_where(m)			((m)->where)
#define match_zno(m)			((m)->zno)
#define match_is_correct_place(m)	(match_where(m) == match_zno(m))



void match_array_free(match_array_t *);
match_array_t *match_array_new(int);
void match_merge(match_array_t *, archive_t **, int, int);
match_t *match_new(where_t, int, int, state_t, off_t);
int matchcmp(const match_t *, const match_t *);

#endif /* match.h */
