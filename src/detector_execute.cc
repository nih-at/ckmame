/*
  detector_execute.c -- match file against detector and compute size/hashes
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

#include <algorithm>

#include <cstring>

#include "detector.h"

#define BUF_SIZE (static_cast<uint64_t>(16 * 1024))



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

static const size_t nops = sizeof(ops) / sizeof(ops[0]);

static const uint8_t bitswap[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


Detector::Result Detector::execute(File *file, detector_read_cb cb_read, void *ud) const {
    auto ctx = Context(cb_read, ud);

    for (auto &rule : rules) {
        auto result = rule.execute(file, &ctx);
        if (result != MATCH) {
            return result;
        }
    }

    return MATCH;
}


bool Detector::Test::bit_cmp(const uint8_t *b) const {
    switch (type) {
        case TEST_OR:
            for (size_t i = 0; i < length; i++) {
                if ((b[i] | mask[i]) != value[i]) {
                    return false;
                }
            }
            return true;
            
        case TEST_AND:
            for (size_t i = 0; i < length; i++) {
                if ((b[i] & mask[i]) != value[i]) {
                    return false;
                }
            }
            return true;

        case TEST_XOR:
            for (size_t i = 0; i < length; i++) {
                if ((b[i] ^ mask[i]) != value[i]) {
                    return false;
                }
            }
            return true;
            
        default:
            return false;
    }
}


bool Detector::Context::compute_values(File *file, Operation operation, uint64_t start, uint64_t end) {
    auto size = end - start;
    Hashes hashes;
    
    hashes.types = Hashes::TYPE_CRC | Hashes::TYPE_MD5 | Hashes::TYPE_SHA1;
    Hashes::Update hu(&hashes);

    if (start > buf.size()) {
        // skip to start of data we're interested in
        if (!skip(start - buf.size())) {
            return false;
        }
    }
    else if (start < buf.size()) {
        auto length = std::min(end - start, buf.size() - start);
        // make sure we process multiple of 4 bytes
        if (length % 4 != 0) {
            length += 4 - (length % 4);
            if (!fill_buffer(start + length)) {
                return false;
            }
        }
        // we already have relevant data, use it
        update(&hu, operation, start, length);
        start = buf.size();
    }
    
    buf.resize(BUF_SIZE);
    
    while (start < end) {
        auto length = std::min(BUF_SIZE, end - start);
        
        if (!cb_read(ud, buf.data(), length)) {
            return false;
        }
        
        update(&hu, operation, 0, length);
        start += length;
    }
    
    hu.end();

    file->size_detector = size;
    file->hashes_detector = hashes;

    return true;
}

void Detector::Context::update(Hashes::Update *hu, Operation operation, uint64_t offset, uint64_t length) {
    switch (operation) {
        case OP_BITSWAP: {
            for (size_t i = offset; i < offset + length; i++) {
                buf[i] = bitswap[buf[i]];
            }
            break;
        }
            
        case OP_BYTESWAP: {
            auto data = reinterpret_cast<uint16_t *>(buf.data() + offset);
            for (size_t i = 0; i < length / 2; i++) {
                data[i] = static_cast<uint16_t>(data[i] >> 8) | static_cast<uint16_t>(data[i] << 8);
            }
            break;
        }

        case OP_WORDSWAP: {
            auto data = reinterpret_cast<uint32_t *>(buf.data() + offset);
            for (size_t i = 0; i < length / 4; i++) {
                data[i] = (data[i] >> 24) | ((data[i] >> 8) & 0xff00) | ((data[i] << 8) & 0xff0000) | (data[i] << 24);
            }
        }
            
        default:
            break;
    }
    
    hu->update(buf.data() + offset, length);
}


Detector::Result Detector::Rule::execute(File *file, Context *ctx) const {
    auto start = start_offset;
    if (start < 0) {
        start += file->size;
    }
    auto end = end_offset;
    if (end == DETECTOR_OFFSET_EOF) {
        end = static_cast<int64_t>(file->size);
    }
    else if (end < 0) {
        end += file->size;
    }
    
    if (start < 0 || static_cast<uint64_t>(start) > file->size || end < 0 || static_cast<uint64_t>(end) > file->size || start > end) {
	return MISMATCH;
    }

    for (auto &test : tests) {
        auto ret = test.execute(file, ctx);
        if (ret != MATCH) {
            return ret;
        }
    }

    if (!ctx->compute_values(file, operation, static_cast<uint64_t>(start), static_cast<uint64_t>(end))) {
	return ERROR;
    }
    
    return MATCH;
}


Detector::Result Detector::Test::execute(File *file, Context *ctx) const {
    auto match = false;
    
    switch (type) {
        case TEST_DATA:
        case TEST_OR:
        case TEST_AND:
        case TEST_XOR: {
            auto off = offset;
            
            if (off < 0) {
                off += file->size;
            }
            
            if (off < 0 || static_cast<uint64_t>(off) + length > file->size) {
                return MISMATCH;
            }
            
            if (static_cast<uint64_t>(off) + length > ctx->bytes_read) {
                if (!ctx->fill_buffer(static_cast<uint64_t>(off) + length)) {
                    return ERROR;
                }
            }
            
            if (mask.empty()) {
                match = (memcmp(ctx->buf.data() + off, value.data(), length) == 0);
            }
            else {
                match = bit_cmp(ctx->buf.data() + off);
            }
            break;
        }

        case TEST_FILE_EQ:
        case TEST_FILE_LE:
        case TEST_FILE_GR:
            if (offset == DETECTOR_SIZE_POWER_OF_2) {
                match = false;
                for (auto i = 0; i < 64; i++) {
                    if (file->size == (static_cast<uint64_t>(1) << i)) {
                        match = true;
                        break;
                    }
                }
            }
            else {
                int64_t cmp = offset - static_cast<int64_t>(file->size);
                
                switch (type) {
                    case TEST_FILE_EQ:
                        match = (cmp == 0);
                        break;
                    case TEST_FILE_LE:
                        match = (cmp < 0);
                        break;
                    case TEST_FILE_GR:
                        match = (cmp > 0);
                        break;
                        
                    default:
                        match = false;
                }
            }
        }

    if (match) {
        return result ? MATCH : MISMATCH;
    }
    else {
        return result ? MISMATCH : MATCH;
    }
}

bool Detector::Context::fill_buffer(uint64_t length) {
    auto bytes_read = buf.size();

    if (bytes_read < length) {
        buf.resize(length, 0);
        if (cb_read(ud, buf.data() + bytes_read, length - bytes_read) < 0) {
	    return false;
        }
    }
    return true;
}
