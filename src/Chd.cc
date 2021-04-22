/*
  chd.c -- accessing chd files
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

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

#include "Chd.h"

#include <cstring>

#include "Exception.h"
#include "SharedFile.h"

#define MAX_HEADERLEN 124 /* maximum header length */
#define TAG "MComprHD"
#define TAG_LEN 8      /* length of tag */
#define TAG_AND_LEN 12 /* length of tag + header length */

#define HEADER_LEN_V5 124

#define GET_UINT32(b) (b += 4, (static_cast<uint32_t>((b)[-4]) << 24) | (static_cast<uint32_t>((b)[-3]) << 16) | (static_cast<uint32_t>((b)[-2]) << 8) | static_cast<uint32_t>((b)[-1]))
#define GET_UINT64(b) (b += 8, (static_cast<uint64_t>((b)[-8]) << 56) | (static_cast<uint64_t>((b)[-7]) << 48) | (static_cast<uint64_t>((b)[-6]) << 40) | (static_cast<uint64_t>((b)[-5]) << 32) | (static_cast<uint64_t>((b)[-4]) << 24) | (static_cast<uint64_t>((b)[-3]) << 16) | (static_cast<uint64_t>((b)[-2]) << 8) | (static_cast<uint64_t>((b)[-1])))


Chd::Chd(const std::string &name) {
    unsigned char b[MAX_HEADERLEN];

    auto fp = make_shared_file(name, "rb");

    if (!fp) {
	throw Exception("can't open file " + name).append_system_error();
    }

    if (fread(b, TAG_AND_LEN, 1, fp.get()) != 1) {
        throw Exception("not a CHD file");
    }

    if (memcmp(b, TAG, TAG_LEN) != 0) {
        throw Exception("not a CHD file");
    }

    auto p = b + TAG_LEN;
    uint32_t len = GET_UINT32(p);
    if (len < TAG_AND_LEN || len > MAX_HEADERLEN) {
        throw Exception("not a CHD file");
    }
    if (fread(p, len - TAG_AND_LEN, 1, fp.get()) != 1) {
        throw Exception("unexpected EOF");
    }

    auto version = GET_UINT32(p);

    if (version > 5) {
        throw Exception("unsupported CHD version " + std::to_string(version));
    }

    if (version >= 5) {
        read_header_v5(b, len);
	return;
    }

    /* skip flags */
    p += 4;
    /* skip compressor */
    p += 4;

    /* TODO: check len against expected value for version */

    if (version < 3) {
        auto hunk_len = GET_UINT32(p);
        auto total_hunks = GET_UINT32(p);
        p += 12; /* skip c/h/s */

        hashes.set(Hashes::TYPE_MD5, p);
        p += Hashes::SIZE_MD5;
	/* skip parent hashes */
        p += Hashes::SIZE_MD5;

        if (version == 1) {
            hunk_len *= 512;
        }
        else {
            hunk_len *= GET_UINT32(p);
        }
        total_len = hunk_len * total_hunks;
    }
    else {
	/* skip total number of hunks */
	p += 4;
        total_len = GET_UINT64(p);
	/* skip meta offset */
	p += 8;

        if (version == 3) {
            hashes.set(Hashes::TYPE_MD5, p);
            p += Hashes::SIZE_MD5;
            /* skip parent hashes */
            p += Hashes::SIZE_MD5;
        }

	/* skip hunk length */
	p += 4;

        hashes.set(Hashes::TYPE_SHA1, p);
        p += Hashes::SIZE_SHA1;
	/* skip parent hashes */
        p += Hashes::SIZE_SHA1;
    }
}


void Chd::read_header_v5(const uint8_t *header, uint32_t header_len) {
    /*
    V5 header:

    [  0] char   tag[8];        // 'MComprHD'
    [  8] UINT32 length;        // length of header (including tag and
				// length fields)
    [ 12] UINT32 version;       // drive format version
    [ 16] UINT32 compressors[4];// which custom compressors are used?
    [ 32] UINT64 logicalbytes;  // logical size of the data (in bytes)
    [ 40] UINT64 mapoffset;     // offset to the map
    [ 48] UINT64 metaoffset;    // offset to the first blob of
				// metadata
    [ 56] UINT32 hunkbytes;     // number of bytes per hunk (512k
				// maximum)
    [ 60] UINT32 unitbytes;     // number of bytes per unit within
				// each hunk
    [ 64] UINT8  rawsha1[20];   // raw data SHA1
    [ 84] UINT8  sha1[20];      // combined raw+meta SHA1
    [104] UINT8  parentsha1[20];// combined raw+meta SHA1 of parent
    [124] (V5 header length)
    */

    auto p = header + TAG_AND_LEN + 4;

    if (header_len < HEADER_LEN_V5) {
        throw Exception("unexpected EOF");
    }

    for (int i = 0; i < 4; i++) {
	/* skip compressors */
	p += 4;
    }

    total_len = GET_UINT64(p);

    /* skip map offset */
    p += 8;
    /* skip meta offset */
    p += 8;
    /* skip hunk length */
    p += 4;
    /* skip unit bytes */
    p += 4;

    /* skip raw hashes */
    p += Hashes::SIZE_SHA1;
    hashes.set(Hashes::TYPE_SHA1, p);
    p += Hashes::SIZE_SHA1;
    /* skip parent hashes */
    p += Hashes::SIZE_SHA1;
}
