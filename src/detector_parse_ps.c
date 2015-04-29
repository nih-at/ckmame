/*
  detector_parse_ps.c -- parse clrmamepro header skip detector XML file
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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
#include <stdlib.h>
#include <string.h>

#include "detector.h"
#include "myinttypes.h"
#include "util.h"
#include "xmalloc.h"
#include "xmlutil.h"


struct ctx {
    detector_t *d;
    detector_rule_t *dr;
    detector_test_t *dt;
};

struct str_enum {
    const char *name;
    int value;
};


static int parse_enum(int *, const char *, const struct str_enum *, int);
static int parse_hex(detector_test_t *, uint8_t **, const char *);
static int parse_number(int64_t *, const char *);
static int parse_offset(int64_t *, const char *);
static int parse_size(int64_t *, const char *);
static int rule_close(struct ctx *, int);
static int rule_end_offset(struct ctx *, int, int, const char *);
static int rule_open(struct ctx *, int);
static int rule_operation(struct ctx *, int, int, const char *);
static int rule_start_offset(struct ctx *, int, int, const char *);
static int test_close(struct ctx *, int);
static int test_mask(struct ctx *, int, int, const char *);
static int test_offset(struct ctx *, int, int, const char *);
static int test_open(struct ctx *, int);
static int test_operator(struct ctx *ctx, int, int, const char *);
static int test_result(struct ctx *, int, int, const char *);
static int test_size(struct ctx *, int, int, const char *);
static int test_value(struct ctx *, int, int, const char *);
static int text_author(struct ctx *, const char *);
static int text_name(struct ctx *, const char *);
static int text_version(struct ctx *, const char *);


#define XA(f)	((xmlu_attr_cb)f)
#define XC(f)	((xmlu_tag_cb)f)
#define XO(f)	((xmlu_tag_cb)f)
#define XT(f)	((xmlu_text_cb)f)

static const xmlu_attr_t attr_bit[] = {
    { "offset",       XA(test_offset), 0, 0 },
    { "mask",         XA(test_mask), 0, 0 },
    { "value",        XA(test_value), 0, 0 },
    { "result",       XA(test_result), 0, 0 },
    { NULL }
};

static const xmlu_attr_t attr_data[] = {
    { "offset",       XA(test_offset), 0, 0 },
    { "value",        XA(test_value), 0, 0 },
    { "result",       XA(test_result), 0, 0 },
    { NULL }
};

static const xmlu_attr_t attr_file[] = {
    { "size",         XA(test_size), 0, 0 },
    { "operator",     XA(test_operator), 0, 0 },
    { "result",       XA(test_result), 0, 0 },
    { NULL }
};

static const xmlu_attr_t attr_rule[] = {
    { "start_offset", XA(rule_start_offset), 0, 0 },
    { "end_offset",   XA(rule_end_offset), 0, 0 },
    { "operation",    XA(rule_operation), 0, 0 },
    { NULL }
};

static const xmlu_entity_t entities[] = {
    { "and", attr_bit, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_AND },
    { "author", NULL, NULL, NULL, XT(text_author), 0 },
    { "data", attr_data, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_DATA },
    { "file", attr_file, XO(test_open), XC(test_close), NULL, 0 },
    { "name", NULL, NULL, NULL, XT(text_name), 0 },
    { "or", attr_bit, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_OR },
    { "rule", attr_rule, XO(rule_open), XC(rule_close), NULL, 0 },
    { "version", NULL, NULL, NULL, XT(text_version), 0 },
    { "xor", attr_bit, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_XOR },
};
static const int nentities = sizeof(entities)/sizeof(entities[0]);


detector_t *
detector_parse_ps(parser_source_t *ps)
{
    struct ctx ctx;

    ctx.d = detector_new();
    ctx.dr = NULL;
    ctx.dt = NULL;

    /* TODO: lineno callback */
    if (xmlu_parse(ps, &ctx, NULL, entities, nentities) < 0) {
	detector_free(ctx.d);
	return NULL;
    }

    return ctx.d;
}


static int
parse_enum(int *ep, const char *value,
	   const struct str_enum *enums, int nenums)
{
    int i;

    /* TODO: use bsearch? */

    for (i=0; i<nenums; i++) {
	if (strcmp(value, enums[i].name) == 0) {
	    *ep = enums[i].value;
	    return 0;
	}
    }

    errno = EINVAL;
    return -1;
}


