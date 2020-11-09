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

    if (strcmp(attr, "good") == 0)
	status = STATUS_OK;
    else if (strcmp(attr, "verified") == 0)
	status = STATUS_OK;
    else if (strcmp(attr, "baddump") == 0)
	status = STATUS_BADDUMP;
    else if (strcmp(attr, "nodump") == 0)
	status = STATUS_NODUMP;
    else {
	myerror(ERRFILE, "%d: illegal status '%s'", ctx->lineno, attr);
	return -1;
    }

    if (ft == TYPE_DISK)
	disk_status(ctx->d) = status;
    else
	file_status_(ctx->r) = status;

    return 0;
}


int
parse_file_hash(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
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
	myerror(ERRFILE, "%d: invalid argument for %s", ctx->lineno, hash_type_string(ht));
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

    if (ft == TYPE_DISK)
	disk_merge(ctx->d) = xstrdup(attr);
    else
	file_merge(ctx->r) = xstrdup(attr);

    return 0;
}


/*ARGSUSED3*/
int
parse_file_mtime(parser_context_t *ctx, filetype_t ft, int ht, time_t mtime) {
    if (ft == TYPE_ROM) {
	file_mtime(ctx->r) = mtime;
    }

    return 0;
}


/*ARGSUSED3*/
int
parse_file_name(parser_context_t *ctx, filetype_t ft, int dummy, const char *attr) {
    char *p;

    CHECK_STATE(ctx, PARSE_IN_FILE);

    if (ft == TYPE_DISK)
	disk_name(ctx->d) = xstrdup(attr);
    else {
	file_name(ctx->r) = xstrdup(attr);

	/* TODO: warn about broken dat file? */
	p = file_name(ctx->r);
	while ((p = strchr(p, '\\')))
	    *(p++) = '/';
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
    file_size_(ctx->r) = strtol(attr, NULL, 0);

    return 0;
}


int
parse_file_start(parser_context_t *ctx, filetype_t ft) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    if (ft == TYPE_DISK)
	ctx->d = static_cast<disk_t *>(array_grow(game_disks(ctx->g), reinterpret_cast<void (*)(void *)>(disk_init)));
    else
        ctx->r = static_cast<file_t *>(array_grow(game_roms(ctx->g), reinterpret_cast<void (*)(void *)>(file_init)));

    SET_STATE(ctx, PARSE_IN_FILE);

    return 0;
}


/*ARGSUSED3*/
int
parse_game_cloneof(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    game_cloneof(ctx->g, 0) = xstrdup(attr);

    return 0;
}


int
parse_game_description(parser_context_t *ctx, const char *attr) {
    CHECK_STATE(ctx, PARSE_IN_GAME);

    game_description(ctx->g) = xstrdup(attr);

    return 0;
}


