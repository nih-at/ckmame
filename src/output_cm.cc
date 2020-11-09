/*
  output-cm.c -- write games to clrmamepro dat files
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
static int write_game(output_context_cm_t *, game_t *);


output_context_t *
output_cm_new(const char *fname, int flags) {
    output_context_cm_t *ctx;
    FILE *f;

    ctx = (output_context_cm_t *)xmalloc(sizeof(*ctx));

    if (fname == NULL) {
	f = stdout;
	fname = "*stdout*";
    }
    else {
	if ((f = fopen(fname, "w")) == NULL) {
	    myerror(ERRDEF, "cannot create '%s': %s", fname, strerror(errno));
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
output_cm_close(output_context_t *out) {
    output_context_cm_t *ctx;
    int i, ret;

    ctx = (output_context_cm_t *)out;

    parray_sort(ctx->games, reinterpret_cast<int (*)(const void *, const void *)>(cmp_games));
    for (i = 0; i < parray_length(ctx->games); i++)
	write_game(ctx, static_cast<game_t *>(parray_get(ctx->games, i)));
    parray_free(ctx->games, reinterpret_cast<void (*)(void *)>(game_free));

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else
	ret = fclose(ctx->f);

    free(ctx);

    return ret;
}


static int
output_cm_game(output_context_t *out, game_t *g) {
    output_context_cm_t *ctx;

    ctx = (output_context_cm_t *)out;

    parray_push(ctx->games, g);

    return 1;
}


static int
output_cm_header(output_context_t *out, dat_entry_t *dat) {
    output_context_cm_t *ctx;

    ctx = (output_context_cm_t *)out;

    fputs("clrmamepro (\n", ctx->f);
    output_cond_print_string(ctx->f, "\tname ", dat_entry_name(dat), "\n");
    output_cond_print_string(ctx->f, "\tdescription ", (dat_entry_description(dat) ? dat_entry_description(dat) : dat_entry_name(dat)), "\n");
    output_cond_print_string(ctx->f, "\tversion ", dat_entry_version(dat), "\n");
    fputs(")\n\n", ctx->f);

    return 0;
}


static int
cmp_games(const game_t *g1, const game_t *g2) {
    return strcasecmp(game_name(g1), game_name(g2));
}


static int
write_game(output_context_cm_t *ctx, game_t *g) {
    file_t *r;
    int i;
    const char *fl = NULL;

    fputs("game (\n", ctx->f);
    output_cond_print_string(ctx->f, "\tname ", game_name(g), "\n");
    output_cond_print_string(ctx->f, "\tdescription ", game_description(g) ? game_description(g) : game_name(g), "\n");
    output_cond_print_string(ctx->f, "\tcloneof ", game_cloneof(g, 0), "\n");
    output_cond_print_string(ctx->f, "\tromof ", game_cloneof(g, 0), "\n");
    for (i = 0; i < game_num_roms(g); i++) {
	r = game_rom(g, i);

	fputs("\trom ( ", ctx->f);
	output_cond_print_string(ctx->f, "name ", file_name(r), " ");
	if (file_where(r) != FILE_INGAME)
	    output_cond_print_string(ctx->f, "merge ", file_merge(r) ? file_merge(r) : file_name(r), " ");
	fprintf(ctx->f, "size %" PRIu64 " ", file_size_(r));
	output_cond_print_hash(ctx->f, "crc ", HASHES_TYPE_CRC, file_hashes(r), " ");
	output_cond_print_hash(ctx->f, "sha1 ", HASHES_TYPE_SHA1, file_hashes(r), " ");
	output_cond_print_hash(ctx->f, "md5 ", HASHES_TYPE_MD5, file_hashes(r), " ");
	switch (file_status_(r)) {
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
	output_cond_print_string(ctx->f, "flags ", fl, " ");
	fputs(")\n", ctx->f);
    }
    /* TODO: disks */
    fputs(")\n\n", ctx->f);

    return 0;
}