static int
parse_hex(detector_test_t *dt, uint8_t **vp, const char *value)
{
    uint64_t len;
    uint8_t *v;

    len = strlen(value);

    if (len % 2) {
	errno = EINVAL;
	return -1;
    }
    len /= 2;

    if (dt->length != 0 && dt->length != len) {
	errno = EINVAL;
	return -1;
    }

    v = xmalloc(len);
    if (hex2bin(v, value, len) < 0) {
	free(v);
	errno = EINVAL;
	return -1;
    }

    dt->length = len;
    *vp = v;
    return 0;
}


static int
parse_number(int64_t *offsetp, const char *value)
{
    intmax_t i;
    char *end;

    if (value[0] == '\0') {
	errno = EINVAL;
	return -1;
    }

    errno = 0;
    i = strtoimax(value, &end, 16);

    if (*end != '\0') {
	errno = EINVAL;
	return -1;
    }

    if (errno == ERANGE || i < INT64_MIN || i > INT64_MAX) {
	errno = ERANGE;
	return -1;
    }

    *offsetp = i;
    return 0;
}


static int
parse_offset(int64_t *offsetp, const char *value)
{
    if (strcmp(value, "EOF") == 0) {
	*offsetp = DETECTOR_OFFSET_EOF;
	return 0;
    }

    return parse_number(offsetp, value);
}


static int
parse_size(int64_t *offsetp, const char *value)
{
    if (strcmp(value, "PO2") == 0) {
	*offsetp = DETECTOR_SIZE_PO2;
	return 0;
    }

    return parse_number(offsetp, value);
}


static int
rule_close(struct ctx *ctx, int arg1)
{
    ctx->dr = NULL;

    return 0;
}


static int
rule_end_offset(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_offset(&detector_rule_end_offset(ctx->dr), value);
}


static int
rule_open(struct ctx *ctx, int arg1)
{
    ctx->dr = array_grow(detector_rules(ctx->d), detector_rule_init);

    return 0;
}


static int
rule_operation(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    static const struct str_enum op[] = {
	{ "bitswap", DETECTOR_OP_BITSWAP },
	{ "byteswap", DETECTOR_OP_BYTESWAP },
	{ "none", DETECTOR_OP_NONE },
	{ "wordswap", DETECTOR_OP_WORDSWAP }
    };
    static const int nop = sizeof(op)/sizeof(op[0]);

    int i;

    if (parse_enum(&i, value, op, nop) < 0)
	return -1;

    detector_rule_operation(ctx->dr) = i;
    return 0;
}


static int
rule_start_offset(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_offset(&detector_rule_start_offset(ctx->dr), value);
}


static int
test_close(struct ctx *ctx, int arg1)
{
    ctx->dt = NULL;

    return 0;
}


static int
test_mask(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_hex(ctx->dt, &detector_test_mask(ctx->dt), value);
}


static int
test_offset(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_offset(&detector_test_offset(ctx->dt), value);
}


static int
test_open(struct ctx *ctx, int type)
{
    ctx->dt = array_grow(detector_rule_tests(ctx->dr), detector_test_init);
    detector_test_type(ctx->dt) = type;

    return 0;
}


static int
test_operator(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    static const struct str_enum en[] = {
	{ "equal", DETECTOR_TEST_FILE_EQ },
	{ "greater", DETECTOR_TEST_FILE_GR },
	{ "less", DETECTOR_TEST_FILE_LE },
    };
    static const int nen = sizeof(en)/sizeof(en[0]);

    int i;

    if (parse_enum(&i, value, en, nen) < 0)
	return -1;

    detector_test_type(ctx->dt) = i;
    return 0;
}


static int
test_result(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    static const struct str_enum en[] = {
	{ "false", false },
	{ "true", true },
    };
    static const int nen = sizeof(en)/sizeof(en[0]);

    int i;

    if (parse_enum(&i, value, en, nen) < 0)
	return -1;

    detector_test_result(ctx->dt) = i;
    return 0;
}


static int
test_size(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_size(&detector_test_size(ctx->dt), value);
}


static int
test_value(struct ctx *ctx, int arg1, int arg2, const char *value)
{
    return parse_hex(ctx->dt, &detector_test_value(ctx->dt), value);
}


static int
text_author(struct ctx *ctx, const char *txt)
{
    detector_author(ctx->d) = xstrdup(txt);

    return 0;
}


static int
text_name(struct ctx *ctx, const char *txt)
{
    detector_name(ctx->d) = xstrdup(txt);

    return 0;
}


static int
text_version(struct ctx *ctx, const char *txt)
{
    detector_version(ctx->d) = xstrdup(txt);

    return 0;
}
