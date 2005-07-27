/*
  $NiH: match.c,v 1.5 2005/07/13 17:42:20 dillo Exp $

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



void
match_finalize(match_t *m)
{
    if (match_where(m) == ROM_ELSEWHERE)
	archive_free(match_archive(m));
}



void
match_init(match_t *m)
{
    m->quality = QU_MISSING;
    m->archive = NULL;
    m->where = ROM_NOWHERE;
    m->index = -1;
    m->offset = -1;
}



#if 0
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
#endif



#if 0
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
#endif
