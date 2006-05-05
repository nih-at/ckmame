/*
  $NiH: parse.c,v 1.14 2006/05/04 07:52:45 dillo Exp $

  parse.c -- parser frontend
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



#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dat.h"
#include "dbh.h"
#include "error.h"
#include "file_location.h"
#include "funcs.h"
#include "map.h"
#include "parse.h"
#include "r.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

#define CHECK_IN_GAME						\
	if (g == NULL) {					\
	    parse_errstr = "parse error: no game started";	\
	    return -1;						\
	}

#define CHECK_STATE(s)						\
	CHECK_IN_GAME;						\
	if (state != s) {					\
	    parse_errstr = "parse error: in wrong state";	\
	    return -1;						\
	}



struct fbh_context {
    DB *db;
    filetype_t ft;
};

static void disk_end(parser_context_t *);
static void enter_file_hash(map_t *, filetype_t, const char *,
			    int, const hashes_t *);
static void familymeeting(DB *, filetype_t, game_t *, game_t *);
static int file_location_copy(const hashes_t *, parray_t *, void *);
static int handle_lost(parser_context_t *);
static int lost(game_t *, filetype_t);
static int name_matches(const game_t *, const parray_t *);
static void rom_end(parser_context_t *, filetype_t);
static int write_hashes(parser_context_t *);
static int write_hashtypes(parser_context_t *);
static int write_lists(parser_context_t *);



int
parse(parser_context_t *ctx, const char *fname, const dat_entry_t *dat)
{
    FILE *fin;
    int c, ret;

    if (fname == NULL) {
	fin = stdin;
	seterrinfo("*stdin*", NULL);
    }
    else {
	if ((fin=fopen(fname, "r")) == NULL) {
	    myerror(ERRSTR, "can't open romlist file `%s'", fname);
	    return -1;
	}
	seterrinfo(fname, NULL);
    }
    
    parser_context_init_perfile(ctx);
    ctx->fin = fin;

    c = getc(ctx->fin);
    ungetc(c, ctx->fin);

    if (c == '<')
	ret = parse_xml(ctx);
    else
	ret = parse_cm(ctx);

    if (fname == NULL) {
	/* don't close stdin */
	ctx->fin = NULL;
    }
    
    dat_push(ctx->dat, dat, &ctx->de);
    
    parser_context_finalize_perfile(ctx);
    return ret;
}

int
parse_bookkeeping(parser_context_t *ctx)
{
    w_dat(ctx->db, ctx->dat);

    if (handle_lost(ctx) < 0
	|| write_lists(ctx) < 0
	|| write_hashes(ctx) < 0
	|| write_hashtypes(ctx) < 0)
	return -1;

    return 0;
}



int
parse_file_end(parser_context_t *ctx, filetype_t ft)
{
    if (ft == TYPE_DISK)
	disk_end(ctx);
    else
	rom_end(ctx, ft);
    
    return 0;
}



/*ARGSUSED3*/
int
parse_file_status(parser_context_t *ctx, filetype_t ft, int ht,
		  const char *attr)
{
    status_t status;

    if (strcmp(attr, "good") == 0)
	status = STATUS_OK;
    else if (strcmp(attr, "baddump") == 0)
	status = STATUS_BADDUMP;
    else if (strcmp(attr, "nodump") == 0)
	status = STATUS_NODUMP;
    else {
	myerror(ERRFILE, "%d: illegal status `%s'",
		ctx->lineno, attr);
	return -1;
    }

    if (ft == TYPE_DISK)
	disk_status(game_last_disk(ctx->g)) = status;
    else
	rom_status(game_last_file(ctx->g, ft)) = status;
    
    return 0;
}



int
parse_file_hash(parser_context_t *ctx, filetype_t ft, int ht, const char *attr)
{
    hashes_t *h;
    int *types;

    if (ft == TYPE_DISK) {
	h = disk_hashes(game_last_disk(ctx->g));
	types = &ctx->diskhashtypes;
    }
    else {
	h = rom_hashes(game_last_file(ctx->g, ft));
	types = &ctx->romhashtypes;
    }
	
    if (hash_from_string(h, attr) != ht) {
	myerror(ERRFILE, "%d: invalid argument for %s",
		ctx->lineno, hash_type_string(ht));
	return -1;
    }
    
    *types |= ht;
    
    return 0;
}



