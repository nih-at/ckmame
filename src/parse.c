/*
  $NiH: parse.c,v 1.4 2005/07/13 17:42:20 dillo Exp $

  parse.c -- parser frontend
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "error.h"
#include "file_by_hash.h"
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
static int file_by_hash_copy(const hashes_t *, parray_t *, void *);
static int handle_lost(parser_context_t *);
static int lost(game_t *, filetype_t);
static int parser_context_init(parser_context_t *);
static void parser_context_finalize(parser_context_t *);
static void rom_end(parser_context_t *, filetype_t);
static int write_hashes(parser_context_t *);
static int write_hashtypes(parser_context_t *);
static int write_lists(parser_context_t *);
static int write_prog(parser_context_t *, const char *, const char *);



int
parse(DB *db, const char *fname,
      const char *prog_name, const char *prog_version)
{
    FILE *fin;
    parser_context_t ctx;
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
    
    parser_context_init(&ctx);
    ctx.db = db;
    ctx.fin = fin;

    c = getc(ctx.fin);
    ungetc(c, ctx.fin);

    if (c == '<')
	ret = parse_xml(&ctx);
    else
	ret = parse_cm(&ctx);

    fclose(ctx.fin);
    ctx.fin = NULL;

    if (ret < 0
	|| handle_lost(&ctx) < 0
	|| write_lists(&ctx) < 0
	|| write_hashes(&ctx) < 0
	|| write_hashtypes(&ctx) < 0
	|| write_prog(&ctx, prog_name, prog_version) < 0)
	ret = -1;
    else
	ret = 0;

    parser_context_finalize(&ctx);

    return ret;
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



int
parse_file_flags(parser_context_t *ctx, filetype_t ft, int ht,
		 const char *attr)
{
    flags_t flags;

    if (strcmp(attr, "good") == 0)
	flags = FLAGS_OK;
    else if (strcmp(attr, "baddump") == 0)
	flags = FLAGS_BADDUMP;
    else if (strcmp(attr, "nodump") == 0)
	flags = FLAGS_NODUMP;
    else {
	myerror(ERRFILE, "%d: illegal flags `%s'",
		ctx->lineno, attr);
	return -1;
    }

    if (ft == TYPE_DISK)
	disk_flags(game_last_disk(ctx->g)) = flags;
    else
	rom_flags(game_last_file(ctx->g, ft)) = flags;
    
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
	/* XXX:
	   myerror(ERRFILE, "%d: invalid argument for %s",
		   lineno, hash_type_string(ht));
	*/
	return -1;
    }
    
    *types |= HASHES_TYPE_MD5;
    
    return 0;
}



int parse_file_merge(parser_context_t *ctx, filetype_t ft, int ht,
		     const char *attr)
{
    if (ft == TYPE_DISK)
	disk_merge(game_last_disk(ctx->g)) = xstrdup(attr);
    else
	rom_merge(game_last_file(ctx->g, ft)) = xstrdup(attr);

    return 0;
}



int
parse_file_name(parser_context_t *ctx, filetype_t ft, int dummy,
		const char *attr)
{
    if (ft == TYPE_DISK) {
	disk_name(game_last_disk(ctx->g)) = xstrdup(attr);
	parray_push(ctx->list[TYPE_DISK], xstrdup(attr));
    }
    else
	rom_name(game_last_file(ctx->g, ft)) = xstrdup(attr);

    return 0;
}



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



int
parse_game_end(parser_context_t *ctx, filetype_t ft)
{
    int i, to_do;
    game_t *g, *parent;

    g = ctx->g;

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
		|| lost(parent, i)) {
		to_do |= 1<<i;
		if (parent)
		    game_free(parent);
	    }
	    else {
		familymeeting(ctx->db, i, parent, g);
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
	myerror(ERRSTR, "can't write game `%s' to db: %s",
		game_name(g), ddb_error());
    else
	parray_push(ctx->list[TYPE_ROM], xstrdup(game_name(g)));

    game_free(ctx->g);
    ctx->g = NULL;

    return 0;
}




int
parse_game_name(parser_context_t *ctx, filetype_t ft, int ht,
		const char *attr)
{
    game_name(ctx->g) = xstrdup(attr);

    return 0;
}



int
parse_game_start(parser_context_t *ctx, filetype_t ft)
{
    if (ctx->g) {
	myerror(ERRFILE, "%d: game inside game", ctx->lineno);
	return -1;
    }

    ctx->g = game_new();

    return 0;
}



int
parse_prog_name(parser_context_t *ctx, const char *attr)
{
    ctx->prog_name = xstrdup(attr);

    return 0;
}



