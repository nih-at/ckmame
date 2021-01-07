#ifndef HAD_HASHES_H
#define HAD_HASHES_H

/*
  hashes.h -- hash related functions
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

#include <cinttypes>
#include <string>
#include <unordered_map>

#include <string.h>

#include "intstr.h"

class HashesContexts;

class Hashes {
public:
    class Update {
    public:
        Update(Hashes *hashes);
        ~Update();
        
        void update(const void *data, size_t length);
        void end();
        
    private:
        HashesContexts *contexts;
        Hashes *hashes;
    };
    
    enum {
        TYPE_CRC = 1,
        TYPE_MD5 = 2,
        TYPE_SHA1 = 4,
        TYPE_MAX = 4,
        TYPE_ALL = 7
    };
    enum {
        SIZE_CRC = 4,
        SIZE_MD5 = 16,
        SIZE_SHA1 = 20,
        MAX_SIZE = 20
    };
    enum Compare {
        NOCOMMON = -1,
        MATCH,
        MISMATCH
    };
    
    int types;
    uint32_t crc;
    uint8_t md5[SIZE_MD5];
    uint8_t sha1[SIZE_SHA1];
    
    Hashes() : types(0), crc(0) { }
    
    bool are_crc_complement(const Hashes &other) const;
    bool has_type(int type) const;
    bool has_all_types(const Hashes &other) const { return has_all_types(other.types); }
    bool has_all_types(int requested_types) const { return (types & requested_types) == requested_types; }
    Compare compare(const Hashes &other) const;
    bool operator==(const Hashes &other) const;
    bool verify(int type, const void *data) const;
    std::string to_string(int type) const;
    bool empty() const { return types == 0; }

    void set(int type, const void *data);
    void set_crc(uint32_t crc_) { types |= TYPE_CRC; crc = crc_; }
    int set_from_string(const std::string s);

    static int types_from_string(const std::string &s);
    static std::string type_name(int type);
    static size_t hash_size(int type);

private:
    static std::unordered_map<std::string, int> name_to_type;
    static std::unordered_map<int, std::string> type_to_name;
    
    const void *hash_data(int type) const;
};

#endif /* hashes.h */
