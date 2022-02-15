/*
DatDB.cc -- cache database for dat files
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

#include "DatDb.h"

const std::string DatDB::db_name = ".mkmamedb.db";

const DB::DBFormat DatDB::format = {
    0x02,
    4,
    "create table file (\n\
file_id integer primary key autoincrement,\n\
file_name text not null,\n\
mtime integer not null,\n\
size integer not null\n\
);\n\
create index file_name on file (file_name);\n\
create table dat (\n\
file_id integer not null,\n\
entry_name text,\n\
name text,\n\
version text\n\
);\n\
create index dat_name on dat (name);\n\
",
    {}
};

std::unordered_map<DatDB::Statement, std::string> DatDB::queries = {
    { DELETE_DATS, "delete from dat where file_id = :file_id" },
    { DELETE_FILE, "delete from file where file_id = :file_id"},
    { INSERT_DAT, "insert into dat (file_id, entry_name, name, version) values (:file_id, :entry_name, :name, :version)" },
    { INSERT_FILE, "insert into file (file_name, mtime, size) values (:file_name, :mtime, :size)" },
    { LIST_DATS, "select distinct name from dat"},
    { LIST_FILES, "select file_name from file" },
    { QUERY_DAT, "select file_name, entry_name, version from file f, dat d where f.file_id = d.file_id and name = :name"},
    { QUERY_FILE_ID, "select file_id from file where file_name = :file_name" },
    { QUERY_FILE_LAST_CHANGE, "select file_id, size, mtime from file where file_name = :file_name" },
    { QUERY_HAS_FILES, "select file_id from file limit 1" }
};


DatDB::DatDB(const std::string& directory) : DB(format, directory + "/" + db_name, DBH_CREATE | DBH_WRITE) {

}


std::string DatDB::get_query(int name, bool parameterized) const {
    if (parameterized) {
	return "";
    }
    else {
	auto it = queries.find(static_cast<Statement>(name));
	if (it == queries.end()) {
	    return "";
	}
	return it->second;
    }
}


std::vector<std::string> DatDB::list_dats() {
    auto stmt = get_statement(LIST_DATS);

    std::vector<std::string> dats;

    while (stmt->step()) {
	dats.emplace_back(stmt->get_string("name"));
    }

    return dats;
}


std::vector<std::string> DatDB::list_files() {
    auto stmt = get_statement(LIST_FILES);

    std::vector<std::string> files;

    while (stmt->step()) {
	files.emplace_back(stmt->get_string("file_name"));
    }

    return files;
}


std::optional<int64_t> DatDB::get_file_id(const std::string &file_name) {
    auto stmt = get_statement(QUERY_FILE_ID);

    stmt->set_string("file_name", file_name);

    if (!stmt->step()) {
	return {};
    }

    return stmt->get_int64("file_id");
}



bool DatDB::get_last_change(const std::string &file_name, time_t *mtime, size_t *size) {
    auto stmt = get_statement(QUERY_FILE_LAST_CHANGE);

    stmt->set_string("file_name", file_name);

    if (!stmt->step()) {
	return false;
    }

    *mtime = stmt->get_int64("mtime");
    *size = stmt->get_uint64("size");

    return true;
}


void DatDB::delete_file(const std::string &file_name) {
    auto id = get_file_id(file_name);
    if (id.has_value()) {
	auto stmt = get_statement(DELETE_FILE);
    	stmt->set_int64("file_id", id.value());
    	stmt->execute();

	stmt = get_statement(DELETE_DATS);
	stmt->set_int64("file_id", id.value());
	stmt->execute();
    }
}


void DatDB::insert_file(const std::string &file_name, time_t mtime, size_t size, const std::vector<DatEntry> &dats) {
    auto stmt = get_statement(INSERT_FILE);

    stmt->set_string("file_name", file_name);
    stmt->set_int64("mtime", mtime);
    stmt->set_uint64("size", size);
    stmt->execute();
    auto id = stmt->get_rowid();

    if (!dats.empty()) {
	stmt = get_statement(INSERT_DAT);

	for (const auto &dat : dats) {
	    stmt->set_int64("file_id", id);
	    stmt->set_string("entry_name", dat.entry_name);
	    stmt->set_string("name", dat.name);
	    stmt->set_string("version", dat.version);
	    stmt->execute();
	    stmt->reset();
	}
    }
}


std::vector<DatDB::DatInfo> DatDB::get_dats(const std::string &name) {
    auto stmt = get_statement(QUERY_DAT);

    stmt->set_string("name", name);

    std::vector<DatInfo> dats;

    while (stmt->step()) {
	dats.emplace_back(stmt->get_string("file_name"), stmt->get_string("entry_name"), name, stmt->get_string("version"));
    }

    return dats;
}


bool DatDB::is_empty() {
    auto stmt = get_statement(QUERY_HAS_FILES);

    if (stmt->step()) {
	return false;
    }
    return true;
}
