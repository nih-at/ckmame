/*
  parse.c -- parser frontend
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

#include "compat.h"
#include "dat.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "parse.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

#define CHECK_STATE(ctx, s)                                        \
    do {                                                           \
	if ((ctx)->state != (s)) {                                 \
	    myerror(ERRFILE, "%d: in wrong state", (ctx)->lineno); \
	    return -1;                                             \
	}                                                          \
    } while (0)

#define SET_STATE(ctx, s) ((ctx)->state = (s))


static int parse_header_end(parser_context_t *);

static void disk_end(parser_context_t *);
static void rom_end(parser_context_t *, filetype_t);


int
parse(parser_source_t *ps, const parray_t *exclude, const dat_entry_t *dat, output_context_t *out, int flags) {
    parser_context_t *ctx;
    int c, ret;

    ctx = parser_context_new(ps, exclude, dat, out, flags);

    c = ps_peek(ps);

    switch (c) {
#if defined(HAVE_LIBXML2)
    case '<':
	ret = parse_xml(ps, ctx);
	break;
#endif
    case '[':
	ret = parse_rc(ps, ctx);
	break;
    default:
	ret = parse_cm(ps, ctx);
    }

    if (ret == 0) {
	if (parse_eof(ctx) < 0) {
	    ret = -1;
	}
    }
    parser_context_free(ctx);

    return ret;
}


int
parse_eof(parser_context_t *ctx) {
    if (ctx->state == PARSE_IN_HEADER) {
	if (parse_header_end(ctx) < 0) {
	    return -1;
	}
    }

    return 0;
}


/*ARGSUSED3*/
/*ARGSUSED4*/
int
parse_file_continue(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
	myerror(ERRFILE, "%d: file continuation only supported for ROMs", ctx->lineno);
	return -1;
    }

    if (ctx->flags & PARSE_FL_ROM_DELETED) {
	myerror(ERRFILE, "%d: internal error: trying to continue deleted file", ctx->lineno);
	return -1;
    }

    ctx->flags |= PARSE_FL_ROM_CONTINUED;

    return 0;
}


int
parse_file_end(parser_context_t *ctx, filetype_t ft) {
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
parse_file_status_(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    status_t status;

    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (strcmp(attr, "good") == 0) {
	status = STATUS_OK;
    }
    else if (strcmp(attr, "verified") == 0) {
	status = STATUS_OK;
    }
    else if (strcmp(attr, "baddump") == 0) {
	status = STATUS_BADDUMP;
    }
    else if (strcmp(attr, "nodump") == 0) {
	status = STATUS_NODUMP;
    }
    else {
	myerror(ERRFILE, "%d: illegal status '%s'", ctx->lineno, attr);
	return -1;
    }
 
    if (ft == TYPE_DISK) {
	disk_status(ctx->d) = status;
    }
    else {
	ctx->r->status = status;
    }

    return 0;
}


int
parse_file_hash(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    Hashes *h;

    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (strcmp(attr, "-") == 0) {
	/* some dat files record crc - for 0-byte files, so skip it */
	return 0;
    }

    if (ft == TYPE_DISK) {
	h = disk_hashes(ctx->d);
    }
    else {
        h = &ctx->r->hashes;
    }

    if (h->set_from_string(attr) != ht) {
	myerror(ERRFILE, "%d: invalid argument for %s", ctx->lineno, Hashes::type_name(ht).c_str());
	return -1;
    }

    return 0;
}


/*ARGSUSED3*/
/*ARGSUSED4*/
int
parse_file_ignore(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
	myerror(ERRFILE, "%d: file ignoring only supported for ROMs", ctx->lineno);
	return -1;
    }

    ctx->flags |= PARSE_FL_ROM_IGNORE;

    return 0;
}


/*ARGSUSED3*/
int
parse_file_merge(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        disk_merge(ctx->d) = xstrdup(attr);
    }
    else {
	ctx->r->merge = attr;
    }

    return 0;
}


/*ARGSUSED3*/
int
parse_file_mtime(parser_context_t *ctx, filetype_t ft, int ht, time_t mtime) {
    if (ft == TYPE_ROM) {
        ctx->r->mtime = mtime;
    }

    return 0;
}


/*ARGSUSED3*/
int
parse_file_name(parser_context_t *ctx, filetype_t ft, int dummy, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
	disk_name(ctx->d) = xstrdup(attr);
    }
    else {
        auto name = std::string(attr);
        
        /* TODO: warn about broken dat file? */
        std::replace(name.begin(), name.end(), '\\', '/');
        
        ctx->r->name = name;
    }

    return 0;
}


