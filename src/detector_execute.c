/*
  detector_execute.c -- match file against detector and compute size/hashes
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>

#include "bswap.h"
#include "detector.h"
#include "xmalloc.h"

#define BUF_SIZE	(16*1024)

struct ctx {
    uint64_t bytes_read;
    detector_read_cb cb_read;
    void *ud;
};

static void op_bitswap(uint8_t *, size_t);
static void op_byteswap(uint8_t *, size_t);
static void op_wordswap(uint8_t *, size_t);

/* keep in sync with enum detector_operation in detector.h */

static const struct {
    int align;
    void (*op)(uint8_t *, size_t);
} ops[] = {
    { 0, NULL },
    { 1, op_bitswap },
    { 2, op_byteswap },
    { 4, op_wordswap }
};

static const int nops = sizeof(ops)/sizeof(ops[0]);

static const uint8_t bitswap[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};



static int bit_cmp(const uint8_t *, const uint8_t *, const uint8_t *,
		   detector_test_type_t, size_t);
static void buf_grow(detector_t *, size_t);
static int compute_values(detector_t *, file_t *, int64_t, int64_t,
			  detector_operation_t, struct ctx *);
static int execute_rule(detector_t *, detector_rule_t *, file_t *,
			struct ctx *);
static int execute_test(detector_t *, detector_test_t *, file_t *,
			struct ctx *);
static int fill_buffer(detector_t *, detector_read_arg_t, struct ctx *);



int
detector_execute(detector_t *d, file_t *r, detector_read_cb cb_read,
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
compute_values(detector_t *d, file_t *r, int64_t start, int64_t end,
	       detector_operation_t op, struct ctx *ctx)
{
    hashes_t h;
    hashes_update_t *hu;
    size_t off;
    detector_read_arg_t len;
    unsigned long size;
    int align;

    size = end-start;

    if (start > ctx->bytes_read) {
	/* XXX: read in chunks */
	buf_grow(d, start - ctx->bytes_read);
	if (ctx->cb_read(ctx->ud, d->buf,
			 (detector_read_arg_t)(start - ctx->bytes_read)) < 0)
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

	if (op > 0 && (int)op < nops) {
	    align = ops[op].align;
	    if (len % align != 0) {
		len += align - (len%align);
		if (fill_buffer(d, off+len, ctx) < 0) {
		    hashes_update_final(hu);
		    return -1;
		}
	    }
	    if (off % align != 0) {
		memmove(d->buf, d->buf+off, len);
		off = 0;
	    }
	    ops[op].op(d->buf+off, len);
	}

	hashes_update(hu, d->buf+off, len);
	start += len;
    }

    hashes_update_final(hu);

    file_size_xxx(r, FILE_SH_DETECTOR) = size;
    memcpy(file_hashes_xxx(r, FILE_SH_DETECTOR), &h, sizeof(h));

    return 0;
}



static int
execute_rule(detector_t *d, detector_rule_t *dr, file_t *r, struct ctx *ctx)
{
    int i, ret;
    int64_t start, end;

    start = detector_rule_start_offset(dr);
    if (start < 0)
	start += file_size(r);
    end = detector_rule_end_offset(dr);
    if (end == DETECTOR_OFFSET_EOF)
	end = file_size(r);
    else if (end < 0)
	end += file_size(r);

    if (start < 0 || (uint64_t)start > file_size(r)
	|| end < 0 || (uint64_t)end > file_size(r)
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
execute_test(detector_t *d, detector_test_t *dt, file_t *r, struct ctx *ctx)
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
	    off = file_size(r) + detector_test_offset(dt);

	if (off < 0 || off+detector_test_length(dt) > file_size(r))
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
		if (file_size(r) == ((uint64_t)1) << i) {
		    ret = 1;
		    break;
		}
	}
	else {
	    int cmp;
	    
	    cmp = detector_test_size(dt) - file_size(r);

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
fill_buffer(detector_t *d, detector_read_arg_t len, struct ctx *ctx)
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



static void
op_bitswap(uint8_t *b, size_t len)
{
    size_t i;

    for (i=0; i<len; i++)
	b[i] = bitswap[b[i]];
}



static void
op_byteswap(uint8_t *b, size_t len)
{
    uint16_t *bw;
    size_t i;

    bw = (uint16_t *)b;

    for (i=0; i<len/2; i++)
	bw[i] = bswap16(bw[i]);
}



static void
op_wordswap(uint8_t *b, size_t len)
{
    uint32_t *bl;
    size_t i;

    bl = (uint32_t *)b;

    for (i=0; i<len/4; i++)
	bl[i] = bswap32(bl[i]);
}
