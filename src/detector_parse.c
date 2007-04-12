/*
  $NiH: detector_parse.c,v 1.1 2007/04/10 16:26:46 dillo Exp $

  detector_parse.c -- parse clrmamepro header skip detector XML file
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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
    { "and", attr_bit, true, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_AND },
    { "author", NULL, false, NULL, NULL, XT(text_author), 0 },
    { "data", attr_data, true, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_DATA },
    { "file", attr_file, true, XO(test_open), XC(test_close), NULL, 0 },
    { "name", NULL, false, NULL, NULL, XT(text_name), 0 },
    { "or", attr_bit, true, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_OR },
    { "rule", attr_rule, false, XO(rule_open), XC(rule_close), NULL, 0 },
    { "version", NULL, false, NULL, NULL, XT(text_version), 0 },
    { "xor", attr_bit, true, XO(test_open), XC(test_close), NULL, DETECTOR_TEST_XOR },
};
static const int nentities = sizeof(entities)/sizeof(entities[0]);



detector_t *
detector_parse(const char *fname)
{
    struct ctx ctx;
    FILE *f;

    if ((f=fopen(fname, "r")) == NULL) {
	/* XXX: error */
	return NULL;
    }

    ctx.d = detector_new();
    ctx.dr = NULL;
    ctx.dt = NULL;

    if (xmlu_parse(f, &ctx, entities, nentities) < 0
	|| fclose(f) < 0) {
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

    /* XXX: use bsearch? */

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
