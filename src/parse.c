/*
  $NiH: parse.c,v 1.19 2006/05/24 09:29:18 dillo Exp $

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



#include <sys/stat.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dat.h"
#include "error.h"
#include "funcs.h"
#include "parse.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

#define CHECK_STATE(ctx, s)						\
	do {								\
	    if ((ctx)->state != (s)) {					\
		myerror(ERRFILE, "%d: in wrong state", (ctx)->lineno);	\
		return -1;						\
	    }								\
	} while (0)

#define SET_STATE(ctx, s)	((ctx)->state = (s))



static void parser_context_free(parser_context_t *);
static parser_context_t *parser_context_new(const parray_t *,
					    const dat_entry_t *,
					    output_context_t *);
static int parse_header_end(parser_context_t *);

static void disk_end(parser_context_t *);
static int name_matches(const game_t *, const parray_t *);
static void rom_end(parser_context_t *, filetype_t);



int
parse(const char *fname, const parray_t *exclude, const dat_entry_t *dat,
	  output_context_t *out)
{
    parser_context_t *ctx;
    FILE *fin;
    int c, ret;
    struct stat st;

    fin = NULL;
    if (fname == NULL) {
	fin = stdin;
	seterrinfo("*stdin*", NULL);
    }
    else {
	if (stat(fname, &st) == -1) {
	    myerror(ERRSTR, "can't stat romlist file `%s'", fname);
	    return -1;
	}
	if ((st.st_mode & S_IFMT) != S_IFDIR) {
	    if ((fin=fopen(fname, "r")) == NULL) {
		myerror(ERRSTR, "can't open romlist file `%s'", fname);
		return -1;
	    }
	    seterrinfo(fname, NULL);
	}
    }

    ctx = parser_context_new(exclude, dat, out);

    if (fin) {
	c = getc(fin);
	ungetc(c, fin);

	if (c == '<')
	    ret = parse_xml(fin, ctx);
	else
	    ret = parse_cm(fin, ctx);

	if (fname != NULL)
	    fclose(fin);
    }
    else
	ret = parse_dir(fname, ctx);

    parser_context_free(ctx);

    return ret;
}



int
parse_file_end(parser_context_t *ctx, filetype_t ft)
{
    CHECK_STATE(ctx, PARSE_IN_FILE);
    
    if (ft == TYPE_DISK)
	disk_end(ctx);
    else
	rom_end(ctx, ft);

    SET_STATE(ctx, PARSE_IN_GAME);

    return 0;
}



/*ARGSUSED3*/
int
parse_file_status(parser_context_t *ctx, filetype_t ft, int ht,
		  const char *attr)
{
    status_t status;

    CHECK_STATE(ctx, PARSE_IN_FILE);
    
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
	disk_status(ctx->d) = status;
    else
	rom_status(ctx->r) = status;
    
    return 0;
}



int
parse_file_hash(parser_context_t *ctx, filetype_t ft, int ht, const char *attr)
{
    hashes_t *h;

    CHECK_STATE(ctx, PARSE_IN_FILE);
    
    if (ft == TYPE_DISK)
	h = disk_hashes(ctx->d);
    else
	h = rom_hashes(ctx->r);
	
    if (hash_from_string(h, attr) != ht) {
	myerror(ERRFILE, "%d: invalid argument for %s",
		ctx->lineno, hash_type_string(ht));
	return -1;
    }
    
    return 0;
}



