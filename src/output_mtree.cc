/*
  output_mtree.c -- write games to mtree(8) files
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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


struct output_context_mtree {
    output_context_t output;

    bool extended;

    FILE *f;
    char *fname;
};

typedef struct output_context_mtree output_context_mtree_t;


static int output_mtree_close(output_context_t *);
static int output_mtree_game(output_context_t *, GamePtr);
static int output_mtree_header(output_context_t *, dat_entry_t *);


output_context_t *
output_mtree_new(const char *fname, int flags) {
    output_context_mtree_t *ctx;
    FILE *f;

    ctx = (output_context_mtree_t *)xmalloc(sizeof(*ctx));

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

    ctx->output.close = output_mtree_close;
    ctx->output.output_detector = NULL;
    ctx->output.output_game = output_mtree_game;
    ctx->output.output_header = output_mtree_header;

    ctx->extended = (flags & OUTPUT_FL_EXTENDED);
    ctx->f = f;
    ctx->fname = xstrdup(fname);

    return (output_context_t *)ctx;
}


static int
output_mtree_close(output_context_t *out) {
    output_context_mtree_t *ctx;
    int ret;

    ctx = (output_context_mtree_t *)out;

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else {
	ret = fclose(ctx->f);
    }

    free(ctx);

    return ret;
}


static char *
strsvis_cstyle(const char *in) {
    char *out;
    int outpos;
    size_t inpos;
    /* maximal extension = 2/char */
    out = (char *)xmalloc(2 * strlen(in));

    outpos = 0;
    for (inpos = 0; inpos < strlen(in); inpos++) {
	switch (in[inpos]) {
	case '\007':
	    out[outpos++] = '\\';
	    out[outpos++] = 'a';
	    break;
	case '\010':
	    out[outpos++] = '\\';
	    out[outpos++] = 'b';
	    break;
	case '\014':
	    out[outpos++] = '\\';
	    out[outpos++] = 'f';
	    break;
	case '\012':
	    out[outpos++] = '\\';
	    out[outpos++] = 'n';
	    break;
	case '\015':
	    out[outpos++] = '\\';
	    out[outpos++] = 'r';
	    break;
	case '\040':
	    out[outpos++] = '\\';
	    out[outpos++] = 's';
	    break;
	case '\011':
	    out[outpos++] = '\\';
	    out[outpos++] = 't';
	    break;
	case '\013':
	    out[outpos++] = '\\';
	    out[outpos++] = 'v';
	    break;
	case '#':
	    out[outpos++] = '\\';
	    out[outpos++] = '#';
	    break;
	default:
	    out[outpos++] = in[inpos];
	    break;
	}
    }
    out[outpos++] = '\0';

    return out;
}


static int
output_mtree_game(output_context_t *out, GamePtr game) {
    auto ctx = reinterpret_cast<output_context_mtree_t *>(out);

    auto dirname = strsvis_cstyle(game->name.c_str());

    fprintf(ctx->f, "./%s type=dir\n", dirname);
    for (size_t i = 0; i < game->roms.size(); i++) {
        auto &rom = game->roms[i];

        auto filename = strsvis_cstyle(rom.name.c_str());
        fprintf(ctx->f, "./%s/%s type=file size=%" PRIu64, dirname, filename, rom.size);
	free(filename);
	output_cond_print_hash(ctx->f, " sha1=", Hashes::TYPE_SHA1, &rom.hashes, "");
        output_cond_print_hash(ctx->f, " md5=", Hashes::TYPE_MD5, &rom.hashes, "");
        const char *fl;
	switch (rom.status) {
            case STATUS_BADDUMP:
                fl = "baddump";
                break;
                
            case STATUS_NODUMP:
                fl = "nodump";
                break;
                
            default:
                fl = NULL;
                break;
        }
	output_cond_print_string(ctx->f, " status=", fl, "");
	if (ctx->extended) {
	    /* crc is not in the standard set supported on NetBSD */
	    output_cond_print_hash(ctx->f, " crc=", Hashes::TYPE_CRC, &rom.hashes, "");
            fprintf(ctx->f, " time=%llu", static_cast<unsigned long long>(rom.mtime));
	}
	fputs("\n", ctx->f);
    }
    for (size_t i = 0; i < game->disks.size(); i++) {
        auto d = &game->disks[i];

        auto filename = strsvis_cstyle(disk_name(d));
	fprintf(ctx->f, "./%s/%s type=file" PRIu64, dirname, filename);
	free(filename);
	output_cond_print_hash(ctx->f, " sha1=", Hashes::TYPE_SHA1, disk_hashes(d), "");
	output_cond_print_hash(ctx->f, " md5=", Hashes::TYPE_MD5, disk_hashes(d), "");
        const char *fl;
	switch (disk_status(d)) {
            case STATUS_BADDUMP:
                fl = "baddump";
                break;
            case STATUS_NODUMP:
                fl = "nodump";
                break;
            default:
                fl = NULL;
                break;
        }
        output_cond_print_string(ctx->f, " status=", fl, "");
	fputs("\n", ctx->f);
    }

    free(dirname);
    return 0;
}


static int
output_mtree_header(output_context_t *out, dat_entry_t *dat) {
    output_context_mtree_t *ctx;

    ctx = (output_context_mtree_t *)out;

    fprintf(ctx->f, ". type=dir\n");

    return 0;
}
