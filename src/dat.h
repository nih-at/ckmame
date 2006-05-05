#ifndef HAD_DAT_H
#define HAD_DAT_H

/*
  $NiH: dat.h,v 1.4 2006/03/17 16:46:01 dillo Exp $

  dat.h -- information about dat file
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



#include "array.h"

struct dat_entry {
    char *name;
    char *description;
    char *version;
};

typedef array_t dat_t;
typedef struct dat_entry dat_entry_t;



#define dat_free(d)		(array_free(d, dat_entry_finalize))
#define dat_entry_description(de)	((de)->description)
#define dat_entry_name(de)	((de)->name)
#define dat_entry_version(de)	((de)->version)
#define dat_get(d, i)		((dat_entry_t *)array_get((d), (i)))
#define dat_length		array_length
#define dat_description(d, i)	(dat_entry_description(dat_get((d), (i))))
#define dat_name(d, i)		(dat_entry_name(dat_get((d), (i))))
#define dat_new()		(array_new(sizeof(dat_entry_t)))
#define dat_version(d, i)	(dat_entry_version(dat_get((d), (i))))

void dat_entry_finalize(dat_entry_t *);
void dat_entry_merge(dat_entry_t *, const dat_entry_t *, const dat_entry_t *);
void dat_entry_init(dat_entry_t *);
void *dat_push(dat_t *, const dat_entry_t *, const dat_entry_t *);

#endif /* rom.h */
