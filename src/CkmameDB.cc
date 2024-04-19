/*
 dbh_cache.c -- files in dirs sqlite3 data base
 Copyright (C) 2014-2015 Dieter Baron and Thomas Klausner

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

#include "CkmameDB.h"
#include "globals.h"

#include <utility>

#include "Detector.h"
#include "Dir.h"
#include "Exception.h"
#include "fix.h"
#include "Progress.h"
#include "util.h"

const std::string CkmameDB::db_name = ".ckmame.db";

const DB::DBFormat CkmameDB::format = {
    0x02,
    4,
    "create table archive (\n\
    archive_id integer primary key autoincrement,\n\
    name text not null,\n\
    mtime integer not null,\n\
    size integer not null,\n\
    file_type integer not null\n\
);\n\
create index archive_name on archive (name);\n\
create table detector (\n\
    detector_id integer primary key autoincrement,\n\
    name text not null,\n\
    version text not null\n\
);\n\
create index detector_name_version on detector (name, version);\n\
create table file (\n\
    archive_id integer not null,\n\
    file_idx integer,\n\
    name text not null,\n\
    mtime integer not null,\n\
    status integer not null,\n\
    size integer not null,\n\
    crc integer,\n\
    md5 binary,\n\
    sha1 binary,\n\
    detector_id integer not null default 0\n\
);\n\
create index file_archive_id on file (archive_id);\n\
create index file_idx on file (file_idx);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n",
    {
        { MigrationVersions(2, 3), "\
    create table detector (\n\
    detector_id integer primary key autoincrement,\n\
    name text not null,\n\
    version text not null\n\
    );\n\
    create index detector_name_version on detector (name, version);\n\
    alter table file add column detector_id integer not null default 0;\n\
    create index file_size on file (size);\n\
    create index file_crc on file (crc);\n\
    create index file_md5 on file (md5);\n\
    create index file_sha1 on file (sha1);\n\
    " },
        { MigrationVersions(3, 4), "\
alter table archive add column file_type integer not null default " + std::to_string(TYPE_ROM) + ";\n\
update archive set file_type=" + std::to_string(TYPE_DISK) + " where exists(select * from file f where f.archive_id = archive.archive_id and f.crc is null);\
    " }
    }
};


std::unordered_map<CkmameDB::Statement, std::string> CkmameDB::queries = {
    { DELETE_ARCHIVE, "delete from archive where archive_id = :archive_id" },
    { DELETE_FILE, "delete from file where archive_id = :archive_id" },
    { INSERT_ARCHIVE, "insert into archive (name, file_type, mtime, size) values (:name, :file_type, :mtime, :size)" },
    { INSERT_ARCHIVE_ID, "insert into archive (name, archive_id, file_type, mtime, size) values (:name, :archive_id, :file_type, :mtime, :size)" },
    { INSERT_DETECTOR, "insert into detector (detector_id, name, version) values (:detector_id, :name, :version)" },
    { INSERT_FILE, "insert into file (archive_id, file_idx, detector_id, name, mtime, status, size, crc, md5, sha1) values (:archive_id, :file_idx, :detector_id, :name, :mtime, :status, :size, :crc, :md5, :sha1)" },
    { LIST_ARCHIVES, "select name, file_type from archive" },
    { LIST_DETECTORS, "select detector_id, name, version from detector" },
    { QUERY_ARCHIVE_ID, "select archive_id from archive where name = :name and file_type = :file_type" },
    { QUERY_ARCHIVE_LAST_CHANGE, "select mtime, size from archive where archive_id = :archive_id" },
    { QUERY_FILE, "select file_idx, detector_id, name, mtime, status, size, crc, md5, sha1 from file where archive_id = :archive_id order by file_idx, detector_id" },
    { QUERY_HAS_ARCHIVES, "select archive_id from archive limit 1" },
    { UPDATE_FILE_HASHES, "update file set crc = :crc, md5 = :md5, sha1 = :sha1 where archive_id = :archive_id and file_idx = :file_idx and detector_id = 0" }
};

std::unordered_map<CkmameDB::ParameterizedStatement, std::string> CkmameDB::parameterized_queries = {
    { QUERY_FIND_FILE, "select archive.name as archive_name, file_idx, detector_id from archive, file f where archive.file_type = :file_type and archive.archive_id = f.archive_id and (f.detector_id = 0 or f.detector_id = :detector_id) @SIZE@ @HASH@ order by archive_name" }
};

CkmameDB::CkmameDB(const std::string& directory, where_t where) : CkmameDB(make_db_file_name(directory, db_name, configuration.extra_directory_use_central_cache_directory(directory)), directory, where) {
}

CkmameDB::CkmameDB(const std::string &dbname, std::string directory_, where_t where) : DB(format, dbname, DBH_CREATE | DBH_WRITE), directory(std::move(directory_)), where(where) {
    auto stmt = get_statement(LIST_DETECTORS);

    while (stmt->step()) {
        detector_ids.add(DetectorDescriptor(stmt->get_string("name"), stmt->get_string("version")), stmt->get_uint64("detector_id"));
    }
}


std::string CkmameDB::get_query(int name, bool parameterized) const {
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


void CkmameDB::delete_archive(int id) {
    delete_files(id);

    auto stmt = get_statement(DELETE_ARCHIVE);

    stmt->set_int("archive_id", id);
    stmt->execute();
}


void CkmameDB::delete_archive(const std::string &name, filetype_t filetype) {
    auto id = get_archive_id(name, filetype);

    delete_archive(id);
}


void CkmameDB::delete_files(int id) {
    auto stmt = get_statement(DELETE_FILE);

    stmt->set_int("archive_id", id);
    stmt->execute();
}


int CkmameDB::get_archive_id(const std::string &name, filetype_t filetype) {
    auto archive_name = name_in_db(name);
    if (archive_name.empty()) {
    return 0;
    }

    auto stmt = get_statement(QUERY_ARCHIVE_ID);

    stmt->set_string("name", archive_name);
    stmt->set_int("file_type", filetype);

    if (!stmt->step()) {
        return 0;
    }

    return stmt->get_int("archive_id");
}


void CkmameDB::get_last_change(int id, time_t *mtime, off_t *size) {
    auto stmt = get_statement(QUERY_ARCHIVE_LAST_CHANGE);

    stmt->set_int("archive_id", id);

    if (!stmt->step()) {
        throw Exception("archive not found in ckmamedb");
    }

    *mtime = stmt->get_int64("mtime");
    *size = stmt->get_int64("size");
}


bool CkmameDB::is_empty() {
    auto stmt = get_statement(QUERY_HAS_ARCHIVES);

    if (stmt->step()) {
        return false;
    }
    return true;
}


std::vector<ArchiveLocation> CkmameDB::list_archives() {
    auto stmt = get_statement(LIST_ARCHIVES);
    std::vector<ArchiveLocation> archives;

    while (stmt->step()) {
        archives.emplace_back(stmt->get_string("name"), static_cast<filetype_t>(stmt->get_int("file_type")));
    }

    return archives;
}


int CkmameDB::read_files(int archive_id, std::vector<File> *files) {
    if (archive_id == 0) {
        return 0;
    }

    auto stmt = get_statement(QUERY_FILE);

    stmt->set_int("archive_id", archive_id);

    files->clear();

    while (stmt->step()) {
        auto detector_id = stmt->get_uint64("detector_id");

        if (detector_id == 0) {
            // There is exactly one entry per file_idx with detector_id 0, which is retrieved in order.
            File file;

            file.name = stmt->get_string("name");
            file.mtime = stmt->get_int64("mtime");
            file.broken = stmt->get_int("status");
            file.hashes = stmt->get_hashes();
            file.hashes.size = stmt->get_uint64("size", Hashes::SIZE_UNKNOWN);

            files->push_back(file);
        }
        else {
            auto file_id = stmt->get_uint64("file_idx");
            auto global_detector_id = get_global_detector_id(detector_id);

            Hashes hashes = stmt->get_hashes();
            hashes.size = stmt->get_uint64("size", Hashes::SIZE_UNKNOWN);

            (*files)[file_id].detector_hashes[global_detector_id] = hashes;
        }
    }

    return archive_id;
}


void CkmameDB::seterr() {
    output.set_error_database(this);
}


void CkmameDB::write_archive(ArchiveContents *archive) {
    auto id = archive->cache_id;

    if (id == 0) {
        id = get_archive_id(archive->name, archive->filetype);
    }

    if (id != 0) {
        delete_archive(id);
    }

    auto name = name_in_db(archive->name);
    if (name.empty()) {
        throw Exception();
    }

    id = write_archive_header(id, name, archive->filetype, archive->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? 0 : archive->mtime, archive->size);

    auto stmt = get_statement(INSERT_FILE);

    for (size_t i = 0; i < archive->files.size(); i++) {
        const auto &file = archive->files[i];

        stmt->set_int("archive_id", id);
        stmt->set_int("file_idx", static_cast<int>(i));
        stmt->set_uint64("detector_id", 0);
        stmt->set_string("name", file.name);
        stmt->set_int64("mtime", file.mtime);
        stmt->set_int("status", file.broken ? 1 : 0);
        stmt->set_uint64("size", file.hashes.size);
        stmt->set_hashes(file.hashes, true);

        stmt->execute();
        stmt->reset();

        for (auto &pair : file.detector_hashes) {
            auto detector_id = get_detector_id(pair.first);

            stmt->set_int("archive_id", id);
            stmt->set_int("file_idx", static_cast<int>(i));
            stmt->set_uint64("detector_id", detector_id);
            stmt->set_string("name", "", true);
            stmt->set_int64("mtime", 0);
            stmt->set_int("status", 0);
            stmt->set_uint64("size", pair.second.size);
            stmt->set_hashes(pair.second, true);

            stmt->execute();
            stmt->reset();
        }
    }

    archive->cache_id = id;
}


std::string CkmameDB::name_in_db(const std::string &name) {
    if (name == directory) {
        return ".";
    }

    auto offset = directory.length() + 1;
    auto length = name.length() - offset;

    return name.substr(offset, length);
}


int CkmameDB::write_archive_header(int id, const std::string &name, filetype_t filetype, time_t mtime, uint64_t size) {
    auto stmt = get_statement(INSERT_ARCHIVE_ID);

    stmt->set_string("name", name);
    stmt->set_int("file_type", filetype);
    stmt->set_int64("mtime", mtime);
    stmt->set_uint64("size", size);

    if (id > 0) {
        stmt->set_int("archive_id", id);
    }

    stmt->execute();

    if (id <= 0) {
        id = static_cast<int>(stmt->get_rowid());
    }

    return id;
}


size_t CkmameDB::get_detector_id(size_t global_id) {
    auto detector = Detector::get_descriptor(global_id);

    if (detector == nullptr) {
        throw Exception();
    }

    auto known = detector_ids.known(*detector);
    auto id = detector_ids.get_id(*detector);

    if (!known) {
        auto stmt = get_statement(INSERT_DETECTOR);

        stmt->set_uint64("detector_id", id);
        stmt->set_string("name", detector->name);
        stmt->set_string("version", detector->version);

        stmt->execute();
    }

    return id;
}


size_t CkmameDB::get_global_detector_id(size_t id) {
    auto detector = detector_ids.get_descriptor(id);
    if (detector == nullptr) {
        throw Exception();
    }
    return Detector::get_id(*detector);
}


void CkmameDB::find_file(filetype_t filetype, size_t detector_id, const FileData& file, std::vector<FindResult>& results) {
    auto stmt = get_statement(QUERY_FIND_FILE, file.hashes, file.is_size_known());

    if (file.is_size_known()) {
        stmt->set_uint64("size", file.hashes.size);
    }

    stmt->set_int("file_type", filetype);
    stmt->set_hashes(file.hashes, false);
    stmt->set_uint64("detector_id", detector_id);

    while (stmt->step()) {
        auto archive_name = stmt->get_string("archive_name");
        if (archive_name == ".") {
            archive_name = directory;
        }
        else {
            archive_name = directory + "/" + archive_name;
        }
        // TODO: no trailing slash if archive_name is empty
        results.emplace_back(archive_name, stmt->get_uint64("file_idx"), stmt->get_uint64("detector_id"), where);
    }
}

void CkmameDB::refresh() {
    if (configuration.roms_zipped) {
        refresh_zipped();
    }
    else {
        refresh_unzipped();
    }

    // TODO: remove archives from DB that are no longer in file system.
}

void CkmameDB::refresh_unzipped() {
    try {
        Dir dir(directory, false);
        std::filesystem::path filepath;

        while (!(filepath = dir.next()).empty()) {
            if (name_type(filepath) == NAME_IGNORE) {
                continue;
            }
            if (std::filesystem::is_directory(filepath)) {
                Progress::set_message("currently scanning '" + filepath.string() + "'");
                auto a = Archive::open(filepath, TYPE_ROM, where, 0);
                a->close();
            }
        }

        Progress::set_message("currently scanning '" + directory + "'");
        auto a = Archive::open_toplevel(directory, TYPE_ROM, where, 0);
        if (a) {
            a->close();
        }
    }
    catch (...) {}
}

void CkmameDB::refresh_zipped() {
    try {
        Dir dir(directory, true);
        std::filesystem::path filepath;

        while (!(filepath = dir.next()).empty()) {
            name_type_t nt;

            switch ((nt = name_type(filepath))) {
            case NAME_IMAGES:
            case NAME_ZIP: {
                Progress::set_message("currently scanning '" + filepath.string() + "'");
                auto a = Archive::open(filepath, nt == NAME_ZIP ? TYPE_ROM : TYPE_DISK, where, 0);
                if (a) {
                    a->close();
                }
                break;
            }

            case NAME_IGNORE:
            case NAME_UNKNOWN:
                // TODO: loose: add unknown files?
                break;
            }
        }

        Progress::set_message("currently scanning '" + directory + "'");
        auto a = Archive::open_toplevel(directory, TYPE_DISK, where, 0);
        if (a) {
            a->close();
        }
    }
    catch (...) {
    }
}

bool CkmameDB::compute_detector_hashes(const std::unordered_map<size_t, DetectorPtr>& detectors) {
    if (detectors.empty()) {
        return false;
    }

    auto got_new_hashes = false;

    for (const auto& location: list_archives()) {
        auto contents = ArchiveContents::by_name(location.filetype, directory + "/" + location.name);
        if (contents == nullptr || contents->has_all_detector_hashes(detectors)) {
            continue;
        }
        auto archive = Archive::open(contents);
        if (archive->compute_detector_hashes(detectors)) {
            got_new_hashes = true;
        }
    }

    return got_new_hashes;
}

void CkmameDB::update_file_hashes(int archive_id, size_t file_id, const Hashes& hashes) {
    auto stmt = get_statement(UPDATE_FILE_HASHES);

    stmt->set_int("archive_id", archive_id);
    stmt->set_int("file_idx", static_cast<int>(file_id));
    stmt->set_hashes(hashes, true);

    stmt->execute();
}

void CkmameDB::insert_file_detector_hashes(int archive_id, size_t file_id, size_t detector_id, const Hashes& hashes) {
    auto local_detector_id = get_detector_id(detector_id);

    auto stmt = get_statement(INSERT_FILE);

    stmt->set_int("archive_id", archive_id);
    stmt->set_int("file_idx", static_cast<int>(file_id));
    stmt->set_uint64("detector_id", local_detector_id);
    stmt->set_string("name", "", true);
    stmt->set_int64("mtime", 0);
    stmt->set_int("status", 0);
    stmt->set_uint64("size", hashes.size);
    stmt->set_hashes(hashes, true);

    stmt->execute();
}
