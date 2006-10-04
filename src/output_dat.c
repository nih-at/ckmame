/*
  $NiH: output_dat.c,v 1.2 2006/05/26 21:46:50 dillo Exp $

  output-dat.c -- write games to clrmamepro dat files
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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
    rom_t *r;
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
