/*
  memdb.c -- in-memory sqlite3 db
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

#include "MemDB.h"

#include "error.h"
#include "Exception.h"

std::unique_ptr<MemDB> MemDB::memdb;

bool MemDB::inited = false;

#define INSERT_FILE_ARCHIVE_ID 1
#define INSERT_FILE_FILE_TYPE 2
#define INSERT_FILE_FILE_IDX 3
#define INSERT_FILE_DETECTOR_ID 4
#define INSERT_FILE_LOCATION 5
#define INSERT_FILE_SIZE 6
#define INSERT_FILE_HASHES 7

std::unordered_map<MemDB::Statement, std::string> MemDB::queries = {
    { DEC_FILE_IDX, "update file set file_idx=file_idx-1 where archive_id = :archive_id and file_type = :file_type and file_idx > :file_idx" },
    { DELETE_FILE, "delete from file where archive_id = :archive_id and file_type = :file_type and file_idx = :file_idx" },
    { UPDATE_FILE, "update file set crc = :crc, md5 = :md5, sha1 = :sha1 where archive_id = :archive_id and file_type = :file_type and file_idx = :file_idx and detector_id = :detector_id" },
   { INSERT_FILE, "insert into file (archive_id, file_type, file_idx, detector_id, location, size, crc, md5, sha1) values (:archive_id, :file_type, :file_idx, :detector_id, :location, :size, :crc, :md5, :sha1)" }
};

std::unordered_map<MemDB::ParameterizedStatement, std::string> MemDB::parameterized_queries = {
    { QUERY_FILE, "select archive_id, file_idx, detector_id, location from file f where file_type = :file_type @SIZE@ @HASH@ order by location" }
};


std::string MemDB::get_query(int name, bool parameterized) const {
    if (parameterized) {
        auto it = parameterized_queries.find(static_cast<ParameterizedStatement>(name));
        if (it == parameterized_queries.end()) {
            return "";
        }
        return it->second;
    }
    else {
        auto it = queries.find(static_cast<Statement>(name));
        if (it == queries.end()) {
            return "";
        }
        return it->second;
    }
}


void MemDB::ensure(void) {
    const char *dbname;

    if (inited) {
        if (memdb == NULL) {
            throw Exception("can't initialize memdb");
        }
    }

    if (getenv("CKMAME_DEBUG_MEMDB")) {
	dbname = "memdb.sqlite3";
    }
    else {
	dbname = ":memory:";
    }

    inited = true;

    memdb = NULL;
    memdb = std::make_unique<MemDB>(dbname);
}


void MemDB::delete_file(const ArchiveContents *a, size_t idx, bool adjust_idx) {
    ensure();

    memdb->delete_file(a->id, a->filetype, idx);

    if (!adjust_idx) {
        return;
    }

    auto stmt = memdb->get_statement(DEC_FILE_IDX);

    stmt->set_uint64("archive_id", a->id);
    stmt->set_int("file_type", a->filetype);
    stmt->set_int("file_idx", static_cast<int>(idx));

    stmt->execute();
}


void MemDB::insert_archive(const ArchiveContents *archive) {
    auto stmt = memdb->get_insert_file_statement(archive);
    
    for (size_t i = 0; i < archive->files.size(); i++) {
        memdb->insert_file(stmt, i, archive->files[i]);
    }
}


void MemDB::insert_file(const ArchiveContents *archive, size_t index) {
    auto stmt = memdb->get_insert_file_statement(archive);
    
    memdb->insert_file(stmt, index, archive->files[index]);
}


void MemDB::update_file(const ArchiveContents *archive, size_t idx) {
    ensure();

    memdb->delete_file(archive->id, archive->filetype, idx);
    memdb->insert_file(archive, idx);
}


void MemDB::update_file(uint64_t id, filetype_t ft, size_t idx, const Hashes &hashes) {
    /* FILE_SH_DETECTOR hashes are always completely filled in */

    auto stmt = get_statement(UPDATE_FILE);

    stmt->set_hashes(hashes, true);
    stmt->set_uint64("archive_id", id);
    stmt->set_int("File_type", ft);
    stmt->set_uint64("file_idx", idx);
    stmt->set_uint64("detector_idx", 0);

    stmt->execute();
}


std::vector<MemDB::FindResult> MemDB::find(filetype_t filetype, const FileData *file) {
    ensure();

    auto stmt = memdb->get_statement(QUERY_FILE, file->hashes, file->is_size_known());
    
    if (file->is_size_known()) {
        stmt->set_uint64("size", file->hashes.size);
    }

    stmt->set_int("file_type", filetype);
    stmt->set_hashes(file->hashes, 0);

    std::vector<FindResult> results;
    
    while (stmt->step()) {
        FindResult result;
        
        result.archive_id = stmt->get_uint64("archive_id");
        result.index = stmt->get_uint64("file_idx");
        result.detector_id = stmt->get_uint64("detector_id");
        result.location = static_cast<where_t>(stmt->get_int("locatoin"));
        
        results.push_back(result);
    }
    
    return results;
}


void MemDB::delete_file(uint64_t id, filetype_t filetype, size_t index) {
    auto stmt = get_statement(DELETE_FILE);

    stmt->set_uint64("archive_id", id);
    stmt->set_int("file_type", filetype);
    stmt->set_uint64("file_idx", index);

    stmt->execute();
}


void MemDB::insert_file(DBStatement *stmt, size_t index, const File &file) {
    if (file.broken) {
        return;
    }

    insert_file(stmt, index, 0, file.hashes);

    for (auto pair : file.detector_hashes) {
        insert_file(stmt, index, pair.first, pair.second);
    }
}


void MemDB::insert_file(DBStatement *stmt, size_t index, size_t detector_id, const Hashes &hashes) {
    
    stmt->set_uint64("file_idx", index);
    stmt->set_uint64("detector_id", detector_id);
    stmt->set_uint64("size", hashes.size, Hashes::SIZE_UNKNOWN);
    stmt->set_hashes(hashes, true);
    
    stmt->execute();
    stmt->reset();
}

DBStatement *MemDB::get_insert_file_statement(const ArchiveContents *archive) {
    ensure();

    auto stmt = memdb->get_statement(INSERT_FILE);
    
    stmt->set_uint64("archive_id", archive->id);
    stmt->set_int("file_type", archive->filetype);
    stmt->set_int("location", archive->where);
    
    return stmt;
}