/*ARGSUSED3*/
int
parse_file_merge(parser_context_t *ctx, filetype_t ft, int ht,
		 const char *attr)
{
    if (ft == TYPE_DISK)
	disk_merge(game_last_disk(ctx->g)) = xstrdup(attr);
    else
	rom_merge(game_last_file(ctx->g, ft)) = xstrdup(attr);

    return 0;
}


 
/*ARGSUSED3*/
int
parse_file_name(parser_context_t *ctx, filetype_t ft, int dummy,
		const char *attr)
{
    char *p;
    
    if (ft == TYPE_DISK) {
	disk_name(game_last_disk(ctx->g)) = xstrdup(attr);
	parray_push(ctx->list[TYPE_DISK], xstrdup(attr));
    }
    else {
	rom_name(game_last_file(ctx->g, ft)) = xstrdup(attr);

	/* XXX: warn about broken dat file? */
	p = rom_name(game_last_file(ctx->g, ft));
	while ((p=strchr(p, '\\')))
	    *(p++) = '/';
    }

    return 0;
}



/*ARGSUSED3*/
int
parse_file_size(parser_context_t *ctx, filetype_t ft, int dummy,
		const char *attr)
{
    if (ft == TYPE_DISK) {
	myerror(ERRFILE, "%d: unknown attribute `size' for disk",
		ctx->lineno);
	return -1;
    }

    /* XXX: check for strol errors */
    rom_size(game_last_file(ctx->g, ft)) = strtol(attr, NULL, 10);

    return 0;
}



int
parse_file_start(parser_context_t *ctx, filetype_t ft)
{
    if (ft == TYPE_DISK)
	array_grow(game_disks(ctx->g), disk_init);
    else
	array_grow(game_files(ctx->g, ft), rom_init);

    return 0;
}



/*ARGSUSED3*/
int
parse_game_cloneof(parser_context_t *ctx, filetype_t ft, int ht,
		   const char *attr)
{
    if (ft == TYPE_DISK)
	return -1;

    game_cloneof(ctx->g, ft, 0) = xstrdup(attr);

    return 0;
}



int
parse_game_description(parser_context_t *ctx, const char *attr)
{
    game_description(ctx->g) = xstrdup(attr);

    return 0;
}



