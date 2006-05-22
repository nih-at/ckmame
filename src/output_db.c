/*
  $NiH$

  output-db.c -- write games to DB
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <stdlib.h>

#include "dbh.h"
#include "error.h"
#include "file_location.h"
#include "map.h"
#include "output.h"
#include "w.h"
#include "xmalloc.h"



struct output_context_db {
    output_context_t output;

    DB *db;

    dat_t *dat;
    
    map_t *map_rom;
    map_t *map_disk;
    int romhashtypes;
    int diskhashtypes;

    parray_t *lost_children;
    array_t *lost_children_types;

    parray_t *list[TYPE_MAX];
};

typedef struct output_context_db output_context_db_t;

struct fbh_context {
    DB *db;
    filetype_t ft;
};



static void enter_file_hash(map_t *, filetype_t, const char *,
			    int, const hashes_t *);
static void familymeeting(DB *, filetype_t, game_t *, game_t *);
static int file_location_copy(const hashes_t *, parray_t *, void *);
static int handle_lost(output_context_db_t *);
static int lost(game_t *, filetype_t);
static int output_db_close(output_context_t *);
static int output_db_game(output_context_t *, game_t *);
static int output_db_header(output_context_t *, dat_entry_t *);
static int write_hashes(output_context_db_t *);
static int write_hashtypes(output_context_db_t *);
static int write_lists(output_context_db_t *);



output_context_t *
output_db_new(const char *dbname)
{
    output_context_db_t *ctx;
    int i;

    ctx = (output_context_db_t *)xmalloc(sizeof(*ctx));

    ctx->output.close = output_db_close;
    ctx->output.output_game = output_db_game;
    ctx->output.output_header = output_db_header;

    ctx->dat=dat_new();
    ctx->map_rom = NULL;
    ctx->map_disk = NULL;
    ctx->romhashtypes = ctx->diskhashtypes = 0;
    ctx->lost_children = parray_new();
    ctx->lost_children_types = array_new(sizeof(int));
    for (i=0; i<TYPE_MAX; i++)
	ctx->list[i] = parray_new();
    
    if ((ctx->map_rom=map_new()) == NULL) {
	myerror(ERRDB, "can't create hash table");
	output_db_close((output_context_t *)ctx);
	return NULL;
    }
    if ((ctx->map_disk=map_new()) == NULL) {
	myerror(ERRDB, "can't create hash table");
	output_db_close((output_context_t *)ctx);
	return NULL;
    }

    remove(dbname);
    ctx->db = dbh_open(dbname, DBL_WRITE);
    if (ctx->db == NULL) {
	output_db_close((output_context_t *)ctx);
	myerror(ERRDB, "can't create hash table");
	return NULL;
    }

    w_version(ctx->db);

    return (output_context_t *)ctx;
}



static void
enter_file_hash(map_t *map, filetype_t filetype,
		const char *name, int index, const hashes_t *hashes)
{
    int type;

    type = file_location_default_hashtype(filetype);

    if (hashes_has_type(hashes, type)) {
	if (map_add(map, type, hashes,
		    file_location_new(name, index)) < 0) {
	    /* XXX: error */
	}
    }
    else {
	/* XXX: handle non-existent */
    }
}



static void
familymeeting(DB *db, filetype_t ft, game_t *parent, game_t *child)
{
    game_t *gparent;
    int i, j;
    rom_t * cr, *pr;
    
    if (game_cloneof(parent, ft, 0)) {
	/* tell grandparent of his new grandchild */
	/* XXX: handle error */
	gparent = r_game(db, game_cloneof(parent, ft, 0));
	game_add_clone(gparent, ft, game_name(child));
	w_game(db, gparent);
	game_free(gparent);

	/* tell child of his grandfather */
	game_cloneof(child, ft, 1) = xstrdup(game_cloneof(parent, ft, 0));
    }

    /* tell father of his child */
    game_add_clone(parent, ft, game_name(child));

    /* look for ROMs in parent */
    for (i=0; i<game_num_files(child, ft); i++) {
	cr = game_file(child, ft, i);
	for (j=0; j<game_num_files(parent, ft); j++) {
	    pr = game_file(parent, ft, j);
	    if (rom_compare_msc(cr, pr) == 0) {
		rom_where(cr) = (where_t)(rom_where(pr) + 1);
		break;
	    }
	}
    }

    return;
}



