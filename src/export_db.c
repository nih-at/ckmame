/*
  $NiH: export_db.c,v 1.2 2006/10/04 17:36:43 dillo Exp $

  export_db.c -- export games from db to output backend
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include "dbh.h"
#include "error.h"
#include "parse.h"



int
export_db(DB *db, const parray_t *exclude, const dat_entry_t *dat,
	  output_context_t *out)
{
    parray_t *list;
    int i;
    game_t *g;
    dat_entry_t de;
    dat_t *db_dat;

    db_dat = r_dat(db);

    if (out == NULL) {
	/* XXX: split into original dat files */
	return 0;
    }

    /* XXX: export detector */
    
    dat_entry_merge(&de, dat,
		    ((db_dat && dat_length(db_dat) == 1)
		     ? dat_get(db_dat, 0) : NULL));
    output_header(out, &de);
    dat_entry_finalize(&de);
    dat_free(db_dat);

    if ((list=r_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "db error reading game list");
	return -1;
    }

    for (i=0; i<parray_length(list); i++) {
	if ((g=r_game(db, parray_get(list, i))) == NULL) {
	    /* XXX: error */
	    continue;
	}
	if (!name_matches(g, exclude))
	    output_game(out, g);
	game_free(g);
    }

    return 0;
}
