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

#include <algorithm>

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
    std::vector<GamePtr> games;
};

static struct {
    bool operator()(GamePtr a, GamePtr b) const {
        return a->name < b->name;
    }
} cmp_game;

typedef struct output_context_cm output_context_cm_t;


static int output_cm_close(output_context_t *);
static int output_cm_game(output_context_t *, GamePtr);
static int output_cm_header(output_context_t *, dat_entry_t *);
static int write_game(output_context_cm_t *, Game *);


output_context_t *
output_cm_new(const char *fname, int flags) {
    auto ctx = new output_context_cm_t();
    FILE *f;

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

    return (output_context_t *)ctx;
}


static int
output_cm_close(output_context_t *out) {
    auto ctx = reinterpret_cast<output_context_cm_t *>(out);

    std::sort(ctx->games.begin(), ctx->games.end(), cmp_game);

    for (auto &game : ctx->games) {
        write_game(ctx, game.get());
    }

    int ret = 0;
    
    if (ctx->f != NULL && ctx->f != stdout) {
        ret = fclose(ctx->f);
    }

    free(ctx);

    return ret;
}


static int
output_cm_game(output_context_t *out, GamePtr g) {
    auto ctx = reinterpret_cast<output_context_cm_t *>(out);

    ctx->games.push_back(g);

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
write_game(output_context_cm_t *ctx, Game *game) {
    fputs("game (\n", ctx->f);
    output_cond_print_string(ctx->f, "\tname ", game->name, "\n");
    output_cond_print_string(ctx->f, "\tdescription ", game->description.empty() ? game->name : game->description, "\n");
    output_cond_print_string(ctx->f, "\tcloneof ", game->cloneof[0], "\n");
    output_cond_print_string(ctx->f, "\tromof ", game->cloneof[0], "\n");
    for (auto &rom : game->roms) {
	fputs("\trom ( ", ctx->f);
        output_cond_print_string(ctx->f, "name ", rom.name, " ");
        if (rom.where != FILE_INGAME) {
            output_cond_print_string(ctx->f, "merge ", rom.merge.empty() ? rom.name : rom.merge, " ");
        }
        fprintf(ctx->f, "size %" PRIu64 " ", rom.size);
        output_cond_print_hash(ctx->f, "crc ", Hashes::TYPE_CRC, &rom.hashes, " ");
        output_cond_print_hash(ctx->f, "sha1 ", Hashes::TYPE_SHA1, &rom.hashes, " ");
        output_cond_print_hash(ctx->f, "md5 ", Hashes::TYPE_MD5, &rom.hashes, " ");
        const char *fl = NULL;
        switch (rom.status) {
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
