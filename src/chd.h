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
#include <memory>
#include <string>
#include <vector>

#include <zlib.h>

#include "hashes.h"
#include "SharedFile.h"

#define CHD_FLAG_HAS_PARENT 0x01

class ChdMapEntry {
public:
    uint64_t offset; /* offset within the file of the data */
    uint32_t crc;    /* 32-bit CRC of the data */
    uint16_t length; /* length of the data */
    uint8_t type;    /* map entry type */
    uint8_t flags;   /* misc flags */
};

class Chd {
public:
    Chd(const std::string &name_);

    uint32_t flags;          /* flags field */
    Hashes hashes;
    uint32_t version;        /* drive format version */

private:
    FILEPtr f;

    uint32_t hdr_length;     /* length of header data */
    uint32_t hunk_len;       /* number of bytes per hunk */
    uint64_t total_hunks;    /* total # of hunks represented */
    uint64_t total_len;      /* logical size of the data */
    uint64_t map_offset;     /* offset of hunk map in file */
    uint64_t meta_offset;    /* offset in file of first metadata */
    Hashes parent_hashes;    /* hashes of parent file */
    Hashes raw_hashes;       /* SHA1 checksum of raw data */
    uint32_t compressors[4]; /* compression algorithms used */

    std::vector<ChdMapEntry> map;          /* hunk map */

    void read_header(void);
    void read_header_v5(const uint8_t *header);
};

typedef std::shared_ptr<Chd> ChdPtr;

#endif /* chd.h */
