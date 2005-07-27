#ifndef HAD_ROM_H
#define HAD_ROM_H

/*
  $NiH: rom.h,v 1.1.2.3 2005/07/20 00:26:44 dillo Exp $

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
#include "map.h"
#include "types.h"
#include "parray.h"

struct rom {
    char *name;
    char *merge;
    hashes_t hashes;
    unsigned long size;
    status_t status;
    where_t where;
    parray_t *altnames;
};

typedef struct rom rom_t;



#define rom_altname(r, i)	((char *)parray_get(rom_altnames(r), (i)))
#define rom_altnames(r)		((r)->altnames)
#define rom_hashes(r)		(&(r)->hashes)
#define rom_merge(r)		((r)->merge)
#define rom_name(r)		((r)->name)
#define rom_num_altnames(r)	(rom_altnames(r) ? \
				 parray_length(rom_altnames(r)) : 0)
#define rom_size(r)		((r)->size)
#define rom_status(r)		((r)->status)
#define rom_where(r)		((r)->where)

#define rom_compare_n(r1, r2)	(compare_names(rom_name(r1), rom_name(r2)))
#define rom_compare_nsc(r1, r2)						\
	(rom_compare_n((r1), (r2)) || rom_compare_sc((r1), (r2)))
#define rom_compare_sc(rg, ra)					\
	(!(rom_size(rg) == 0					\
	   || (rom_size(rg) == rom_size(ra)			\
	       && hashes_cmp(rom_hashes(rg), rom_hashes(ra))	\
	       == HASHES_CMP_MATCH)))

void rom_add_altname(rom_t *, const char *);
void rom_init(rom_t *);
void rom_finalize(rom_t *);
state_t romcmp(const rom_t *, const rom_t *, int);

#endif /* rom.h */