/*ARGSUSED3*/
int
parse_file_merge(parser_context_t *ctx, filetype_t ft, int ht,
		 const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_FILE);
    
    if (ft == TYPE_DISK)
	disk_merge(ctx->d) = xstrdup(attr);
    else
	rom_merge(ctx->r) = xstrdup(attr);

    return 0;
}


 
/*ARGSUSED3*/
int
parse_file_name(parser_context_t *ctx, filetype_t ft, int dummy,
		const char *attr)
{
    char *p;
    
    CHECK_STATE(ctx, PARSE_IN_FILE);
    
    if (ft == TYPE_DISK)
	disk_name(ctx->d) = xstrdup(attr);
    else {
	rom_name(ctx->r) = xstrdup(attr);

	/* XXX: warn about broken dat file? */
	p = rom_name(ctx->r);
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
    CHECK_STATE(ctx, PARSE_IN_FILE);
    
    if (ft == TYPE_DISK) {
	myerror(ERRFILE, "%d: unknown attribute `size' for disk",
		ctx->lineno);
	return -1;
    }

    /* XXX: check for strol errors */
    rom_size(ctx->r) = strtol(attr, NULL, 10);

    return 0;
}



int
parse_file_start(parser_context_t *ctx, filetype_t ft)
{
    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    if (ft == TYPE_DISK)
	ctx->d = array_grow(game_disks(ctx->g), disk_init);
    else
	ctx->r = array_grow(game_files(ctx->g, ft), rom_init);

    SET_STATE(ctx, PARSE_IN_FILE);
    
    return 0;
}



/*ARGSUSED3*/
int
parse_game_cloneof(parser_context_t *ctx, filetype_t ft, int ht,
		   const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    if (ft == TYPE_DISK)
	return -1;

    game_cloneof(ctx->g, ft, 0) = xstrdup(attr);

    return 0;
}



int
parse_game_description(parser_context_t *ctx, const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    game_description(ctx->g) = xstrdup(attr);

    return 0;
}



/*ARGSUSED2*/
int
parse_game_end(parser_context_t *ctx, filetype_t ft)
{
    int i;
    game_t *g;

    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    if (!name_matches(ctx->g, ctx->ignore)) {
	g = ctx->g;

	/* omit description if same as name (to save space) */
	if (game_name(g) && game_description(g)
	    && strcmp(game_name(g), game_description(g)) == 0) {
	    free(game_description(g));
	    game_description(g) = NULL;
	}
	
	for (i=0; i<GAME_RS_MAX; i++) {
	    if (game_cloneof(g, i, 0)) {
		if (strcmp(game_cloneof(g, i, 0), game_name(g)) == 0) {
		    free(game_cloneof(g, i, 0));
		    game_cloneof(g, i, 0) = NULL;
		    continue;
		}
	    }
	}

	output_game(ctx->output, ctx->g);
    }

    game_free(ctx->g);
    ctx->g = NULL;

    SET_STATE(ctx, PARSE_OUTSIDE);
    
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
    if (ctx->state == PARSE_IN_HEADER)
	parse_header_end(ctx);

    CHECK_STATE(ctx, PARSE_OUTSIDE);
    
    if (ctx->g) {
	myerror(ERRFILE, "%d: game inside game", ctx->lineno);
	return -1;
    }

    ctx->g = game_new();

    SET_STATE(ctx, PARSE_IN_GAME);

    return 0;
}



int
parse_prog_description(parser_context_t *ctx, const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_HEADER);
    
    dat_entry_description(&ctx->de) = xstrdup(attr);

    return 0;
}



int
parse_prog_name(parser_context_t *ctx, const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_HEADER);
    
    dat_entry_name(&ctx->de) = xstrdup(attr);

    return 0;
}



int
parse_prog_version(parser_context_t *ctx, const char *attr)
{
    CHECK_STATE(ctx, PARSE_IN_HEADER);
    
    dat_entry_version(&ctx->de) = xstrdup(attr);

    return 0;
}



void
parser_context_free(parser_context_t *ctx)
{
    game_free(ctx->g);
    ctx->g = NULL;
    dat_entry_finalize(&ctx->de);

    free(ctx);
}



static parser_context_t *
parser_context_new(const parray_t *exclude, const dat_entry_t *dat,
		   output_context_t *out)
{
    parser_context_t *ctx;

    ctx = (parser_context_t *)xmalloc(sizeof(*ctx));

    dat_entry_merge(&ctx->dat_default, dat, NULL);
    ctx->output = out;
    ctx->ignore = exclude;
    ctx->state = PARSE_IN_HEADER;

    ctx->lineno = 0;
    dat_entry_init(&ctx->de);
    ctx->g = NULL;

    return ctx;
}



static int
parse_header_end(parser_context_t *ctx)
{
    dat_entry_t de;

    CHECK_STATE(ctx, PARSE_IN_HEADER);

    dat_entry_merge(&de, &ctx->dat_default, &ctx->de);
    output_header(ctx->output, &de);
    dat_entry_finalize(&de);

    SET_STATE(ctx, PARSE_OUTSIDE);

    return 0;
}



static void
disk_end(parser_context_t *ctx)
{
    if (hashes_types(disk_hashes(ctx->d)) == 0)
	disk_status(ctx->d) = STATUS_NODUMP;

    if (disk_merge(ctx->d) != NULL
	&& strcmp(disk_name(ctx->d), disk_merge(ctx->d)) == 0) {
	free(disk_merge(ctx->d));
	disk_merge(ctx->d) = NULL;
    }
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

    r = ctx->r;    
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
    }
}
