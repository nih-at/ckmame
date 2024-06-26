/*
  Hashes.cc -- utility functions for hash handling
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

#include "Hashes.h"

#include <cinttypes>
#include <utility>

#include "Exception.h"
#include "util.h"


uint64_t Hashes::SIZE_UNKNOWN = UINT64_MAX;

std::unordered_map<std::string, int> Hashes::name_to_type = {
    { "crc", TYPE_CRC },
    { "md5", TYPE_MD5},
    { "sha1", TYPE_SHA1 }
};

std::unordered_map<int, std::string> Hashes::type_to_name = {
    { TYPE_CRC, "crc" },
    { TYPE_MD5, "md5" },
    { TYPE_SHA1, "sha1" }
};

const Hashes Hashes::zero(0, TYPE_CRC | TYPE_MD5 | TYPE_SHA1,
                          0,
                          { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e },
                          { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09 });

Hashes::Hashes(size_t size, int types, uint32_t crc, std::vector<uint8_t> md5, std::vector<uint8_t> sha1)
    : size(size), crc(crc), md5(std::move(md5)), sha1(std::move(sha1)), types(types) {

}

int Hashes::types_from_string(const std::string &s) {
    int types = 0;

    auto rest = s;
    do {
	std::string type;
	auto comma = rest.find(',');

	if (comma == std::string::npos) {
	    type = rest;
	    rest = "";
	} else {
	    type = rest.substr(0, comma-1);
	    rest = rest.substr(comma+1);
	}

	auto it = name_to_type.find(type);
	if (it == name_to_type.end()) {
	    return 0;
	}
	types |= it->second;
    } while (!rest.empty());

    return types;
}


void Hashes::add_types(int new_types) {
    for (auto type = 1; type <= TYPE_ALL; type <<= 1) {
        if ((new_types & type) && (types & type) == 0) {
            switch (type) {
                case TYPE_CRC:
                    break;
                    
                case TYPE_MD5:
                    md5.resize(SIZE_MD5);
                    break;
                    
                case TYPE_SHA1:
                    sha1.resize(SIZE_SHA1);
                    break;
            }
        }
    }
    
    types |= new_types;
}


bool Hashes::are_crc_complement(const Hashes &other) const {
    if (!has_type(TYPE_CRC) || !other.has_type(TYPE_CRC)) {
        return false;
    }
    return ((crc ^ other.crc) & 0xffffffff) == 0xffffffff;
}

Hashes::Compare Hashes::compare_with_size(const Hashes &other) const {
    if (has_size() && other.has_size() && size != other.size) {
        return MISMATCH;
    }
    return compare(other);
}


Hashes::Compare Hashes::compare(const Hashes &other) const {
    if (types == 0 || other.types == 0) {
        return MATCH;
    }

    auto common_types = (types & other.types);

    if (common_types == 0) {
	return NOCOMMON;
    }

    if ((common_types & TYPE_CRC) != 0) {
        if (crc != other.crc) {
            return MISMATCH;
        }
    }

    if ((common_types & TYPE_MD5) != 0) {
        if (md5 != other.md5) {
            return MISMATCH;
        }
    }

    if ((common_types & TYPE_SHA1) != 0) {
        if (sha1 != other.sha1) {
	    return MISMATCH;
        }
    }

    return MATCH;
}


bool Hashes::operator==(const Hashes &other) const {
    if (types != other.types) {
	return false;
    }

    if (types & TYPE_CRC) {
	if (crc != other.crc) {
	    return false;
	}
    }

    if (types & TYPE_MD5) {
        if (md5 != other.md5) {
	    return false;
	}
    }

    if (types & TYPE_SHA1) {
        if (sha1 != other.sha1) {
	    return false;
	}
    }

    return true;
}


void Hashes::merge(const Hashes &other) {
    auto new_types = other.types & ~types;
    
    if (new_types & TYPE_CRC) {
        crc = other.crc;
    }
    
    if (new_types & TYPE_MD5) {
        md5 = other.md5;
    }
    
    if (new_types & TYPE_SHA1) {
        sha1 = other.sha1;
    }
    
    types |= new_types;
}

bool
Hashes::has_type(int requested_type) const {
    if ((types & requested_type) == requested_type) {
	return true;
    }
    return false;
}

size_t Hashes::hash_size(int type) {
    switch (type) {
        case TYPE_CRC:
            return SIZE_CRC;

        case TYPE_MD5:
            return SIZE_MD5;

        case TYPE_SHA1:
            return SIZE_SHA1;

        default:
            return 0;
    }
}


void Hashes::set_crc(uint32_t data, bool ignore_zero) {
    if (data == 0 && ignore_zero) {
        return;
    }
    
    crc = data;
    types |= TYPE_CRC;
}


void Hashes::set_md5(const std::vector<uint8_t> &data, bool ignore_zero) {
    set(TYPE_MD5, md5, data, ignore_zero);
}


void Hashes::set_md5(const uint8_t *data, bool ignore_zero) {
    set(TYPE_MD5, md5, std::vector<uint8_t>(data, data + SIZE_MD5), ignore_zero);
}


void Hashes::set_sha1(const std::vector<uint8_t> &data, bool ignore_zero) {
    set(TYPE_SHA1, sha1, data, ignore_zero);

}


void Hashes::set_sha1(const uint8_t *data, bool ignore_zero) {
    set(TYPE_SHA1, sha1, std::vector<uint8_t>(data, data + SIZE_SHA1), ignore_zero);
}


void Hashes::set(int type, std::vector<uint8_t> &hash, const std::vector<uint8_t> &data, bool ignore_zero) {
    if (data.size() != hash_size(type)) {
        throw Exception("invalid length for hash");
    }
    
    if (ignore_zero) {
        auto all_zero = true;
        for (auto b : data) {
            if (b != 0) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            return;
        }
    }

    switch (type) {
        case TYPE_MD5:
            md5 = data;
            break;
            
        case TYPE_SHA1:
            sha1 = data;
            break;
            
        default:
            throw Exception("invalid hash type");
    }
    types |= type;
}


bool Hashes::is_zero(int type) const {
    if ((types & type) == 0) {
        return true;
    }
    
    const std::vector<uint8_t> *data;

    switch (type) {
        case TYPE_CRC:
            return crc == 0;
            
        case TYPE_MD5:
            data = &md5;
            break;
            
        case TYPE_SHA1:
            data = &sha1;
            break;
            
        default:
            throw Exception("invalid hash type");
    }
    
    for (auto x : *data) {
        if (x != 0) {
            return false;
        }
    }
    
    return true;
}

void Hashes::set_hashes(const Hashes &other) {
    types = other.types;
    crc = other.crc;
    md5 = other.md5;
    sha1 = other.sha1;
}


std::string Hashes::to_string(int type) const {
    if (!has_type(type)) {
        return "";
    }

    switch (type) {
        case Hashes::TYPE_CRC: {
            char str[10];
            snprintf(str, sizeof(str), "%.8" PRIx32, crc);
            return str;
        }

        case Hashes::TYPE_MD5:
            return bin2hex(md5);

        case Hashes::TYPE_SHA1:
            return bin2hex(sha1);

        default:
            return "";
    }

}


int Hashes::set_from_string(const std::string& s) {
    auto str = s;

    /* remove leading & trailing whitespace */
    auto data_start = str.find_first_not_of(" \t\n\r");
    if (data_start != std::string::npos) {
        str = str.substr(data_start);
    }
    auto data_end = str.find_last_not_of(" \t\n\r");
    if (data_end != std::string::npos) {
        str = str.substr(0, data_end + 1);
    }

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str = str.substr(2);
    }

    size_t length = str.length();

    if (length % 2 != 0 || str.find_first_not_of("0123456789ABCDEFabcdef") != std::string::npos) {
        return -1;
    }

    int type = 0;

    switch (length / 2) {
        case Hashes::SIZE_CRC:
            type = Hashes::TYPE_CRC;
            crc = static_cast<uint32_t>(std::stoul(str, nullptr, 16));
            break;

        case Hashes::SIZE_MD5:
            type = Hashes::TYPE_MD5;
            // TODO: check length
            md5 = hex2bin(str);
            break;

        case Hashes::SIZE_SHA1:
            type = Hashes::TYPE_SHA1;
            // TODO: check length
            sha1 = hex2bin(str);
            break;

        default:
            return -1;
    }

    types |= type;
    return type;
}


std::string Hashes::type_name(int type) {
    auto it = type_to_name.find(type);

    if (it == type_to_name.end()) {
        return "unknown type " + std::to_string(type);
    }

    return it->second;
}
