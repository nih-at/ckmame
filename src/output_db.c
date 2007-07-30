/*
  $NiH: output_db.c,v 1.6 2006/10/04 17:36:44 dillo Exp $

  output-db.c -- write games to DB
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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

#include "dbh.h"
#include "error.h"
#include "file_location.h"
#include "output.h"
#include "xmalloc.h"



struct output_context_db {
    output_context_t output;

    sqlite3 *db;

    dat_t *dat;
    
    parray_t *lost_children;
    array_t *lost_children_types;
};

typedef struct output_context_db output_context_db_t;

struct fbh_context {
    sqlite3 *db;
    filetype_t ft;
};



static void familymeeting(sqlite3 *, filetype_t, game_t *, game_t *);
static int handle_lost(output_context_db_t *);
static int lost(game_t *, filetype_t);
static int output_db_close(output_context_t *);
static int output_db_detector(output_context_t *, detector_t *);
static int output_db_game(output_context_t *, game_t *);
static int output_db_header(output_context_t *, dat_entry_t *);



output_context_t *
output_db_new(const char *dbname)
{
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)xmalloc(sizeof(*ctx));

    ctx->output.close = output_db_close;
    ctx->output.output_detector = output_db_detector;
    ctx->output.output_game = output_db_game;
    ctx->output.output_header = output_db_header;

    ctx->dat=dat_new();
    ctx->lost_children = parray_new();
    ctx->lost_children_types = array_new(sizeof(int));

    remove(dbname);
    ctx->db = dbh_open(dbname, DBL_NEW);
    if (ctx->db == NULL) {
	output_db_close((output_context_t *)ctx);
	myerror(ERRDB, "can't create hash table");
	return NULL;
    }

    return (output_context_t *)ctx;
}



static void
familymeeting(sqlite3 *db, filetype_t ft, game_t *parent, game_t *child)
{
    int i, j;
    file_t * cr, *pr;

    if (game_cloneof(parent, ft, 0)) {
       /* tell child of his grandfather */
       game_cloneof(child, ft, 1) = xstrdup(game_cloneof(parent, ft, 0));
    }


    /* look for ROMs in parent */
    for (i=0; i<game_num_files(child, ft); i++) {
	cr = game_file(child, ft, i);
	for (j=0; j<game_num_files(parent, ft); j++) {
	    pr = game_file(parent, ft, j);
	    if (file_compare_msc(cr, pr) == 0) {
		file_where(cr) = (where_t)(file_where(pr) + 1);
		break;
	    }
	}
    }

    return;
}



static int
handle_lost(output_context_db_t *ctx)
{
    game_t *child, *parent;
    int i, ft, types, old_types;

    while (parray_length(ctx->lost_children) > 0) {
	/* processing order does not matter and deleting last
	   element is cheaper*/
	for (i=parray_length(ctx->lost_children)-1; i>=0; --i) {
	    /* get current lost child from database, get parent,
	       look if parent is still lost, if not, do child */
	    if ((child=r_game(ctx->db,
			      parray_get(ctx->lost_children, i))) == NULL) {
		myerror(ERRDEF,
			"internal database error: child not in database");
		return -1;
	    }
	    
	    types = *((int *)array_get(ctx->lost_children_types, i));
	    old_types = types;
	    
	    for (ft = 0; ft<GAME_RS_MAX; ft++) {
		if ((types & (1<<ft)) == 0)
		    continue;

		if ((parent=r_game(ctx->db,
				   game_cloneof(child, ft, 0))) == NULL) {
		    myerror(ERRDEF,
			    "inconsistency: %s has non-existent parent %s",
			    game_name(child), game_cloneof(child, ft, 0));
		    
		    /* remove non-existent cloneof */
		    free(game_cloneof(child, ft, 0));
		    game_cloneof(child, ft, 0) = NULL;
		    u_game_parent(ctx->db, child, ft);
		    types &= ~(1<<ft);
		    continue;
		}
		
		if (lost(parent, (filetype_t)ft)) {
		    game_free(parent);
		    continue;
		}

		/* parent found */
		familymeeting(ctx->db, (filetype_t)ft, parent, child);
		game_free(parent);
		types &= ~(1<<ft);
	    }

	    if (types != old_types)
		u_game(ctx->db, child);
	    game_free(child);

	    if (types == 0) {
		parray_delete(ctx->lost_children, i, free);
		array_delete(ctx->lost_children_types, i, NULL);
	    }
	    else
		array_set(ctx->lost_children_types, i, &types);
	}
    }

    return 0;
}



static int
lost(game_t *g, filetype_t ft)
{
    int i;

    if (game_cloneof(g, ft, 0) == NULL)
	return 0;

    for (i=0; i<game_num_files(g, ft); i++)
	if (file_where(game_file(g, ft, i)) != FILE_INZIP)
	    return 0;

    return 1;
}



static int
output_db_close(output_context_t *out)
{
    output_context_db_t *ctx;
    int ret;

    ctx = (output_context_db_t *)out;

    ret = 0;

    if (ctx->db) {
	w_dat(ctx->db, ctx->dat);

	if (handle_lost(ctx) < 0)
	    ret = -1;

	if (sqlite3_exec(ctx->db, sql_db_init_2, NULL, NULL, NULL)
	    != SQLITE_OK)
	    ret = -1;
	
	dbh_close(ctx->db);
    }

    dat_free(ctx->dat);
    parray_free(ctx->lost_children, free);
    array_free(ctx->lost_children_types, NULL);

    free(ctx);

    return ret;
}



static int
output_db_detector(output_context_t *out, detector_t *d)
{
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)out;

    if (w_detector(ctx->db, d) != 0) {
	seterrdb(ctx->db);
	myerror(ERRDB, "can't write detector to db");
	return -1;
    }

    return 0;
}    



static int
output_db_game(output_context_t *out, game_t *g)
{
    output_context_db_t *ctx;
    int i, to_do;
    game_t *g2, *parent;

    ctx = (output_context_db_t *)out;

    if ((g2=r_game(ctx->db, game_name(g))) != NULL) {
	myerror(ERRDEF, "duplicate game ``%s'' skipped", game_name(g));
	game_free(g2);
	return -1;
    }

    game_dat_no(g) = dat_length(ctx->dat)-1;

    to_do = 0;
    for (i=0; i<GAME_RS_MAX; i++) {
	if (game_cloneof(g, i, 0)) {
	    if (((parent=r_game(ctx->db, game_cloneof(g, i, 0))) == NULL)
		|| lost(parent, (filetype_t)i)) {
		to_do |= 1<<i;
		if (parent)
		    game_free(parent);
	    }
	    else {
		familymeeting(ctx->db, (filetype_t)i, parent, g);
		/* XXX: check error */
		game_free(parent);
	    }
	}
    }
    
    if (to_do) {
	parray_push(ctx->lost_children, xstrdup(game_name(g)));
	array_push(ctx->lost_children_types, &to_do);
    }
    
    if (w_game(ctx->db, g) != 0) {
	myerror(ERRDB, "can't write game `%s' to db", game_name(g));
	return -1;
    }

    return 0;
}



static int
output_db_header(output_context_t *out, dat_entry_t *dat)
{
    output_context_db_t *ctx;

    ctx = (output_context_db_t *)out;

    dat_push(ctx->dat, dat, NULL);

    return 0;
}
