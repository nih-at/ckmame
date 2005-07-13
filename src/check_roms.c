/*
  $NiH: match.c,v 1.4 2005/07/07 22:00:20 dillo Exp $

  check_roms.c -- match files against ROMs
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



#include "archive.h"
#include "funcs.h"
#include "game.h"
#include "match.h"

static void process_zip(const game_t *, filetype_t,
			archive_t *, int zno, match_array_t *);



match_array_t *
check_roms(game_t *game, archive_t **zip, filetype_t ft)
{
    int i, zno[3];
    match_array_t *ma;

    ma = match_array_new(game_num_files(game, ft));

    for (i=0; i<3; i++) {
	if (zip[i] && archive_name(zip[i])) {
	    process_zip(game, ft, zip[i], i, ma);
	    zno[i] = archive_num_files(zip[i]);
	}
	else
	    zno[i] = 0;
    }

    marry(ma, zno);

    return ma;
}



static void
process_zip(const game_t *game, filetype_t ft,
	    archive_t *zip, int zno, match_array_t *ma)
{
    int i, j, offset;
    state_t st;
    rom_t *r;
    
    for (i=0; i<game_num_files(game, ft); i++) {
	r = game_file(game, ft, i);
	for (j=0; j<archive_num_files(zip); j++) {
	    st = romcmp(archive_file(zip, j), r, (zno > 0));
	    if (st == ROM_LONG) {
		offset = archive_find_offset(zip, j,
					     rom_size(r), rom_hashes(r));
		if (offset != -1)
		    st = ROM_LONGOK;
	    }
	    
	    if (st >= ROM_NAMERR)
		match_array_add(ma, i, match_new(rom_where(r),
						 zno, j, st, offset));
	}
    }
}
