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

#include <algorithm>

#include "chd.h"
#include "compat.h"
#include "error.h"
#include "Exception.h"

#define CHD_MAP_TYPE_COMPRESSOR0 0x00
#define CHD_MAP_TYPE_COMPRESSOR1 0x01
#define CHD_MAP_TYPE_COMPRESSOR2 0x02
#define CHD_MAP_TYPE_COMPRESSOR3 0x03
#define CHD_MAP_TYPE_UNCOMPRESSED 0x04
#define CHD_MAP_TYPE_SELF_REF 0x05
#define CHD_MAP_TYPE_PARENT_REF 0x06
#define CHD_MAP_TYPE_MINI 0x07

#define CHD_MAP_FL_NOCRC 0x10

#define MAKE_TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define CHD_CODEC_ZLIB MAKE_TAG('z', 'l', 'i', 'b')
#define CHD_CODEC_LZMA MAKE_TAG('l', 'z', 'm', 'a')
#define CHD_CODEC_HUFFMAN MAKE_TAG('h', 'u', 'f', 'f')
#define CHD_CODEC_FLAC MAKE_TAG('f', 'l', 'a', 'c')
#define CHD_CODEC_CD_ZLIB MAKE_TAG('c', 'd', 'z', 'l')
#define CHD_CODEC_CD_LZMA MAKE_TAG('c', 'd', 'l', 'z')
#define CHD_CODEC_CD_FLAC MAKE_TAG('c', 'd', 'f', 'l')
#define CHD_CODEC_AVHUFF MAKE_TAG('a', 'v', 'h', 'u')

#define MAX_HEADERLEN 124 /* maximum header length */
#define TAG "MComprHD"
#define TAG_LEN 8      /* length of tag */
#define TAG_AND_LEN 12 /* length of tag + header length */

#define META_HEADERLEN 16

#define MAP_ENTRY_SIZE_V12 8 /* size of map entry, versions 1 & 2 */
#define MAP_ENTRY_SIZE_V3 16 /* size of map entry, version 3 */

#define HEADER_LEN_V5 124

#define GET_UINT16(b) (b += 2, static_cast<uint16_t>((static_cast<uint16_t>((b)[-2]) << 8) | static_cast<uint16_t>((b)[-1])))
#define GET_UINT32(b) (b += 4, (static_cast<uint32_t>((b)[-4]) << 24) | (static_cast<uint32_t>((b)[-3]) << 16) | (static_cast<uint32_t>((b)[-2]) << 8) | static_cast<uint32_t>((b)[-1]))
#define GET_UINT64(b) (b += 8, (static_cast<uint64_t>((b)[-8]) << 56) | (static_cast<uint64_t>((b)[-7]) << 48) | (static_cast<uint64_t>((b)[-6]) << 40) | (static_cast<uint64_t>((b)[-5]) << 32) | (static_cast<uint64_t>((b)[-4]) << 24) | (static_cast<uint64_t>((b)[-3]) << 16) | (static_cast<uint64_t>((b)[-2]) << 8) | (static_cast<uint64_t>((b)[-1])))

static uint32_t v4_compressors[] = {0, CHD_CODEC_ZLIB, CHD_CODEC_ZLIB, /* TODO: zlib plus */
				    CHD_CODEC_AVHUFF};

static uint8_t v4_map_types[] = {0, CHD_MAP_TYPE_COMPRESSOR0, CHD_MAP_TYPE_UNCOMPRESSED, CHD_MAP_TYPE_MINI, CHD_MAP_TYPE_SELF_REF, CHD_MAP_TYPE_PARENT_REF};

#if 0
static uint8_t  v5_map_types[] = {
    CHD_MAP_TYPE_UNCOMPRESSED, CHD_MAP_TYPE_SELF_REF, CHD_MAP_TYPE_PARENT_REF, CHD_MAP_TYPE_MINI,
    CHD_MAP_TYPE_COMPRESSOR0, CHD_MAP_TYPE_COMPRESSOR1, CHD_MAP_TYPE_COMPRESSOR2, CHD_MAP_TYPE_COMPRESSOR3
};
#endif


Chd::Chd(const std::string &name) {
    f = make_shared_file(name, "rb");
    if (!f) {
	throw Exception("can't open '" + name + "'").append_system_error();
    }
    
    memset(md5, 0, sizeof(md5));
    memset(parent_md5, 0, sizeof(parent_md5));
    memset(sha1, 0, sizeof(sha1));
    memset(parent_sha1, 0, sizeof(parent_sha1));
    memset(raw_sha1, 0, sizeof(raw_sha1));

    read_header();
}


