#ifndef HAD_DBH_CACHE_H
#define HAD_DBH_CACHE_H
/*
 dbh_cache.h -- files in dirs sqlite3 database
 Copyright (C) 2014-2021 Dieter Baron and Thomas Klausner

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
#include <string>
#include <utility>
#include <vector>

#include "ArchiveLocation.h"
#include "DB.h"
#include "Detector.h"
#include "DetectorCollection.h"
#include "File.h"
#include "FileLocation.h"

class ArchiveContents;
class CkmameDB;

typedef std::shared_ptr<CkmameDB> CkmameDBPtr;

class CkmameDB : public DB {
public:
  class FindResult {
    public:
      FindResult(std::string name, size_t index, size_t detector_id, where_t where): name(std::move(name)), index(index), detector_id(detector_id), where(where) {}

      std::string name;
      size_t index;
      size_t detector_id;
      where_t where;
  };

    enum Statement {
        DELETE_ARCHIVE,
        DELETE_FILE,
        INSERT_ARCHIVE,
        INSERT_ARCHIVE_ID,
        INSERT_DETECTOR,
        INSERT_FILE,
        LIST_ARCHIVES,
        LIST_DETECTORS,
        QUERY_ARCHIVE_ID,
        QUERY_ARCHIVE_LAST_CHANGE,
        QUERY_FILE,
        QUERY_HAS_ARCHIVES
    };
    enum ParameterizedStatement {
        QUERY_FIND_FILE
    };

    explicit CkmameDB(const std::string& directory, where_t where);
    CkmameDB(const std::string& dbname, std::string directory, where_t where); // used in dbrestore
    ~CkmameDB() override = default;

    static const DBFormat format;
    static const std::string db_name;

    void delete_archive(const std::string &name, filetype_t filetype);
    void delete_archive(int id);
    int get_archive_id(const std::string &name, filetype_t filetype);
    void get_last_change(int id, time_t *mtime, off_t *size);
    bool is_empty();
    std::vector<ArchiveLocation> list_archives();
    int read_files(int archive_id, std::vector<File> *files);
    void write_archive(ArchiveContents *archive);

    void find_file(filetype_t filetype, size_t detector_id, const FileData& file, std::vector<FindResult> &results);
    bool compute_detector_hashes(const std::unordered_map<size_t, DetectorPtr>& detectors);
    void refresh();
    
    void seterr();
    
protected:
    [[nodiscard]] std::string get_query(int name, bool parameterized) const override;
    
private:
    static std::unordered_map<Statement, std::string> queries;
    static std::unordered_map<ParameterizedStatement, std::string> parameterized_queries;

    std::string directory;
    where_t where;
    DetectorCollection detector_ids;
    
    DBStatement *get_statement(Statement name) { return get_statement_internal(name); }
    DBStatement *get_statement(ParameterizedStatement name, const Hashes &hashes, bool have_size) { return get_statement_internal(name, hashes, have_size); }

    std::string name_in_db(const std::string &name);
    void delete_files(int id);
    int write_archive_header(int id, const std::string &name, filetype_t filetype, time_t mtime, uint64_t size);
    
    size_t get_detector_id(size_t global_id);
    size_t get_global_detector_id(size_t id);

    void refresh_unzipped();
    void refresh_zipped();
};

#endif // HAD_DBH_CACHE_H
