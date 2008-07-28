/*
  output-dat.c -- write games to clrmamepro dat files
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



struct output_context_dat {
    output_context_t output;

    FILE *f;
    char *fname;
};

typedef struct output_context_dat output_context_dat_t;



static int output_dat_close(output_context_t *);
static int output_dat_game(output_context_t *, game_t *);
static int output_dat_header(output_context_t *, dat_entry_t *);



output_context_t *
output_dat_new(const char *fname)
{
    output_context_dat_t *ctx;
    FILE *f;

    ctx = (output_context_dat_t *)xmalloc(sizeof(*ctx));

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

    ctx->output.close = output_dat_close;
    ctx->output.output_game = output_dat_game;
    ctx->output.output_header = output_dat_header;

    ctx->f = f;
    ctx->fname = xstrdup(f);

    return (output_context_t *)ctx;
}



static int
output_dat_close(output_context_t *out)
{
    output_context_dat_t *ctx;
    int ret;

    ctx = (output_context_dat_t *)out;

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else
	ret = fclose(ctx->f);

    free(ctx);

    return ret;
}



static int
output_dat_game(output_context_t *out, game_t *g)
{
    output_context_dat_t *ctx;
    file_t *r;
    disk_t *d;
    int i;
    game_t *parent;

    ctx = (output_context_dat_t *)out;

    /* XXX */

    return 0;
}



static int
output_dat_header(output_context_t *out, dat_entry_t *dat)
{
    output_context_dat_t *ctx;

    ctx = (output_context_dat_t *)out;

    /* XXX */

    return 0;
}
