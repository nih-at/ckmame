/*
  $NiH$

  detector_execute.c -- match file against detector and compute size/hashes
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



#include <stdlib.h>

#include "detector.h"
#include "xmalloc.h"

#define BUF_SIZE	(16*1024)

struct ctx {
    size_t bytes_read;
    detector_read_cb cb_read;
    void *ud;
};

static int bit_cmp(const uint8_t *, const uint8_t *, const uint8_t *,
		   detector_test_type_t, size_t);
static void buf_grow(detector_t *, size_t);
static int compute_values(detector_t *, rom_t *, int64_t, int64_t,
			  detector_operation_t, struct ctx *);
static int execute_rule(detector_t *, detector_rule_t *, rom_t *,
			struct ctx *);
static int execute_test(detector_t *, detector_test_t *, rom_t *,
			struct ctx *);
static int fill_buffer(detector_t *, size_t, struct ctx *);



int
detector_execute(detector_t *d, rom_t *r, detector_read_cb cb_read,
		 void *ud)
{
    struct ctx ctx;
    int i, ret;

    ctx.bytes_read = 0;
    ctx.cb_read = cb_read;
    ctx.ud = ud;

    for (i=0; i<detector_num_rules(d); i++) {
	if ((ret=execute_rule(d, detector_rule(d, i), r, &ctx)) != 0)
	    return ret;
    }

    return 0;
}



static int
bit_cmp(const uint8_t *b, const uint8_t *value, const uint8_t *mask,
	detector_test_type_t type, size_t length)
{
    size_t i;

    switch (type) {
    case DETECTOR_TEST_OR:
	for (i=0; i<length; i++) {
	    if ((b[i]|mask[i]) != value[i])
		return 0;
	}
	return 1;

    case DETECTOR_TEST_AND:
	for (i=0; i<length; i++) {
	    if ((b[i]&mask[i]) != value[i])
		return 0;
	}
	return 1;

    case DETECTOR_TEST_XOR:
	for (i=0; i<length; i++) {
	    if ((b[i]^mask[i]) != value[i])
		return 0;
	}
	return 1;

    default:
	return 0;
    }
}



static void
buf_grow(detector_t *d, size_t size)
{
    if (d->buf_size < size) {
	d->buf = xrealloc(d->buf, size);
	d->buf_size = size;
    }
}



static int
compute_values(detector_t *d, rom_t *r, int64_t start, int64_t end,
	       detector_operation_t op, struct ctx *ctx)
{
    hashes_t h;
    hashes_update_t *hu;
    size_t len, off;
    unsigned long size;

    size = end-start;

    if (start > ctx->bytes_read) {
	/* XXX: read in chunks */
	buf_grow(d, start - ctx->bytes_read);
	if (ctx->cb_read(ctx->ud, d->buf, start - ctx->bytes_read) < 0)
	    return -1;
	
    }

    hashes_types(&h) = HASHES_TYPE_CRC|HASHES_TYPE_MD5|HASHES_TYPE_SHA1; /* XXX */
    hu = hashes_update_new(&h);

    while (start < end) {
	if (start < ctx->bytes_read) {
	    off = start;
	    len = end-start;
	    if (len > ctx->bytes_read - start)
		len = ctx->bytes_read - start;
	}
	else {
	    buf_grow(d, BUF_SIZE);
	    off = 0;
	    len = BUF_SIZE;
	    if (len > end-start)
		len = end-start;

	    if (ctx->cb_read(ctx->ud, d->buf, len) < 0) {
		hashes_update_final(hu);
		return -1;
	    }
	}

	/* XXX: apply operation */

	hashes_update(hu, d->buf+off, len);
	start += len;
    }

    hashes_update_final(hu);

    rom_size(r) = size;
    memcpy(rom_hashes(r), &h, sizeof(h));

    return 0;
}



static int
execute_rule(detector_t *d, detector_rule_t *dr, rom_t *r, struct ctx *ctx)
{
    int i, ret;
    int64_t start, end;

    start = detector_rule_start_offset(dr);
    if (start < 0)
	start += rom_size(r);
    end = detector_rule_end_offset(dr);
    if (end == DETECTOR_OFFSET_EOF)
	end = rom_size(r);
    else if (end < 0)
	end += rom_size(r);

    if (start < 0 || start > rom_size(r)
	|| end < 0 || end > rom_size(r)
	|| start > end)
	return 0;

    for (i=0; i<detector_rule_num_tests(dr); i++) {
	if ((ret=execute_test(d, detector_rule_test(dr, i), r, ctx)) != 1)
	    return ret;
    }

    if (compute_values(d, r, start, end, detector_rule_operation(dr), ctx) < 0)
	return -1;
    return 1;
}



static int
execute_test(detector_t *d, detector_test_t *dt, rom_t *r, struct ctx *ctx)
{
    int64_t off;
    int ret;

    switch (detector_test_type(dt)) {
    case DETECTOR_TEST_DATA:
    case DETECTOR_TEST_OR:
    case DETECTOR_TEST_AND:
    case DETECTOR_TEST_XOR:
	off = detector_test_offset(dt);
	if (off < 0)
	    off = rom_size(r) + detector_test_offset(dt);

	if (off < 0 || off+detector_test_length(dt) > rom_size(r))
	    return 0;

	if (off+detector_test_length(dt) > ctx->bytes_read) {
	    if (fill_buffer(d, off+detector_test_length(dt), ctx) < 0)
		return -1;
	}

	if (detector_test_mask(dt) == 0)
	    ret = (memcmp(d->buf+off, detector_test_value(dt),
			  detector_test_length(dt)) == 0);
	else
	    ret = bit_cmp(d->buf+off, detector_test_value(dt),
			  detector_test_mask(dt), detector_test_type(dt),
			  detector_test_length(dt));
	break;

    case DETECTOR_TEST_FILE_EQ:
    case DETECTOR_TEST_FILE_LE:
    case DETECTOR_TEST_FILE_GR:
	if (detector_test_size(dt) == DETECTOR_SIZE_PO2) {
	    int i;

	    ret = 0;
	    for (i=0; i<64; i++)
		if (rom_size(r) == ((uint64_t)1) << i) {
		    ret = 1;
		    break;
		}
	}
	else {
	    int cmp;
	    
	    cmp = detector_test_size(dt) - rom_size(r);

	    switch (detector_test_type(dt)) {
	    case DETECTOR_TEST_FILE_EQ:
		ret = (cmp == 0);
		break;
	    case DETECTOR_TEST_FILE_LE:
		ret = (cmp < 0);
		break;
	    case DETECTOR_TEST_FILE_GR:
		ret = (cmp > 0);
		break;

	    default:
		ret = 0;
	    }
	}
    }

    if (ret)
	return detector_test_result(dt);
    else
	return !detector_test_result(dt);
}



static int
fill_buffer(detector_t *d, size_t len, struct ctx *ctx)
{
    if (ctx->bytes_read < len) {
	buf_grow(d, len);
	len -= ctx->bytes_read;
	if (ctx->cb_read(ctx->ud, d->buf+ctx->bytes_read, len) < 0)
	    return -1;
	ctx->bytes_read += len;
    }
    return 0;
}

