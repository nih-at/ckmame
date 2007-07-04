/*
  $NiH: output_cm.c,v 1.3 2007/04/09 18:20:40 dillo Exp $

  output-cm.c -- write games to clrmamepro dat files
  Copyright (C) 2006-2007 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "output.h"
#include "xmalloc.h"



struct output_context_cm {
    output_context_t output;

    FILE *f;
    char *fname;
    parray_t *games;
};

typedef struct output_context_cm output_context_cm_t;



static int output_cm_close(output_context_t *);
static int output_cm_game(output_context_t *, game_t *);
static int output_cm_header(output_context_t *, dat_entry_t *);
static int cmp_games(const game_t *, const game_t *);
static void print_hash(output_context_cm_t *, int, hashes_t *, const char *);
static void print_string(output_context_cm_t *, const char *, const char *,
			 const char *);
static int write_game(output_context_cm_t *, game_t *);



output_context_t *
output_cm_new(const char *fname)
{
    output_context_cm_t *ctx;
    FILE *f;

    ctx = (output_context_cm_t *)xmalloc(sizeof(*ctx));

    if (fname == NULL) {
	f = stdout;
	fname = "*stdout*";
    }
    else {
	if ((f=fopen(fname, "w")) == NULL) {
	    myerror(ERRDEF, "cannot create `%s': %s", fname, strerror(errno));
	    free(ctx);
	    return NULL;
	}
    }

    ctx->output.close = output_cm_close;
    ctx->output.output_detector = NULL;
    ctx->output.output_game = output_cm_game;
    ctx->output.output_header = output_cm_header;

    ctx->f = f;
    ctx->fname = xstrdup(fname);
    ctx->games = parray_new();

    return (output_context_t *)ctx;
}



static int
output_cm_close(output_context_t *out)
{
    output_context_cm_t *ctx;
    int i, ret;

    ctx = (output_context_cm_t *)out;

    parray_sort(ctx->games, cmp_games);
    for (i=0; i<parray_length(ctx->games); i++)
	write_game(ctx, parray_get(ctx->games, i));
    parray_free(ctx->games, game_free);

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else
	ret = fclose(ctx->f);

    free(ctx);

    return ret;
}



static int output_cm_game(output_context_t *out, game_t *g)
{
    output_context_cm_t *ctx;

    ctx = (output_context_cm_t *)out;

    parray_push(ctx->games, g);

    return 1;
}



static int
output_cm_header(output_context_t *out, dat_entry_t *dat)
{
    output_context_cm_t *ctx;

    ctx = (output_context_cm_t *)out;

    fputs("clrmamepro (\n", ctx->f);
    print_string(ctx, "\tname", dat_entry_name(dat), "\n");
    print_string(ctx, "\tdescription",
		 (dat_entry_description(dat)
		  ? dat_entry_description(dat) : dat_entry_name(dat)), "\n");
    print_string(ctx, "\tversion", dat_entry_version(dat), "\n");
    fputs(")\n\n", ctx->f);

    return 0;
}



static int
cmp_games(const game_t *g1, const game_t *g2)
{
    return strcasecmp(game_name(g1), game_name(g2));
}



static void
print_hash(output_context_cm_t *ctx, int t, hashes_t *h, const char *post)
{
    char hstr[HASHES_SIZE_MAX*2+1];

    print_string(ctx, hash_type_string(t), hash_to_string(hstr, t, h), post);
}



static void
print_string(output_context_cm_t *ctx, const char *pre, const char *str,
	     const char *post)
{
    char *q;

    if (str == NULL)
	return;

    if (strcspn(str, " \t") == strlen(str))
	q = "";
    else
	q = "\"";

    fprintf(ctx->f, "%s %s%s%s%s", pre, q, str, q, post);
}



static int
write_game(output_context_cm_t *ctx, game_t *g)
{
    rom_t *r;
    int i;
    char *fl;

    fputs("game (\n", ctx->f);
    print_string(ctx, "\tname", game_name(g), "\n");
    print_string(ctx, "\tdescription",
		 game_description(g) ? game_description(g) : game_name(g),
		 "\n");
    print_string(ctx, "\tcloneof", game_cloneof(g, TYPE_ROM, 0), "\n");
    print_string(ctx, "\tromof", game_cloneof(g, TYPE_ROM, 0), "\n");
    for (i=0; i<game_num_files(g, TYPE_ROM); i++) {
	r = game_file(g, TYPE_ROM, i);
	
	fputs("\trom ( ", ctx->f);
	print_string(ctx, "name", rom_name(r), " ");
	if (rom_where(r) != ROM_INZIP)
	    print_string(ctx, "merge",
			 rom_merge(r) ? rom_merge(r) : rom_name(r),
			 " ");
	fprintf(ctx->f, "size %lu ", rom_size(r));
	print_hash(ctx, HASHES_TYPE_CRC, rom_hashes(r), " ");
	print_hash(ctx, HASHES_TYPE_SHA1, rom_hashes(r), " ");
	print_hash(ctx, HASHES_TYPE_MD5, rom_hashes(r), " ");
	switch (rom_status(r)) {
	case STATUS_OK:
	    fl = NULL;
	    break;
	case STATUS_BADDUMP:
	    fl = "baddump";
	    break;
	case STATUS_NODUMP:
	    fl = "nodump";
	    break;
	}
	print_string(ctx, "flags", fl, " ");
	fputs(")\n", ctx->f);
    }
    /* XXX: samples */
    /* XXX: disks */
    fputs(")\n\n", ctx->f);

    return 0;
}