uint64_t Chd::read_hunk(uint64_t index, void *data) {
    if (index > total_hunks) {
        throw Exception("invalid hunk index");
    }

    read_map();

    if (map[index].length > hunk_len) {
        throw Exception("invalid length for hunk " + std::to_string(index));
    }

    uint32_t compression_type;
    if (map[index].type < 4) {
	compression_type = compressors[map[index].type];
    }
    else {
	compression_type = map[index].type;
    }

    switch (compression_type) {
        case CHD_CODEC_ZLIB: {
            z_stream z;
            uint8_t buf[hunk_len];

	    if (fseeko(f.get(), static_cast<off_t>(map[index].offset), SEEK_SET) == -1) {
                throw Exception("seek error").append_system_error();
	    }
	    if (fread(buf, 1, map[index].length, f.get()) != map[index].length) {
                throw Exception("read error").append_system_error();
	    }

	    z.zalloc = Z_NULL;
	    z.zfree = Z_NULL;
	    z.opaque = NULL;
	    z.next_in = static_cast<Bytef *>(buf);
            z.avail_in = static_cast<uInt>(map[index].length);
	    z.next_out = static_cast<Bytef *>(data);
	    z.avail_out = hunk_len;
	    if (inflateInit2(&z, -MAX_WBITS) != Z_OK) {
                throw Exception("invalid compressed data");
	    }
	    /* TODO: should use Z_FINISH, but that returns Z_BUF_ERROR */
            auto err = inflate(&z, 0);
	    if (err != Z_OK && err != Z_STREAM_END) {
                throw Exception("invalid compressed data");
	    }
            
            if (z.avail_out != 0) {
                throw Exception("short compressed data");
            }
            break;
	}

        case CHD_MAP_TYPE_UNCOMPRESSED:
            if (fseeko(f.get(), static_cast<off_t>(map[index].offset), SEEK_SET) == -1) {
                throw Exception("seek error").append_system_error();
            }
            /* TODO: use hunk_len instead? */
            if (fread(data, 1, map[index].length, f.get()) != map[index].length) {
                throw Exception("read error").append_system_error();
            }
            break;

        case CHD_MAP_TYPE_MINI: {
            auto b = static_cast<uint8_t *>(data);
            
            b[0] = (map[index].offset >> 56) & 0xff;
            b[1] = (map[index].offset >> 48) & 0xff;
            b[2] = (map[index].offset >> 40) & 0xff;
            b[3] = (map[index].offset >> 32) & 0xff;
            b[4] = (map[index].offset >> 24) & 0xff;
            b[5] = (map[index].offset >> 16) & 0xff;
            b[6] = (map[index].offset >> 8) & 0xff;
            b[7] = map[index].offset & 0xff;
            for (uint64_t i = 8; i < hunk_len; i++) {
                b[i] = b[i - 8];
            }
            break;
        }
                
        case CHD_MAP_TYPE_SELF_REF:
            /* TODO: check CRC here too? */
            return read_hunk(map[index].offset, data);
            
        case CHD_MAP_TYPE_PARENT_REF:
            throw Exception("parent chd not supported");
            
        default:
            throw Exception("unsupported compression method" + std::to_string(compression_type));
    }

    if ((map[index].flags & CHD_MAP_FL_NOCRC) == 0) {
        /* TODO: Can n be > INT_MAX? If so, loop */
        if (crc32(0, static_cast<Bytef *>(data), static_cast<uInt>(hunk_len)) != map[index].crc) {
            throw Exception("CRC error");
        }
    }

    return hunk_len;
}


