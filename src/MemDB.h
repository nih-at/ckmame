#ifndef HAD_MEMDB_H
#define HAD_MEMDB_H

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
    enum Statement {
        DEC_FILE_IDX,
        DELETE_FILE,
        INSERT_FILE,
        UPDATE_FILE
    };
    enum ParameterizedStatement {
        QUERY_FILE
    };

    class FindResult {
    public:
        uint64_t archive_id;
        uint64_t index;
        size_t detector_id;
        where_t location;
    };
    
    explicit MemDB(const std::string &name) : DB(format, name, DBH_NEW) { }
    ~MemDB() override = default;
    
    static const DBFormat format;

    static void ensure();

    void delete_file(const ArchiveContents *a, size_t idx, bool adjust_idx);
    void insert_archive(const ArchiveContents *archive);
    void insert_file(const ArchiveContents *archive, size_t index);
    void update_file(const ArchiveContents *archive, size_t idx);

    std::vector<FindResult> find(filetype_t filetype, const FileData *file);

protected:
    std::string get_query(int name, bool parameterized) const override;

private:
    DBStatement *get_statement(Statement name) { return get_statement_internal(name); }
    DBStatement *get_statement(ParameterizedStatement name, const Hashes &hashes, bool have_size) { return get_statement_internal(name, hashes, have_size); }
    
    static std::unordered_map<Statement, std::string> queries;
    static std::unordered_map<ParameterizedStatement, std::string> parameterized_queries;

    static bool inited;
};

extern std::unique_ptr<MemDB> memdb;

#endif // HAD_MEMDB_H
