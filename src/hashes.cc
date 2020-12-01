/*
  hashes.c -- utility functions for hash handling
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

#include "hashes.h"

#include <string.h>

#include "util.h"

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

int Hashes::types_from_string(const std::string &s) {
    char b[16];
    
    int types = 0;
    auto p = s.c_str();
    const char *q;
    do {
        q = p + strcspn(p, ",");
        if (static_cast<size_t>(q - p) >= sizeof(b)) {
            return 0;
        }
        strncpy(b, p, static_cast<size_t>(q - p));
        b[q-p] = '\0';
        auto it = name_to_type.find(b);
        if (it == name_to_type.end()) {
            return 0;
        }
        types |= it->second;

        p = q + 1;
    } while (*q);

    return types;
}

bool Hashes::are_crc_complement(const Hashes &other) const {
    if (!has_type(TYPE_CRC) || !other.has_type(TYPE_CRC)) {
        return false;
    }
    return ((crc ^ other.crc) & 0xffffffff) == 0xffffffff;
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
        if (memcmp(md5, other.md5, SIZE_MD5) != 0) {
            return MISMATCH;
        }
    }

    if ((common_types & TYPE_SHA1) != 0) {
        if (memcmp(sha1, other.sha1, SIZE_SHA1) != 0) {
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
	if (memcmp(md5, other.md5, SIZE_MD5) != 0) {
	    return false;
	}
    }

    if (types & TYPE_SHA1) {
	if (memcmp(sha1, other.sha1, SIZE_SHA1) != 0) {
	    return false;
	}
    }

    return true;
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

const void *Hashes::hash_data(int type) const {
    switch (type) {
        case TYPE_CRC:
            return &crc;
            
        case TYPE_MD5:
            return md5;

        case TYPE_SHA1:
            return sha1;

        default:
            return NULL;
    }
}

void Hashes::set(int type, const void *data) {
    auto length = hash_size(type);

    if (length == 0) {
        return;
    }
    memcpy(const_cast<void *>(hash_data(type)), data, length);
    types |= type;
}


bool Hashes::verify(int type, const void *data) const {
    auto length = hash_size(type);
    
    if (length == 0) {
        return false;
    }

    return memcmp(data, hash_data(type), length) == 0;
}


std::string Hashes::to_string(int type) const {
    if (!has_type(type)) {
        return "";
    }

    switch (type) {
        case Hashes::TYPE_CRC: {
            char str[10];
            sprintf(str, "%.8" PRIx32, crc);
            return str;
        }

        case Hashes::TYPE_MD5:
            return bin2hex(md5, SIZE_MD5);
            break;
            
        case Hashes::TYPE_SHA1:
            return bin2hex(sha1, SIZE_SHA1);
            break;
            
        default:
            return "";
    }
    
}


int Hashes::set_from_string(const std::string s) {
    const char *str = s.c_str();
    
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }

    size_t length = strlen(str);

    if (length % 2 != 0 || strspn(str, "0123456789ABCDEFabcdef") != length) {
        return -1;
    }

    int type = 0;

    switch (length / 2) {
        case Hashes::SIZE_CRC:
            type = Hashes::TYPE_CRC;
            crc = (uint32_t)strtoul(str, NULL, 16);
            break;
            
        case Hashes::SIZE_MD5:
            type = Hashes::TYPE_MD5;
            hex2bin(md5, str, Hashes::SIZE_MD5);
            break;
            
        case Hashes::SIZE_SHA1:
            type = Hashes::TYPE_SHA1;
            hex2bin(sha1, str, Hashes::SIZE_SHA1);
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
        return "";
    }
    
    return it->second;
}
