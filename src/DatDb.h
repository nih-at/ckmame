#ifndef HAD_DATDB_H
#define HAD_DATDB_H
/*
 DatDB.h -- cache database for dat files
 Copyright (C) 2021 Dieter Baron and Thomas Klausner

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

#include "DB.h"

#include <optional>

class DatDB;

typedef std::shared_ptr<DatDB> DatDBPtr;

class DatDB : public DB {
  public:
    enum Statement {
	DELETE_DATS,
	DELETE_FILE,
	INSERT_DAT,
	INSERT_FILE,
	LIST_DATS,
	LIST_FILES,
	QUERY_DAT,
	QUERY_FILE_ID,
	QUERY_FILE_LAST_CHANGE,
	QUERY_HAS_FILES
    };

    class DatEntry {
      public:
	DatEntry(std::string entry_name, std::string name, std::string version) : entry_name(std::move(entry_name)), name(std::move(name)), version(std::move(version)) { }

	const std::string entry_name;
	const std::string name;
	const std::string version;
    };

    class DatInfo {
      public:
	DatInfo() = default;

	DatInfo(std::string file_name, std::string entry_name, std::string name, std::string version) : file_name(std::move(file_name)), entry_name(std::move(entry_name)), name(std::move(name)), version(std::move(version)) { }

	std::string file_name;
	std::string entry_name;
	std::string name;
	std::string version;
    };

    explicit DatDB(const std::string& directory);

    static const std::string db_name;

    bool is_empty();
    std::vector<std::string> list_dats();
    std::vector<std::string> list_files();

    bool get_last_change(const std::string &file_name, time_t *mtime, size_t *size);
    std::optional<int64_t> get_file_id(const std::string &file_name);
    void delete_file(const std::string &file_name);
    void insert_file(const std::string &file_name, time_t mtime, size_t size, const std::vector<DatEntry> &dats);

    std::vector<DatInfo> get_dats(const std::string &name);

  protected:
    [[nodiscard]] std::string get_query(int name, bool parameterized) const override;

  private:
    static const DBFormat format;
    static std::unordered_map<Statement, std::string> queries;

    DBStatement *get_statement(Statement name) { return get_statement_internal(name); }
};


#endif // HAD_DATDB_H
