#ifndef _HAD_MEMDB_H
#define _HAD_MEMDB_H

/*
  memdb.h -- in-memory sqlite3 db
  Copyright (C) 2007-2020 Dieter Baron and Thomas Klausner

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

#include <memory>
#include <optional>
#include <vector>

#include "Archive.h"
#include "DB.h"


class MemDB: public DB {
public:
    class FindResult {
    public:
        uint64_t game_id;
        uint64_t index;
        int sh;
        where_t location;
    };
    
    MemDB(const char *name) : DB(name, DBH_FMT_MEM | DBH_NEW) { }

    static bool delete_file(const ArchiveContents *a, size_t idx, bool adjust_idx);
    static bool insert_archive(const ArchiveContents *archive);
    static bool insert_file(const ArchiveContents *archive, size_t index);
    static bool update_file(const ArchiveContents *archive, size_t idx);

    static std::optional<std::vector<FindResult>> find(filetype_t filetype, const FileData *file);

private:
    static std::unique_ptr<MemDB> memdb;
    static bool inited;

    static bool ensure();

    bool delete_file(uint64_t id, filetype_t filetype, size_t index);
    bool update_file(uint64_t id, filetype_t filetype, size_t index, const Hashes *hashes);
    sqlite3_stmt *get_insert_file_statement(const ArchiveContents *archive);
    bool insert_file(sqlite3_stmt *stmt, size_t index, const File &file);
    bool insert_file(sqlite3_stmt *stmt, size_t index, size_t detector_id, const Hashes &hashes);
};

#endif /* memdb.h */