void Chd::read_header(void) {

    unsigned char b[MAX_HEADERLEN];

    if (fread(b, TAG_AND_LEN, 1, f.get()) != 1) {
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
    if (fread(p, len - TAG_AND_LEN, 1, f.get()) != 1) {
        throw Exception("unexpected EOF");
    }

    hdr_length = len;
    version = GET_UINT32(p);

    if (version > 5) {
        throw Exception("unsupported CHD version " + std::to_string(version));
    }

    if (version >= 5) {
	return read_header_v5(b);
    }

    flags = GET_UINT32(p);
    compressors[0] = v4_compressors[GET_UINT32(p)];

    /* TODO: check hdr_length against expected value for version */

    if (version < 3) {
	hunk_len = GET_UINT32(p);
	total_hunks = GET_UINT32(p);
	p += 12; /* skip c/h/s */
	memcpy(md5, p, sizeof(md5));
	p += sizeof(md5);
	memcpy(parent_md5, p, sizeof(parent_md5));
	p += sizeof(parent_md5);

        if (version == 1) {
	    hunk_len *= 512;
        }
        else {
	    hunk_len *= GET_UINT32(p);
        }
	total_len = hunk_len * total_hunks;
	meta_offset = 0;
    }
    else {
	total_hunks = GET_UINT32(p);
	total_len = GET_UINT64(p);
	meta_offset = GET_UINT64(p);

	if (version == 3) {
	    memcpy(md5, p, sizeof(md5));
	    p += sizeof(md5);
	    memcpy(parent_md5, p, sizeof(parent_md5));
	    p += sizeof(parent_md5);
	}

	hunk_len = GET_UINT32(p);

	memcpy(sha1, p, sizeof(sha1));
	p += sizeof(sha1);
	memcpy(parent_sha1, p, sizeof(parent_sha1));
	p += sizeof(parent_sha1);

	if (version == 3)
	    memcpy(raw_sha1, sha1, sizeof(raw_sha1));
	else {
	    memcpy(raw_sha1, p, sizeof(raw_sha1));
	    /* p += sizeof(raw_sha1); */
	}
    }

    map_offset = hdr_length;
}


void Chd::read_header_v5(const uint8_t *header) {
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

    if (hdr_length < HEADER_LEN_V5) {
        throw Exception("unexpected EOF");
    }

    for (int i = 0; i < 4; i++) {
	compressors[i] = GET_UINT32(p);
    }

    total_len = GET_UINT64(p);

    map_offset = GET_UINT64(p);
    meta_offset = GET_UINT64(p);

    hunk_len = GET_UINT32(p);
    total_hunks = (total_len + hunk_len - 1) / hunk_len;

    p += 4; /* unit bytes */

    memcpy(raw_sha1, p, sizeof(raw_sha1));
    p += sizeof(raw_sha1);
    memcpy(sha1, p, sizeof(sha1));
    p += sizeof(sha1);
    memcpy(parent_sha1, p, sizeof(parent_sha1));
    /* p += sizeof(parent_sha1); */

    flags = 0;
    for (size_t i = 0; i < sizeof(parent_sha1); i++) {
	if (parent_sha1[i] != 0) {
	    flags = CHD_FLAG_HAS_PARENT;
	    break;
	}
    }
}


void Chd::read_map(void) {
    if (!map.empty()) {
        return;
    }
    
    if (fseek(f.get(), static_cast<off_t>(map_offset), SEEK_SET) < 0) {
        throw Exception("seek error").append_system_error();
    }

    if (version >= 5) {
        throw Exception("unsupported CHD version " + std::to_string(version));
    }

    uint64_t length;
    if (version < 3) {
	length = MAP_ENTRY_SIZE_V12;
    }
    else {
	length = MAP_ENTRY_SIZE_V3;
    }

    uint8_t b[MAP_ENTRY_SIZE_V3];
    for (uint64_t i = 0; i < total_hunks; i++) {
	if (fread(b, length, 1, f.get()) != 1) {
            throw Exception("read error").append_system_error();
	}
	auto p = b;

	/* TODO: why? */
        if (i == 1832 && version < 3) {
	    version = 3;
        }

	auto entry = ChdMapEntry();

	if (version < 3) {
	    uint64_t v = GET_UINT64(p);
	    entry.offset = v & 0xFFFFFFFFFFFLL;
	    entry.crc = 0;
            // TODO: There are 20 bits left, but length is 16 bits.
	    entry.length = static_cast<uint16_t>(v >> 44);
	    entry.flags = CHD_MAP_FL_NOCRC;
            if (entry.length == hunk_len) {
		entry.type = CHD_MAP_TYPE_UNCOMPRESSED;
            }
            else {
		entry.type = CHD_MAP_TYPE_COMPRESSOR0;
            }
	}
	else {
	    entry.offset = GET_UINT64(p);
	    entry.crc = GET_UINT32(p);
	    entry.length = GET_UINT16(p);
            // TODO: flags is only 8 bits.
	    entry.flags = static_cast<uint8_t>(GET_UINT16(p));
	    entry.type = v4_map_types[entry.flags & 0x0f];
	    entry.flags &= 0xf0;
	}

	map.push_back(entry);
    }
}


void Chd::get_hashes(Hashes *h) {
    try {
        read_map();
    }
    catch (Exception &e) {
        myerror(ERRFILE, "warning: unsupported CHD type, integrity not checked");
        h->types = 0;
        return;
    }

    if (version > 3) {
	/* version 4 only defines hash for SHA1 */
	h->types = Hashes::TYPE_SHA1;
    }

    Hashes h_raw;
    h_raw.types = h->types;

    if (version > 2) {
	h_raw.types |= Hashes::TYPE_SHA1;
    }
    if (version < 4) {
	h_raw.types |= Hashes::TYPE_MD5;
    }

    auto hu = Hashes::Update(&h_raw);

    uint8_t buf[hunk_len];
    uint64_t length = total_len;
    for (uint32_t hunk = 0; hunk < total_hunks; hunk++) {
        uint64_t n = std::min(static_cast<uint64_t>(hunk_len), length);
	read_hunk(hunk, buf);

	hu.update(buf, n);
	length -= n;
    }
    hu.end();

    if ((version < 4 && memcmp(h_raw.md5, md5, Hashes::SIZE_MD5) != 0) || (version > 2 && memcmp(h_raw.sha1, raw_sha1, Hashes::SIZE_SHA1) != 0)) {
        throw Exception("checksum mismatch for raw data");
    }
    if (version < 4) {
        *h = h_raw;
    }
}
