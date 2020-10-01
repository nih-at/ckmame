/*
  export_db.c -- export games from db to output backend
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "error.h"
#include "parse.h"
#include <stdlib.h>

int
export_db(romdb_t *db, const parray_t *exclude, const dat_entry_t *dat, output_context_t *out) {
    parray_t *list;
    int i;
    game_t *g;
    dat_entry_t de;
    dat_t *db_dat;

    if (out == NULL) {
	/* TODO: split into original dat files */
	return 0;
    }

    db_dat = romdb_read_dat(db);

    /* TODO: export detector */

    dat_entry_merge(&de, dat, ((db_dat && dat_length(db_dat) == 1) ? dat_get(db_dat, 0) : NULL));
    output_header(out, &de);
    dat_entry_finalize(&de);
    dat_free(db_dat);

    if ((list = romdb_read_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "db error reading game list");
	return -1;
    }

    for (i = 0; i < parray_length(list); i++) {
	if ((g = romdb_read_game(db, static_cast<const char *>(parray_get(list, i)))) == NULL) {
	    /* TODO: error */
	    continue;
	}
	if (!name_matches(game_name(g), exclude))
	    output_game(out, g);
	game_free(g);
    }

    parray_free(list, free);
    return 0;
}
