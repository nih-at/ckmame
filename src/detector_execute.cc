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

#include "Detector.h"

#include <cstring>

const uint64_t Detector::MAX_DETECTOR_FILE_SIZE = 128 * 1024 * 1024;

#define BUF_SIZE (static_cast<uint64_t>(16 * 1024))

static const uint8_t bitswap[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


Hashes Detector::execute(const std::vector<uint8_t> &data) const {
    for (auto &rule : rules) {
        auto hashes = rule.execute(data);
        if (hashes.has_size()) {
            return hashes;
        }
    }

    return Hashes();
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


Hashes Detector::Rule::compute_values(Operation operation, const std::vector<uint8_t> &data, uint64_t start, uint64_t length) const {
    Hashes hashes;
    
    hashes.types = Hashes::TYPE_CRC | Hashes::TYPE_MD5 | Hashes::TYPE_SHA1;
    Hashes::Update hu(&hashes);
    
    if (operation == OP_NONE) {
        hu.update(data.data() + start, length);
    }
    else {
        auto processed_data = std::vector<uint8_t>(length);

        switch (operation) {
        case OP_NONE:
            // can't happen
            break;
        
        case OP_BITSWAP:
            for (size_t i = 0; i < length; i++) {
                processed_data[i] = bitswap[data[start + i]];
            }
            break;
                
        case OP_BYTESWAP:
            for (size_t i = 0; i < length; i += 2) {
                processed_data[i] = data[start + i + 1];
                processed_data[i + 1] = data[start + i];
            }
            break;

        case OP_WORDSWAP:
            for (size_t i = 0; i < length; i += 4) {
                processed_data[i] = data[start + i + 3];
                processed_data[i + 1] = data[start + i + 2];
                processed_data[i + 2] = data[start + i + 1];
                processed_data[i + 3] = data[start + i];
            }
        }
        
        hu.update(processed_data.data(), length);
    }
    
    hu.end();
    
    hashes.size = length;
    return hashes;
}



Hashes Detector::Rule::execute(const std::vector<uint8_t> &data) const {
    auto start = start_offset;
    if (start < 0) {
        start += static_cast<int64_t>(data.size());
    }
    auto end = end_offset;
    if (end == DETECTOR_OFFSET_EOF) {
        end = static_cast<int64_t>(data.size());
    }
    else if (end < 0) {
        end += static_cast<int64_t>(data.size());
    }
    
    if (start < 0 || static_cast<uint64_t>(start) > data.size() || end < 0 || static_cast<uint64_t>(end) > data.size() || start > end || static_cast<uint64_t>(end - start) % operation_unit_size(operation) != 0) {
        return Hashes();
    }

    for (auto &test : tests) {
        if (!test.execute(data)) {
            return Hashes();
        }
    }

    return compute_values(operation, data, static_cast<uint64_t>(start), static_cast<uint64_t>(end - start));
}


bool Detector::Test::execute(const std::vector<uint8_t> &data) const {
    auto match = false;
    
    switch (type) {
        case TEST_DATA:
        case TEST_OR:
        case TEST_AND:
        case TEST_XOR: {
            auto off = offset;
            
            if (off < 0) {
                off += data.size();
            }
            
            if (off < 0 || static_cast<uint64_t>(off) + length < static_cast<uint64_t>(off) || static_cast<uint64_t>(off) + length > data.size()) {
                return false;
            }
            
            if (mask.empty()) {
                match = (memcmp(data.data() + off, value.data(), length) == 0);
            }
            else {
                match = bit_cmp(data.data() + off);
            }
            break;
        }

        case TEST_FILE_EQ:
        case TEST_FILE_LE:
        case TEST_FILE_GR:
            if (offset == DETECTOR_SIZE_POWER_OF_2) {
                match = false;
                for (auto i = 0; i < 64; i++) {
                    if (data.size() == (static_cast<uint64_t>(1) << i)) {
                        match = true;
                        break;
                    }
                }
            }
            else {
                int64_t cmp = offset - static_cast<int64_t>(data.size());
                
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
        return result ? true : false;
    }
    else {
        return result ? false : true;
    }
}


bool Detector::compute_hashes(ZipSourcePtr source, File *file, const std::unordered_map<size_t, DetectorPtr> &detectors) {
    std::unordered_map<size_t, DetectorPtr> needs_update;
    
    if (file->get_size(0) > MAX_DETECTOR_FILE_SIZE) {
        return false;
    }
    
    for (auto pair : detectors) {
        if (file->detector_hashes.find(pair.first) == file->detector_hashes.end()) {
            needs_update[pair.first] = pair.second;
        }
    }
    
    if (needs_update.empty()) {
        return false;
    }
    
    auto data = std::vector<uint8_t>(file->get_size(0));
    source->read(data.data(), file->get_size(0));
    
    for (auto pair : needs_update) {
        file->detector_hashes[pair.first] = pair.second->execute(data);
    }
    
    return true;
}
