#ifndef HAD_ARCHIVE_LOCATION_H
#define HAD_ARCHIVE_LOCATION_H

/*
 ArchiveLocation.h -- name and file type of archive.
 Copyright (C) 2021-2024 Dieter Baron and Thomas Klausner
 
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

#include <string>
#include <utility>

#include "types.h"

class Archive;

class ArchiveLocation { // TODO: better name
public:
    explicit ArchiveLocation(const Archive *a);
    ArchiveLocation(std::string name_, filetype_t filetype_) : name(std::move(name_)), filetype(filetype_) { }

    std::string name;
    filetype_t filetype;
    
    bool operator<(const ArchiveLocation &other) const;
    bool operator==(const ArchiveLocation &other) const { return name == other.name && filetype == other.filetype; }
};


namespace std {
template <>
struct hash<ArchiveLocation> {
    std::size_t operator()(const ArchiveLocation &k) const {
        return std::hash<int>()(k.filetype) ^ std::hash<std::string>()(k.name);
    }
};
}


#endif // HAD_ARCHIVE_LOCATION_H
