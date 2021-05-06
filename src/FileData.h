#ifndef HAD_FILEDATA_H
#define HAD_FILEDATA_H

/*
  FileData.h -- information about one file
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include "Hashes.h"
#include "types.h"

class FileData {
public:
    std::string name;
    std::string filename_extension;
    Hashes hashes;
    uint64_t size_detector;
    Hashes hashes_detector;
    time_t mtime;
    status_t status;
    where_t where;
    
    FileData() : size_detector(SIZE_UNKNOWN_OLD), mtime(0), status(STATUS_OK), where(FILE_INGAME) { }

    uint64_t get_size(bool detector) const { return detector ? size_detector : hashes.size; }
    const Hashes &get_hashes(bool detector) const { return detector ? hashes_detector : hashes; }
    
    std::string filename() const { return name + filename_extension; }
    bool is_size_known(bool detector = false) const { return get_size(detector) != SIZE_UNKNOWN_OLD; }
    
    bool compare_name(const FileData &other) const;
    bool compare_name_size_hashes(const FileData &other) const;
    bool compare_size_hashes(const FileData &other) const;
    Hashes::Compare compare_hashes(const FileData &other) const;
    bool size_hashes_are_set(bool detector) const;
    
    bool operator<(const FileData &other) const { return name < other.name; }

private:
    bool compare_size_hashes_one(const FileData &other, bool detector) const;
};

#endif // HAD_FILEDATA_H
