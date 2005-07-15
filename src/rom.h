#ifndef HAD_ROM_H
#define HAD_ROM_H

/*
  $NiH: rom.h,v 1.1 2005/07/13 17:42:20 dillo Exp $

  rom.h -- information about one rom
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
#include "parray.h"

struct rom {
    char *name;
    char *merge;
    hashes_t hashes;
    unsigned long size;
    flags_t flags;
    state_t state;
    where_t where;
    parray_t *altnames;
};

typedef struct rom rom_t;



#define rom_altname(r, i)	((char *)parray_get(rom_altnames(r), (i)))
#define rom_altnames(r)		((r)->altnames)
#define rom_hashes(r)		(&(r)->hashes)
#define rom_flags(r)		((r)->flags)
#define rom_merge(r)		((r)->merge)
#define rom_name(r)		((r)->name)
#define rom_num_altnames(r)	(rom_altnames(r) ? \
				 parray_length(rom_altnames(r)) : 0)
#define rom_size(r)		((r)->size)
#define rom_state(r)		((r)->state)
#define rom_where(r)		((r)->where)

#define rom_compare_n(r1, r2)	(strcasecmp(rom_name(r1), rom_name(r2)))
#define rom_compare_sc(r1, r2)						    \
	(rom_size(r1) != rom_size(r2)					    \
	 || hashes_cmp(rom_hashes(r1), rom_hashes(r2)) != HASHES_CMP_MATCH)
#define rom_compare_nsc(r1, r2)						\
	(rom_compare_n((r1), (r2)) || rom_compare_sc((r1), (r2)))

void rom_add_altname(rom_t *, const char *);
void rom_init(rom_t *);
void rom_finalize(rom_t *);
state_t romcmp(const rom_t *, const rom_t *, int);

#endif /* rom.h */
