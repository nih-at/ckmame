#ifndef HAD_CHD_H

/*
  chd.h -- accessing chd files
  Copyright (C) 2004-2020 Dieter Baron and Thomas Klausner

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

#include <cinttypes>
#include <string>

#include <stdio.h>
#include <zlib.h>

#include "hashes.h"

#define CHD_ERR_NONE 0    /* N no error */
#define CHD_ERR_OPEN 1    /* S cannot open file */
#define CHD_ERR_READ 2    /* S read error */
#define CHD_ERR_NO_CHD 3  /* N not a CHD file */
#define CHD_ERR_VERSION 4 /* N unsupported version */
#define CHD_ERR_INVAL 5   /* N invalid argument */
#define CHD_ERR_SEEK 6    /* S seek error */
#define CHD_ERR_NOTSUP 7  /* N unsupported chd feature */
#define CHD_ERR_ZLIB 8    /* Z zlib error */
#define CHD_ERR_CRC 9     /* N CRC mismatch */
#define CHD_ERR_NOMEM 10  /* N out of memory */

#define CHD_FLAG_HAS_PARENT 0x01

#define CHD_MAP_TYPE_COMPRESSOR0 0x00
#define CHD_MAP_TYPE_COMPRESSOR1 0x01
#define CHD_MAP_TYPE_COMPRESSOR2 0x02
#define CHD_MAP_TYPE_COMPRESSOR3 0x03
#define CHD_MAP_TYPE_UNCOMPRESSED 0x04
#define CHD_MAP_TYPE_SELF_REF 0x05
#define CHD_MAP_TYPE_PARENT_REF 0x06
#define CHD_MAP_TYPE_MINI 0x07

#define CHD_MAP_FL_NOCRC 0x10


#define CHD_META_FL_CHECKSUM 0x1

#define MAKE_TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define CHD_CODEC_ZLIB MAKE_TAG('z', 'l', 'i', 'b')
#define CHD_CODEC_LZMA MAKE_TAG('l', 'z', 'm', 'a')
#define CHD_CODEC_HUFFMAN MAKE_TAG('h', 'u', 'f', 'f')
#define CHD_CODEC_FLAC MAKE_TAG('f', 'l', 'a', 'c')
#define CHD_CODEC_CD_ZLIB MAKE_TAG('c', 'd', 'z', 'l')
#define CHD_CODEC_CD_LZMA MAKE_TAG('c', 'd', 'l', 'z')
#define CHD_CODEC_CD_FLAC MAKE_TAG('c', 'd', 'f', 'l')
#define CHD_CODEC_AVHUFF MAKE_TAG('a', 'v', 'h', 'u')


struct chd_map_entry {
    uint64_t offset; /* offset within the file of the data */
    uint32_t crc;    /* 32-bit CRC of the data */
    uint16_t length; /* length of the data */
    uint8_t type;    /* map entry type */
    uint8_t flags;   /* misc flags */
};

struct chd_metadata_entry {
    struct chd_metadata_entry *next;
    unsigned char tag[4];
    uint32_t length;
    uint64_t offset;
    uint8_t flags;
};

struct chd {
    FILE *f;
    char *name;
    int error;

    uint32_t hdr_length;     /* length of header data */
    uint32_t version;        /* drive format version */
    uint32_t flags;          /* flags field */
    uint32_t hunk_len;       /* number of bytes per hunk */
    uint64_t total_hunks;    /* total # of hunks represented */
    uint64_t total_len;      /* logical size of the data */
    uint64_t map_offset;     /* offset of hunk map in file */
    uint64_t meta_offset;    /* offset in file of first metadata */
    uint8_t md5[16];         /* MD5 checksum of raw data */
    uint8_t parent_md5[16];  /* MD5 checksum of parent file */
    uint8_t sha1[20];        /* SHA1 checksum of raw data */
    uint8_t parent_sha1[20]; /* SHA1 checksum of parent file */
    uint8_t raw_sha1[20];    /* SHA1 checksum of raw data */
    uint32_t compressors[4]; /* compression algorithms used */

    struct chd_map_entry *map; /* hunk map */
    char *buf;                 /* decompression buffer */
    z_stream z;                /* decompressor */
    uint32_t hno;              /* hunk currently in hbuf */
    unsigned char *hbuf;       /* hunk data buffer */

    struct chd_metadata_entry *meta; /* list of meta data entries */
};


void chd_close(struct chd *);
struct chd_metadata_entry *chd_get_metadata_list(struct chd *);
struct chd *chd_open(const std::string &name, int *errp);
int64_t chd_read_hunk(struct chd *, uint64_t, unsigned char *);
int chd_read_metadata(struct chd *, const struct chd_metadata_entry *, unsigned char *);
int64_t chd_read_range(struct chd *, unsigned char *, uint64_t, uint64_t);
int chd_get_hashes(struct chd *, Hashes *);

#endif /* chd.h */