static int
file_location_copy(const hashes_t *key, parray_t *pa, void *ud)
{
    struct fbh_context *ctx = ud;

    parray_sort(pa, file_location_cmp);
    return w_file_by_hash_parray(ctx->db, ctx->ft, key, pa);
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
		    types &= ~(1<<ft);
		    continue;
		}
		
		if (lost(parent, (filetype_t)ft)) {
		    game_free(parent);
		    continue;
		}

		/* parent found */
		familymeeting(ctx->db, (filetype_t)ft, parent, child);
		w_game(ctx->db, parent);
		game_free(parent);
		types &= ~(1<<ft);
	    }

	    if (types != old_types)
		w_game(ctx->db, child);
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
	if (rom_where(game_file(g, ft, i)) != ROM_INZIP)
	    return 0;

    return 1;
}



static int
output_db_close(output_context_t *out)
{
    output_context_db_t *ctx;
    int i, ret;

    ctx = (output_context_db_t *)out;

    ret = 0;

    if (ctx->db) {
	w_dat(ctx->db, ctx->dat);

	if (handle_lost(ctx) < 0
	    || write_lists(ctx) < 0
	    || write_hashes(ctx) < 0
	    || write_hashtypes(ctx) < 0)
	    ret = -1;

	dbh_close(ctx->db);
    }

    map_free(ctx->map_rom, MAP_FREE_FN(file_location_free));
    map_free(ctx->map_disk, MAP_FREE_FN(file_location_free));
    dat_free(ctx->dat);
    parray_free(ctx->lost_children, free);
    array_free(ctx->lost_children_types, NULL);
    for (i=0; i<TYPE_MAX; i++)
	parray_free(ctx->list[i], free);

    return ret;
}



static int
output_db_game(output_context_t *out, game_t *g)
{
    output_context_db_t *ctx;
    rom_t *r;
    disk_t *d;
    int i, to_do;
    game_t *parent;

    ctx = (output_context_db_t *)out;

    game_dat_no(g) = dat_length(ctx->dat)-1;

    /* add to list of games with samples */
    if (game_num_files(g, TYPE_SAMPLE) > 0)
	parray_push(ctx->list[TYPE_SAMPLE], xstrdup(game_name(g)));

    for (i=0; i<game_num_files(g, TYPE_ROM); i++) {
	r = game_file(g, TYPE_ROM, i);
	ctx->romhashtypes |= hashes_types(rom_hashes(r));
	enter_file_hash(ctx->map_rom, TYPE_ROM, game_name(g),
			i, rom_hashes(r));
    }
    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);
	ctx->diskhashtypes |= hashes_types(disk_hashes(d));
	parray_push(ctx->list[TYPE_DISK], xstrdup(disk_name(d)));
	enter_file_hash(ctx->map_disk, TYPE_DISK, game_name(g),
			i, disk_hashes(d));
    }
    
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
		w_game(ctx->db, parent);
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

    parray_push(ctx->list[TYPE_ROM], xstrdup(game_name(g)));

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



static int
write_hashes(output_context_db_t *ctx)
{
    struct fbh_context ud;

    ud.db = ctx->db;
    
    ud.ft = TYPE_ROM;
    if (map_foreach(ctx->map_rom, file_location_copy, &ud) < 0)
	return -1;

    ud.ft = TYPE_DISK;
    if (map_foreach(ctx->map_disk, file_location_copy, &ud) < 0)
	return -1;
    
    return 0;
}



static int
write_hashtypes(output_context_db_t *ctx)
{
    return w_hashtypes(ctx->db, ctx->romhashtypes, ctx->diskhashtypes);
}



static int
write_lists(output_context_db_t *ctx)
{
    int i;

    for (i=0; i<TYPE_MAX; i++) {
	parray_sort_unique(ctx->list[i], strcmp);
	if (w_list(ctx->db, filetype_db_key((filetype_t)i), ctx->list[i]) < 0)
	    return -1;
    }

    return 0;
}