/*ARGSUSED2*/
int
parse_game_end(parser_context_t *ctx, filetype_t ft) {
    game_t *g;
    int keep_g, ret;

    CHECK_STATE(ctx, PARSE_IN_GAME);

    keep_g = ret = 0;

    if (!name_matches(game_name(ctx->g), ctx->ignore)) {
	g = ctx->g;

	/* omit description if same as name (to save space) */
	if (game_name(g) && game_description(g) && strcmp(game_name(g), game_description(g)) == 0) {
	    free(game_description(g));
	    game_description(g) = NULL;
	}

	if (game_cloneof(g, 0)) {
	    if (strcmp(game_cloneof(g, 0), game_name(g)) == 0) {
		free(game_cloneof(g, 0));
		game_cloneof(g, 0) = NULL;
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
parse_game_name(parser_context_t *ctx, filetype_t ft, int ht, const char *attr) {
    size_t i;

    game_name(ctx->g) = xstrdup(attr);

    if (!ctx->full_archive_name) {
	/* slashes are directory separators on some systems, and at least
	 * one redump dat contained a slash in a rom name */
	for (i = 0; i < strlen(game_name(ctx->g)); i++) {
	    if (game_name(ctx->g)[i] == '/') {
		game_name(ctx->g)[i] = '-';
	    }
	}
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

    ctx->g = game_new();

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
    game_free(ctx->g);
    ctx->g = NULL;
    dat_entry_finalize(&ctx->de);

    free(ctx);
}


parser_context_t *
parser_context_new(parser_source_t *ps, const parray_t *exclude, const dat_entry_t *dat, output_context_t *out, int flags) {
    parser_context_t *ctx;

    ctx = (parser_context_t *)xmalloc(sizeof(*ctx));

    dat_entry_merge(&ctx->dat_default, dat, NULL);
    ctx->output = out;
    ctx->ignore = exclude;
    ctx->full_archive_name = flags & PARSER_FL_FULL_ARCHIVE_NAME;

    ctx->state = PARSE_IN_HEADER;

    ctx->ps = ps;
    ctx->lineno = 0;
    dat_entry_init(&ctx->de);
    ctx->g = NULL;
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
    if (hashes_types(disk_hashes(ctx->d)) == 0)
	disk_status(ctx->d) = STATUS_NODUMP;

    if (disk_merge(ctx->d) != NULL && strcmp(disk_name(ctx->d), disk_merge(ctx->d)) == 0) {
	free(disk_merge(ctx->d));
	disk_merge(ctx->d) = NULL;
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
    file_t *r, *r2;
    int deleted;
    int j, n;

    r = ctx->r;
    n = game_num_roms(ctx->g) - 1;

    if (file_size_(r) == 0) {
        unsigned char zeroes[HASHES_SIZE_MAX];
        hashes_t *h = file_hashes(r);
        
        memset(zeroes, 0, sizeof(zeroes));
        
        /* some dats don't record crc for 0-byte files, so set it here */
        if (!hashes_has_type(h, HASHES_TYPE_CRC)) {
            hashes_set(h, HASHES_TYPE_CRC, zeroes);
        }
        
        /* some dats record all-zeroes md5 and sha1 for 0 byte files, fix */
        if (hashes_has_type(h, HASHES_TYPE_MD5) && hashes_verify(h, HASHES_TYPE_MD5, zeroes)) {
            hashes_set(h, HASHES_TYPE_MD5, (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e");
        }
        if (hashes_has_type(h, HASHES_TYPE_SHA1) && hashes_verify(h, HASHES_TYPE_SHA1, zeroes)) {
            hashes_set(h, HASHES_TYPE_SHA1, (const unsigned char *)"\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09");
        }
    }

    /* omit duplicates */
    deleted = 0;

    if (ctx->flags & PARSE_FL_ROM_IGNORE)
	deleted = 1;
    else if (ctx->flags & PARSE_FL_ROM_CONTINUED) {
	r2 = game_rom(ctx->g, n - 1);
	file_size_(r2) += file_size_(r);
	deleted = 1;
    }
    else if (file_name(r) == NULL) {
	myerror(ERRFILE, "%d: roms without name", ctx->lineno);
	deleted = 1;
    }
    for (j = 0; j < n && !deleted; j++) {
	r2 = game_rom(ctx->g, j);
	if (file_compare_sc(r, r2)) {
	    /* TODO: merge in additional hash types? */
	    if (file_compare_n(r, r2)) {
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s)", ctx->lineno, file_name(r));
		deleted = 1;
		break;
	    }
	    else if (file_merge(r2) && file_merge(r) && strcmp(file_merge(r2), file_merge(r)) != 0) {
		/* file_add_altname(r2, file_name(r)); */
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s, merge-name %s)", ctx->lineno, file_name(r), file_merge(r));
		deleted = 1;
		break;
	    }
	}
	else if (file_compare_n(r, r2)) {
	    myerror(ERRFILE, "%d: two different roms with same name (%s)", ctx->lineno, file_name(r));
	    deleted = 1;
	    break;
	}
    }
    if (file_merge(r) && game_cloneof(ctx->g, 0) == NULL) {
	myerror(ERRFILE, "%d: rom '%s' has merge information but game '%s' has no parent", ctx->lineno, file_name(r), game_name(ctx->g));
    }
    if (deleted) {
	ctx->flags = (ctx->flags & PARSE_FL_ROM_CONTINUED) ? 0 : PARSE_FL_ROM_DELETED;
	array_delete(game_roms(ctx->g), n, reinterpret_cast<void (*)(void *)>(file_finalize));
    }
    else {
	ctx->flags = 0;
	if (file_merge(r) != NULL && strcmp(file_name(r), file_merge(r)) == 0) {
	    free(file_merge(r));
	    file_merge(r) = NULL;
	}
    }
}
