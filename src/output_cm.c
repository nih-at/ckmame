/*
  $NiH: output_cm.c,v 1.1 2006/05/24 09:29:18 dillo Exp $

  output-cm.c -- write games to clrmamepro dat files
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
};

typedef struct output_context_cm output_context_cm_t;



static int output_cm_close(output_context_t *);
static int output_cm_game(output_context_t *, game_t *);
static int output_cm_header(output_context_t *, dat_entry_t *);
static void print_hash(output_context_cm_t *, int, hashes_t *, const char *);
static void print_string(output_context_cm_t *, const char *, const char *,
			 const char *);



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
    ctx->output.output_game = output_cm_game;
    ctx->output.output_header = output_cm_header;

    ctx->f = f;
    ctx->fname = xstrdup(fname);

    return (output_context_t *)ctx;
}



static int
output_cm_close(output_context_t *out)
{
    output_context_cm_t *ctx;
    int ret;

    ctx = (output_context_cm_t *)out;

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else
	ret = fclose(ctx->f);

    free(ctx);

    return ret;
}



static int
output_cm_game(output_context_t *out, game_t *g)
{
    output_context_cm_t *ctx;
    rom_t *r;
    int i;
    char *fl;

    ctx = (output_context_cm_t *)out;

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