/*ARGSUSED2*/
int
parse_game_end(parser_context_t *ctx, filetype_t ft)
{
    int i, to_do;
    game_t *g, *parent;

    if (name_matches(ctx->g, ctx->ignore)) {
	game_free(ctx->g);
	ctx->g = NULL;
	return 0;
    }

    g = ctx->g;

    /* omit description if same as name (to save space) */
    if (game_name(g) && game_description(g)
	&& strcmp(game_name(g), game_description(g)) == 0) {
	free(game_description(g));
	game_description(g) = NULL;
    }

    /* add to list of games with samples */
    if (game_num_files(g, TYPE_SAMPLE) > 0)
	parray_push(ctx->list[TYPE_SAMPLE], xstrdup(game_name(g)));

    to_do = 0;
    for (i=0; i<GAME_RS_MAX; i++) {
	if (game_cloneof(g, i, 0)) {
	    if (strcmp(game_cloneof(g, i, 0), game_name(g)) == 0) {
		free(game_cloneof(g, i, 0));
		game_cloneof(g, i, 0) = NULL;
		continue;
	    }

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
    
    if (w_game(ctx->db, g) != 0)
	myerror(ERRDB, "can't write game `%s' to db", game_name(g));
    else
	parray_push(ctx->list[TYPE_ROM], xstrdup(game_name(g)));

    game_free(ctx->g);
    ctx->g = NULL;

    return 0;
}




/*ARGSUSED3*/
int
parse_game_name(parser_context_t *ctx, filetype_t ft, int ht,
		const char *attr)
{
    game_name(ctx->g) = xstrdup(attr);

    return 0;
}



/*ARGSUSED2*/
int
parse_game_start(parser_context_t *ctx, filetype_t ft)
{
    if (ctx->g) {
	myerror(ERRFILE, "%d: game inside game", ctx->lineno);
	return -1;
    }

    ctx->g = game_new();
    game_dat_no(ctx->g) = dat_length(ctx->dat);

    return 0;
}



int
parse_prog_description(parser_context_t *ctx, const char *attr)
{
    dat_entry_description(&ctx->de) = xstrdup(attr);

    return 0;
}



int
parse_prog_name(parser_context_t *ctx, const char *attr)
{
    dat_entry_name(&ctx->de) = xstrdup(attr);

    return 0;
}



int
parse_prog_version(parser_context_t *ctx, const char *attr)
{
    dat_entry_version(&ctx->de) = xstrdup(attr);

    return 0;
}



void
parser_context_finalize_perfile(parser_context_t *ctx)
{
    if (ctx->fin)
	fclose(ctx->fin);
    ctx->fin = NULL;
    game_free(ctx->g);
    ctx->g = NULL;
    dat_entry_finalize(&ctx->de);
}



void
parser_context_free(parser_context_t *ctx)
{
    int i;

    parser_context_finalize_perfile(ctx);

    map_free(ctx->map_rom, MAP_FREE_FN(file_location_free));
    map_free(ctx->map_disk, MAP_FREE_FN(file_location_free));
    dat_free(ctx->dat);
    parray_free(ctx->lost_children, free);
    array_free(ctx->lost_children_types, NULL);
    for (i=0; i<TYPE_MAX; i++)
	parray_free(ctx->list[i], free);
}



void
parser_context_init_perfile(parser_context_t *ctx)
{
    ctx->fin = NULL;
    ctx->lineno = 0;
    dat_entry_init(&ctx->de);
    ctx->g = NULL;

}



parser_context_t *
parser_context_new(DB *db, const parray_t *ignore)
{
    parser_context_t *ctx;
    int i;

    ctx = (parser_context_t *)xmalloc(sizeof(*ctx));

    ctx->db = db;
    ctx->ignore = ignore;

    parser_context_init_perfile(ctx);

    if ((ctx->map_rom=map_new()) == NULL) {
	myerror(ERRDB, "can't create hash table");
	free(ctx);
	return NULL;
    }
    if ((ctx->map_disk=map_new()) == NULL) {
	myerror(ERRDB, "can't create hash table");
	map_free(ctx->map_rom, NULL);
	free(ctx);
	return NULL;
    }
    ctx->dat=dat_new();
    ctx->romhashtypes = ctx->diskhashtypes = 0;
    ctx->lost_children = parray_new();
    ctx->lost_children_types = array_new(sizeof(int));
    for (i=0; i<TYPE_MAX; i++)
	ctx->list[i] = parray_new();

    return ctx;
}



static void
disk_end(parser_context_t *ctx)
{
    disk_t *d;

    d = game_last_disk(ctx->g);

    if (hashes_types(disk_hashes(d)) == 0)
	disk_status(d) = STATUS_NODUMP;

    if (disk_merge(d) != NULL && strcmp(disk_name(d), disk_merge(d)) == 0) {
	free(disk_merge(d));
	disk_merge(d) = NULL;
    }

    enter_file_hash(ctx->map_disk, TYPE_DISK,
		    game_name(ctx->g), game_num_disks(ctx->g)-1,
		    disk_hashes(d));
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
handle_lost(parser_context_t *ctx)
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
		*((int *)array_get(ctx->lost_children_types, i)) = types;
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
name_matches(const game_t *g, const parray_t *ignore)
{
    int i;

    if (ignore == NULL)
	return 0;

    for (i=0; i<parray_length(ignore); i++) {
	if (fnmatch(parray_get(ignore, i), game_name(g), 0) == 0)
	    return 1;
    }

    return 0;
}



static void
rom_end(parser_context_t *ctx, filetype_t ft)
{
    rom_t *r, *r2;
    hashes_t *h;
    int deleted;
    int j, n;

    r = game_last_file(ctx->g, ft);    
    n = game_num_files(ctx->g, ft)-1;
    h = rom_hashes(r);

    /* omit duplicates */
    deleted = 0;
    for (j=0; j<n; j++) {
	r2 = game_file(ctx->g, ft, j);
	if (rom_compare_sc(r, r2) == 0) {
	    /* XXX: merge in additional hash types? */
	    if (rom_compare_n(r, r2) == 0) {
		deleted = 1;
		break;
	    }
	    else if (rom_merge(r2) && rom_merge(r)
		     && strcmp(rom_merge(r2), rom_merge(r)) != 0) {
		rom_add_altname(r2, rom_name(r));
		deleted = 1;
		break;
	    }
	}
	else if (rom_compare_n(r, r2) == 0) {
	    myerror(ERRFILE, "%d: two different roms with same name (%s)",
		    ctx->lineno, rom_name(r));
	    deleted = 1;
	    break;
	}
    }
    if (deleted)
	array_delete(game_files(ctx->g, ft), n, rom_finalize);
    else {
	if (rom_merge(r) != NULL && strcmp(rom_name(r), rom_merge(r)) == 0) {
	    free(rom_merge(r));
	    rom_merge(r) = NULL;
	}
	if (ft == TYPE_ROM)
	    enter_file_hash(ctx->map_rom, TYPE_ROM, game_name(ctx->g), n, h);
    }
}



static int
write_hashes(parser_context_t *ctx)
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
write_hashtypes(parser_context_t *ctx)
{
    return w_hashtypes(ctx->db, ctx->romhashtypes, ctx->diskhashtypes);
}



static int
write_lists(parser_context_t *ctx)
{
    int i;

    for (i=0; i<TYPE_MAX; i++) {
	parray_sort_unique(ctx->list[i], strcmp);
	if (w_list(ctx->db, filetype_db_key((filetype_t)i), ctx->list[i]) < 0)
	    return -1;
    }

    return 0;
}