/*ARGSUSED3*/
int
parse_file_size_(parser_context_t *ctx, filetype_t ft, int dummy, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
	myerror(ERRFILE, "%d: unknown attribute `size' for disk", ctx->lineno);
	return -1;
    }

    /* TODO: check for strol errors */
    ctx->r->size = strtol(attr, NULL, 0);

    return 0;
}


int
parse_file_start(parser_context_t *ctx, filetype_t ft) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    if (ft == TYPE_DISK) {
        ctx->g->disks.push_back(Disk());
        ctx->d = &ctx->g->disks[ctx->g->disks.size() - 1];
    }
    else {
        ctx->g->roms.push_back(File());
        ctx->r = &ctx->g->roms[ctx->g->roms.size() - 1];
    }

    SET_STATE(ctx, PARSE_IN_FILE);

    return 0;
}


/*ARGSUSED3*/
int
parse_game_cloneof(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    ctx->g->cloneof[0] = attr;

    return 0;
}


int
parse_game_description(parser_context_t *ctx, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    ctx->g->description = attr;

    return 0;
}


/*ARGSUSED2*/
int
parse_game_end(parser_context_t *ctx, filetype_t ft) {
    CHECK_STATE(ctx, PARSE_IN_GAME);
    
    int ret = 0;
    
    if (!name_matches(ctx->g->name.c_str(), ctx->ignore)) {
        Game *game = ctx->g.get();

	/* omit description if same as name (to save space) */
        if (game->name == game->description) {
            game->description = "";
	}

        if (!game->cloneof[0].empty()) {
	    if (game->cloneof[0] == game->name) {
                game->cloneof[0] = "";
	    }
	}

	ret = output_game(ctx->output, ctx->g);
	if (ret == 1) {
	    ret = 0;
	}
    }

    ctx->g = NULL;

    SET_STATE(ctx, PARSE_OUTSIDE);

    return ret;
}


/*ARGSUSED3*/
int
parse_game_name(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    ctx->g->name = attr;

    if (!ctx->full_archive_name) {
	/* slashes are directory separators on some systems, and at least
	 * one redump dat contained a slash in a rom name */
        std::replace(ctx->g->name.begin(), ctx->g->name.end(), '/', '-');
    }

    return 0;
}


/*ARGSUSED2*/
int
parse_game_start(parser_context_t *ctx, filetype_t ft) {
    if (ctx->state == PARSE_IN_HEADER) {
	if (parse_header_end(ctx) < 0) {
	    return -1;
	}
    }

    CHECK_STATE(ctx, PARSE_OUTSIDE);

    if (ctx->g) {
	myerror(ERRFILE, "%d: game inside game", ctx->lineno);
	return -1;
    }

    ctx->g = std::make_shared<Game>();

    SET_STATE(ctx, PARSE_IN_GAME);

    return 0;
}


int
parse_prog_description(parser_context_t *ctx, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_HEADER);

    dat_entry_description(&ctx->de) = xstrdup(attr);

    return 0;
}


/* ARGSUSED3 */
int
parse_prog_header(parser_context_t *ctx, const char *name, int dummy) {
    parser_source_t *ps;
    int ret;

    CHECK_STATE(ctx, PARSE_IN_HEADER);

    if (detector != 0) {
	myerror(ERRFILE, "%d: warning: detector already defined, header '%s' ignored", ctx->lineno, name);
	return 0;
    }

    if ((ps = ps_open(ctx->ps, name)) == NULL) {
	myerror(ERRFILESTR, "%d: cannot open detector '%s'", ctx->lineno, name);
	return -1;
    }
#if defined(HAVE_LIBXML2)
    if ((detector = detector_parse_ps(ps)) == NULL) {
	myerror(ERRFILESTR, "%d: cannot parse detector '%s'", ctx->lineno, name);
	ret = -1;
    }
    else
#endif
	ret = output_detector(ctx->output, detector);

    ps_close(ps);

    return ret;
}


int
parse_prog_name(parser_context_t *ctx, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_HEADER);

    dat_entry_name(&ctx->de) = xstrdup(attr);

    return 0;
}


int
parse_prog_version(parser_context_t *ctx, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_HEADER);

    dat_entry_version(&ctx->de) = xstrdup(attr);

    return 0;
}


void
parser_context_free(parser_context_t *ctx) {
    dat_entry_finalize(&ctx->de);

    delete ctx;
}


