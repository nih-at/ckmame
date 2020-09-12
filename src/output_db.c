/*
  output-db.c -- write games to DB
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "file_location.h"
#include "output.h"
#include "romdb.h"
#include "xmalloc.h"


struct output_context_db {
    output_context_t output;

    romdb_t *db;

    dat_t *dat;

    parray_t *lost_children;
};

typedef struct output_context_db output_context_db_t;

struct fbh_context {
    sqlite3 *db;
    filetype_t ft;
};


static void familymeeting(romdb_t *, game_t *, game_t *);
static int handle_lost(output_context_db_t *);
static bool lost(output_context_db_t *, game_t *);
static int output_db_close(output_context_t *);
static int output_db_detector(output_context_t *, detector_t *);
static int output_db_game(output_context_t *, game_t *);
static int output_db_header(output_context_t *, dat_entry_t *);


output_context_t *
output_db_new(const char *dbname, int flags) {
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)xmalloc(sizeof(*ctx));

    ctx->output.close = output_db_close;
    ctx->output.output_detector = output_db_detector;
    ctx->output.output_game = output_db_game;
    ctx->output.output_header = output_db_header;

    ctx->dat = dat_new();
    ctx->lost_children = parray_new();

    remove(dbname);
    ctx->db = romdb_open(dbname, DBH_NEW);
    if (ctx->db == NULL) {
	output_db_close((output_context_t *)ctx);
	myerror(ERRDB, "can't create hash table");
	return NULL;
    }

    return (output_context_t *)ctx;
}


static void
familymeeting(romdb_t *db, game_t *parent, game_t *child) {
    int i, j;
    file_t *cr, *pr;

    if (game_cloneof(parent, 0)) {
	/* tell child of his grandfather */
	game_cloneof(child, 1) = xstrdup(game_cloneof(parent, 0));
    }

    /* look for ROMs in parent */
    for (i = 0; i < game_num_roms(child); i++) {
	cr = game_rom(child, i);
	for (j = 0; j < game_num_roms(parent); j++) {
	    pr = game_rom(parent, j);
	    if (file_mergeable(cr, pr)) {
		file_where(cr) = (where_t)(file_where(pr) + 1);
		break;
	    }
	}
	if (file_where(cr) == FILE_INGAME && file_merge(cr) != NULL) {
	    myerror(ERRFILE, "In game '%s': '%s': merged from '%s', but parent does not contain matching file", game_name(child), file_name(cr), file_merge(cr));
	}
    }
    for (i = 0; i < game_num_disks(child); i++) {
        disk_t *cd = game_disk(child, i);
        for (j = 0; j < game_num_disks(parent); j++) {
            disk_t *pd = game_disk(parent, j);
            if (disk_mergeable(cd, pd)) {
                disk_where(cd) = (where_t)(disk_where(pd) + 1);
            }
        }
    }

    return;
}


static int
handle_lost(output_context_db_t *ctx) {
    game_t *child, *parent;
    int i;
    bool is_lost;

    while (parray_length(ctx->lost_children) > 0) {
	/* processing order does not matter and deleting last
	   element is cheaper */
	for (i = parray_length(ctx->lost_children) - 1; i >= 0; --i) {
	    /* get current lost child from database, get parent,
	       look if parent is still lost, if not, do child */
	    if ((child = romdb_read_game(ctx->db, parray_get(ctx->lost_children, i))) == NULL) {
		myerror(ERRDEF, "internal database error: child %s not in database", parray_get(ctx->lost_children, i));
		return -1;
	    }

            is_lost = true;

            if ((parent = romdb_read_game(ctx->db, game_cloneof(child, 0))) == NULL) {
                myerror(ERRDEF, "inconsistency: %s has non-existent parent %s", game_name(child), game_cloneof(child, 0));
                
                /* remove non-existent cloneof */
                free(game_cloneof(child, 0));
                game_cloneof(child, 0) = NULL;
                romdb_update_game_parent(ctx->db, child);
                is_lost = false;
            }
            else if (!lost(ctx, parent)) {
                /* parent found */
                familymeeting(ctx->db, parent, child);
                is_lost = false;
            }
            
            if (!is_lost) {
                romdb_update_file_location(ctx->db, child);
                parray_delete(ctx->lost_children, i, free);
            }
            game_free(parent);
            game_free(child);
        }
    }

    return 0;
}


static bool
lost(output_context_db_t *ctx, game_t *g) {
    int i;

    if (game_cloneof(g, 0) == NULL)
	return false;

    for (i = 0; i < parray_length(ctx->lost_children); i++) {
	if (strcmp(parray_get(ctx->lost_children, i), game_name(g)) == 0) {
            return true;
	}
    }

    return false;
}


static int
output_db_close(output_context_t *out) {
    output_context_db_t *ctx;
    int ret;

    ctx = (output_context_db_t *)out;

    ret = 0;

    if (ctx->db) {
	romdb_write_dat(ctx->db, ctx->dat);

	if (handle_lost(ctx) < 0)
	    ret = -1;

	if (sqlite3_exec(romdb_sqlite3(ctx->db), sql_db_init_2, NULL, NULL, NULL) != SQLITE_OK)
	    ret = -1;

	romdb_close(ctx->db);
    }

    dat_free(ctx->dat);
    parray_free(ctx->lost_children, free);

    free(ctx);

    return ret;
}


static int
output_db_detector(output_context_t *out, detector_t *d) {
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)out;

    if (romdb_write_detector(ctx->db, d) != 0) {
	seterrdb(romdb_dbh(ctx->db));
	myerror(ERRDB, "can't write detector to db");
	return -1;
    }

    return 0;
}


static int
output_db_game(output_context_t *out, game_t *g) {
    output_context_db_t *ctx;
    game_t *g2, *parent;

    ctx = (output_context_db_t *)out;

    if ((g2 = romdb_read_game(ctx->db, game_name(g))) != NULL) {
	myerror(ERRDEF, "duplicate game '%s' skipped", game_name(g));
	game_free(g2);
	return -1;
    }

    game_dat_no(g) = dat_length(ctx->dat) - 1;

    if (game_cloneof(g, 0)) {
        if (((parent = romdb_read_game(ctx->db, game_cloneof(g, 0))) == NULL) || lost(ctx, parent)) {
            parray_push(ctx->lost_children, xstrdup(game_name(g)));
        }
        else {
            familymeeting(ctx->db, parent, g);
            /* TODO: check error */
        }
        game_free(parent);
    }

    if (romdb_write_game(ctx->db, g) != 0) {
	myerror(ERRDB, "can't write game '%s' to db", game_name(g));
	return -1;
    }

    return 0;
}


static int
output_db_header(output_context_t *out, dat_entry_t *dat) {
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)out;

    dat_push(ctx->dat, dat, NULL);

    return 0;
}