int
parse_prog_version(parser_context_t *ctx, const char *attr)
{
    ctx->prog_version = xstrdup(attr);

    return 0;
}



static void
disk_end(parser_context_t *ctx)
{
    disk_t *d;

    d = game_last_disk(ctx->g);

    if (hashes_types(disk_hashes(d)) == 0)
	disk_flags(d) = FLAGS_NODUMP;

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

    type = file_by_hash_default_hashtype(filetype);

    if (hashes_has_type(hashes, type)) {
	if (map_add(map, type, hashes,
		    file_by_hash_new(name, index)) < 0) {
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
	    if (romcmp(pr, cr, 1) == ROM_OK) {
		rom_where(cr) = (where_t)(rom_where(pr) + 1);
		break;
	    }
	}
    }

    return;
}



static int
file_by_hash_copy(const hashes_t *key, parray_t *pa, void *ud)
{
    struct fbh_context *ctx = ud;

    parray_sort(pa, file_by_hash_entry_cmp);
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
			    "inconsistency: %s has non-existent parnet %s",
			    game_name(child), game_cloneof(child, ft, 0));
		    
		    /* remove non-existent cloneof */
		    free(game_cloneof(child, ft, 0));
		    game_cloneof(child, ft, 0) = NULL;
		    types &= ~(1<<ft);
		    continue;
		}
		
		if (lost(parent, ft)) {
		    game_free(parent);
		    continue;
		}

		/* parent found */
		familymeeting(ctx->db, ft, parent, child);
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
parser_context_init(parser_context_t *ctx)
{
    int i;
    ctx->db = NULL;
    ctx->fin = NULL;
    if ((ctx->map_rom=map_new()) == NULL) {
	myerror(ERRDEF, "can't create hash table: %s", ddb_error());
	return -1;
    }
    if ((ctx->map_disk=map_new()) == NULL) {
	myerror(ERRDEF, "can't create hash table: %s", ddb_error());
	map_free(ctx->map_rom, NULL);
	return -1;
    }
    ctx->g = NULL;
    ctx->prog_name = ctx->prog_version = NULL;
    ctx->romhashtypes = ctx->diskhashtypes = 0;
    ctx->lost_children = parray_new();
    ctx->lost_children_types = array_new(sizeof(int));
    for (i=0; i<TYPE_MAX; i++)
	ctx->list[i] = parray_new();

    return 0;
}



static void
parser_context_finalize(parser_context_t *ctx)
{
    int i;

    if (ctx->fin)
	fclose(ctx->fin);
    map_free(ctx->map_rom, MAP_FREE_FN(file_by_hash_free));
    map_free(ctx->map_disk, MAP_FREE_FN(file_by_hash_free));
    game_free(ctx->g);
    free(ctx->prog_name);
    free(ctx->prog_version);
    parray_free(ctx->lost_children, free);
    array_free(ctx->lost_children_types, NULL);
    for (i=0; i<TYPE_MAX; i++)
	parray_free(ctx->list[i], free);
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

    /* CRC == 0 was old way of indicating no-good-dumps */
    if ((hashes_has_type(h, HASHES_TYPE_CRC)) && h->crc == 0) {
	h->types &= ~HASHES_TYPE_CRC;
	rom_flags(r) = FLAGS_NODUMP;
    }
    
    /* omit duplicates */
    deleted = 0;
    for (j=0; j<n; j++) {
	if (romcmp(game_file(ctx->g, ft, j), r, 0) == ROM_OK) {
	    deleted = 1;
	    break;
	}
    }
    if (!deleted) {
	for (j=0; j<n; j++) {
	    r2 = game_file(ctx->g, ft, j);
	    if ((romcmp(r2, r, 0) == ROM_NAMERR
		 && rom_merge(r2) && rom_merge(r)
		 && !strcmp(rom_merge(r2), rom_merge(r)))) {
		rom_add_altname(r2, rom_name(r));
		deleted = 1;
		break;
	    }
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
    if (map_foreach(ctx->map_rom, file_by_hash_copy, &ud) < 0)
	return -1;

    ud.ft = TYPE_DISK;
    if (map_foreach(ctx->map_disk, file_by_hash_copy, &ud) < 0)
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
	parray_sort_unique(ctx->list[i], strcasecmp);
	if (w_list(ctx->db, filetype_db_key(i), ctx->list[i]) < 0)
	    return -1;
    }

    return 0;
}



static int
write_prog(parser_context_t *ctx, const char *name, const char *version)
{
    return w_prog(ctx->db, name ? name : ctx->prog_name,
		  name ? version : ctx->prog_version);
}
