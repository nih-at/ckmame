/*
  parse.c -- parser frontend
  Copyright (C) 1999-2008 Dieter Baron and Thomas Klausner

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



#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "dat.h"
#include "error.h"
#include "globals.h"
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



static int parse_header_end(parser_context_t *);

static void disk_end(parser_context_t *);
static void rom_end(parser_context_t *, filetype_t);



int
parse(parser_source_t *ps, const parray_t *exclude, const dat_entry_t *dat,
	  output_context_t *out)
{
    parser_context_t *ctx;
    int c, ret;

    ctx = parser_context_new(ps, exclude, dat, out);

    c = ps_peek(ps);

    switch (c) {
    case '<':
	ret = parse_xml(ps, ctx);
	break;
    case '[':
	ret = parse_rc(ps, ctx);
	break;
    default:
	ret = parse_cm(ps, ctx);
    }
    
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
    else if (strcmp(attr, "verified") == 0)
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
	file_status(ctx->r) = status;
    
    return 0;
}



int
parse_file_hash(parser_context_t *ctx, filetype_t ft, int ht, const char *attr)
{
    hashes_t *h;

    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (strcmp(attr, "-") == 0) {
	/* some dat files record crc - for 0-byte files, so skip it */
	return 0;
    }

    if (ft == TYPE_DISK)
	h = disk_hashes(ctx->d);
    else
	h = file_hashes(ctx->r);
	
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
	file_merge(ctx->r) = xstrdup(attr);

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
	file_name(ctx->r) = xstrdup(attr);

	/* XXX: warn about broken dat file? */
	p = file_name(ctx->r);
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
    file_size(ctx->r) = strtol(attr, NULL, 0);

    return 0;
}



int
parse_file_start(parser_context_t *ctx, filetype_t ft)
{
    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    if (ft == TYPE_DISK)
	ctx->d = array_grow(game_disks(ctx->g), disk_init);
    else
	ctx->r = array_grow(game_files(ctx->g, ft), file_init);

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
    int keep_g, ret;

    CHECK_STATE(ctx, PARSE_IN_GAME);

    keep_g = 0;

    if (!name_matches(game_name(ctx->g), ctx->ignore)) {
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

	ret = output_game(ctx->output, ctx->g);
	if (ret == 1) {
	    keep_g = 1;
	    ret = 0;
	}
    }

    if (!keep_g)
	game_free(ctx->g);
    ctx->g = NULL;

    SET_STATE(ctx, PARSE_OUTSIDE);
    
    return ret;
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



/* ARGSUSED3 */
int
parse_prog_header(parser_context_t *ctx, const char *name, int dummy)
{
    parser_source_t *ps;
    int ret;

    CHECK_STATE(ctx, PARSE_IN_HEADER);

    if (detector != 0) {
	myerror(ERRFILE,
		"%d: warning: detector already defined, header `%s' ignored",
		ctx->lineno, name);
	return 0;
    }

    if ((ps=ps_open(ctx->ps, name)) == NULL) {
	myerror(ERRFILESTR, "%d: cannot open detector `%s'", ctx->lineno, name);
	return -1;
    }

    if ((detector=detector_parse_ps(ps)) == NULL) {
	myerror(ERRFILESTR, "%d: cannot parse detector `%s'",
		ctx->lineno, name);
	ret = -1;
    }
    else
	ret = output_detector(ctx->output, detector);

    ps_close(ps);

    return ret;
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



parser_context_t *
parser_context_new(parser_source_t *ps, const parray_t *exclude,
		   const dat_entry_t *dat, output_context_t *out)
{
    parser_context_t *ctx;

    ctx = (parser_context_t *)xmalloc(sizeof(*ctx));

    dat_entry_merge(&ctx->dat_default, dat, NULL);
    ctx->output = out;
    ctx->ignore = exclude;
    ctx->state = PARSE_IN_HEADER;

    ctx->ps = ps;
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



int
name_matches(const char *name, const parray_t *patterns)
{
    int i;

    if (patterns == NULL)
	return 0;

    for (i=0; i<parray_length(patterns); i++) {
	if (fnmatch(parray_get(patterns, i), name, 0) == 0)
	    return 1;
    }

    return 0;
}



static void
rom_end(parser_context_t *ctx, filetype_t ft)
{
    file_t *r, *r2;
    int deleted;
    int j, n;

    r = ctx->r;    
    n = game_num_files(ctx->g, ft)-1;

    /* some dats don't record crc for 0-byte files, so set it here */
    if (file_size(r) == 0 && !hashes_has_type(file_hashes(r), HASHES_TYPE_CRC))
	hashes_set(file_hashes(r), HASHES_TYPE_CRC,
		   (unsigned char *)"\0\0\0\0");

    /* omit duplicates */
    deleted = 0;

    if (file_name(r) == NULL) {
	    myerror(ERRFILE, "%d: roms without name", ctx->lineno);
	    deleted = 1;
    }	
    for (j=0; j<n && !deleted; j++) {
	r2 = game_file(ctx->g, ft, j);
	if (file_compare_sc(r, r2)) {
	    /* XXX: merge in additional hash types? */
	    if (file_compare_n(r, r2)) {
		deleted = 1;
		break;
	    }
	    else if (file_merge(r2) && file_merge(r)
		     && strcmp(file_merge(r2), file_merge(r)) != 0) {
		/* file_add_altname(r2, file_name(r)); */
		deleted = 1;
		break;
	    }
	}
	else if (file_compare_n(r, r2)) {
	    myerror(ERRFILE, "%d: two different roms with same name (%s)",
		    ctx->lineno, file_name(r));
	    deleted = 1;
	    break;
	}
    }
    if (deleted)
	array_delete(game_files(ctx->g, ft), n, file_finalize);
    else {
	if (file_merge(r) != NULL && strcmp(file_name(r), file_merge(r)) == 0) {
	    free(file_merge(r));
	    file_merge(r) = NULL;
	}
    }
}
