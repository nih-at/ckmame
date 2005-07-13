/*
  $NiH: match.c,v 1.4 2005/07/07 22:00:20 dillo Exp $

  match.c -- information about ROM/file matches
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "dbl.h"
#include "error.h"
#include "hashes.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static void match_array_free_one(parray_t *);



void
match_array_free(match_array_t *ma)
{
    parray_free(ma, match_array_free_one);
}



match_array_t *
match_array_new(int n)
{
    match_array_t *ma;
    parray_t *pa;
    match_t *m;
    int i;

    ma = parray_new_sized(n);

    for (i=0; i<n; i++) {
	m = match_new(ROM_INZIP, -1, -1, ROM_UNKNOWN, -1);
	pa = parray_new();
	parray_push(pa, m);
	parray_push(ma, pa);
    }

    return ma;
}



void
match_merge(match_array_t *ma, archive_t **zip, int pno, int gpno)
{
    int zno[3], i, j;
    match_t *mm, *m;
    rom_t *r;
    
    /* update zip structures */

    zno[0] = 0;
    zno[1] = pno;
    zno[2] = gpno;

    for (i=0; i<match_array_length(ma); i++) {
	m = match_array_get(ma, i, 0);
	if (match_quality(m) > ROM_UNKNOWN) {
	    r = archive_file(zip[match_zno(m)], match_fno(m));
	    if (rom_state(r) < ROM_TAKEN || match_zno(m) == 0) {
		rom_state(r) = ROM_TAKEN;
		rom_where(r) = (where_t)match_zno(m);
	    }
	}
	
	for (j=1; j<match_array_num_matches(ma, i); j++) {
	    mm = match_array_get(ma, i, j);
	    r = archive_file(zip[match_zno(mm)], match_fno(mm));
	    if (match_quality(mm) > ROM_UNKNOWN
		&& match_quality(mm) > rom_state(r)) {
		rom_state(r) = match_quality(mm);
		rom_where(r) = (where_t)match_zno(mm);
	    }
	}
    }

    return;
}



match_t *
match_new(where_t where, int zno, int fno, state_t quality, off_t offset)
{
    match_t *m;

    m = xmalloc(sizeof(*m));

    m->where = where;
    m->zno = zno;
    m->fno = fno;
    m->quality = quality;
    m->offset = (quality == ROM_LONGOK ? offset : -1);

    return m;
}



/* <0: m1 is better than m2 */

int
matchcmp(const match_t *m1, const match_t *m2)
{
    int ret;

    ret = match_quality(m2) - match_quality(m1);

    if (ret == 0)
	ret = match_is_correct_place(m2) - match_is_correct_place(m1);

    return ret;
}



static void
match_array_free_one(parray_t *pa)
{
    parray_free(pa, match_free);
}