parser_context_t *
parser_context_new(parser_source_t *ps, const parray_t *exclude, const dat_entry_t *dat, output_context_t *out, int flags) {
    parser_context_t *ctx;

    ctx = new parser_context_t();

    dat_entry_merge(&ctx->dat_default, dat, NULL);
    ctx->output = out;
    ctx->ignore = exclude;
    ctx->full_archive_name = flags & PARSER_FL_FULL_ARCHIVE_NAME;

    ctx->state = PARSE_IN_HEADER;

    ctx->ps = ps;
    ctx->lineno = 0;
    dat_entry_init(&ctx->de);
    ctx->flags = 0;

    return ctx;
}


static int
parse_header_end(parser_context_t *ctx) {
    dat_entry_t de;

    CHECK_STATE(ctx, PARSE_IN_HEADER);

    dat_entry_merge(&de, &ctx->dat_default, &ctx->de);
    output_header(ctx->output, &de);
    dat_entry_finalize(&de);

    SET_STATE(ctx, PARSE_OUTSIDE);

    return 0;
}


static void
disk_end(parser_context_t *ctx) {
    if (disk_hashes(ctx->d)->empty()) {
	disk_status(ctx->d) = STATUS_NODUMP;
    }

    if (!ctx->d->merge.empty() && ctx->d->name == ctx->d->merge) {
        ctx->d->merge = "";
    }
}


int
name_matches(const char *name, const parray_t *patterns) {
    int i;

    if (patterns == NULL)
	return 0;

    for (i = 0; i < parray_length(patterns); i++) {
	if (fnmatch(static_cast<const char *>(parray_get(patterns, i)), name, 0) == 0)
	    return 1;
    }

    return 0;
}


static void
rom_end(parser_context_t *ctx, filetype_t ft) {
    auto rom = ctx->r;
    size_t n = ctx->g->roms.size() - 1;

    if (rom->size == 0) {
        unsigned char zeroes[Hashes::MAX_SIZE];

        memset(zeroes, 0, sizeof(zeroes));
        
        /* some dats don't record crc for 0-byte files, so set it here */
        if (!rom->hashes.has_type(Hashes::TYPE_CRC)) {
            rom->hashes.set(Hashes::TYPE_CRC, zeroes);
        }
        
        /* some dats record all-zeroes md5 and sha1 for 0 byte files, fix */
        if (rom->hashes.has_type(Hashes::TYPE_MD5) && rom->hashes.verify(Hashes::TYPE_MD5, zeroes)) {
            rom->hashes.set(Hashes::TYPE_MD5, (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e");
        }
        if (rom->hashes.has_type(Hashes::TYPE_SHA1) && rom->hashes.verify(Hashes::TYPE_SHA1, zeroes)) {
            rom->hashes.set(Hashes::TYPE_SHA1, (const unsigned char *)"\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09");
        }
    }

    /* omit duplicates */
    auto deleted = false;

    if (ctx->flags & PARSE_FL_ROM_IGNORE) {
        deleted = true;
    }
    else if (ctx->flags & PARSE_FL_ROM_CONTINUED) {
        auto &rom2 = ctx->g->roms[n - 1];
        rom2.size += rom->size;
	deleted = true;
    }
    else if (rom->name.empty()) {
	myerror(ERRFILE, "%d: ROM without name", ctx->lineno);
	deleted = true;
    }
    for (size_t j = 0; j < n && !deleted; j++) {
        auto &rom2 = ctx->g->roms[j];
        if (rom->compare_size_crc(rom2)) {
	    /* TODO: merge in additional hash types? */
            if (rom->compare_name(rom2)) {
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s)", ctx->lineno, rom->name.c_str());
		deleted = true;
		break;
	    }
            else if (!rom2.merge.empty() && rom->merge == rom2.merge) {
		/* file_add_altname(r2, file_name(r)); */
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s, merge-name %s)", ctx->lineno, rom->name.c_str(), rom->merge.c_str());
		deleted = true;
		break;
	    }
	}
        else if (rom->compare_name(rom2)) {
	    myerror(ERRFILE, "%d: two different roms with same name (%s)", ctx->lineno, rom->name.c_str());
	    deleted = true;
	    break;
	}
    }
    if (!rom->merge.empty() && ctx->g->cloneof[0].empty()) {
        myerror(ERRFILE, "%d: rom '%s' has merge information but game '%s' has no parent", ctx->lineno, rom->name.c_str(), ctx->g->name.c_str());
    }
    if (deleted) {
	ctx->flags = (ctx->flags & PARSE_FL_ROM_CONTINUED) ? 0 : PARSE_FL_ROM_DELETED;
        ctx->g->roms.pop_back();
    }
    else {
	ctx->flags = 0;
        if (!rom->merge.empty() && rom->merge == rom->name) {
            rom->merge = "";
	}
    }
}
