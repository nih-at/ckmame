/*
  romdb.c -- mame.db sqlite3 data base
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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

#include "RomDB.h"

#include "Exception.h"
#include "globals.h"

std::unique_ptr<RomDB> db;
std::unique_ptr<RomDB> old_db;

const DB::DBFormat RomDB::format = {
    0x0,
    4,
    "\
create table dat (\n\
    dat_idx integer primary key,\n\
    name text,\n\
    description text,\n\
    author text,\n\
    version text\n\
);\n\
\n\
create table game (\n\
    game_id integer primary key autoincrement,\n\
    name text not null,\n\
    parent text,\n\
    description text,\n\
    dat_idx integer not null\n\
);\n\
create index game_name on game (name);\n\
\n\
create table file (\n\
    game_id integer,\n\
    file_type integer,\n\
    file_idx integer,\n\
    name text not null,\n\
    merge text,\n\
    status integer not null,\n\
    location integer not null,\n\
    size integer,\n\
    crc integer,\n\
    md5 binary,\n\
    sha1 binary,\n\
    sha256 binary,\n\
    missing integer not null,\n\
    primary key (game_id, file_type, file_idx)\n\
);\n\
create index file_game_type on file (game_id, file_type);\n\
\n\
create table rule (\n\
    rule_idx integer primary key,\n\
    start_offset integer,\n\
    end_offset integer,    \n\
    operation integer\n\
);\n\
\n\
create table test (\n\
    rule_idx integer,\n\
    test_idx integer,\n\
    type integer not null,\n\
    offset integer,\n\
    size integer,\n\
    mask binary,\n\
    value binary,\n\
    result integer not null,\n\
    primary key (rule_idx, test_idx)\n\
);\n",
    {
        {MigrationVersions(3, 4), "\
alter table file add column sha256 binary;\n\
alter table file add column missing integer not null default 0;\n\
"}
    }

};

const std::string RomDB::init2_sql = "\
create index file_name on file (name);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n";


std::unordered_map<int, std::string> RomDB::queries = {
    {  DELETE_FILE, "delete from file where game_id = :game_id" },
    {  DELETE_GAME, "delete from game where game_id = :game_id" },
    {  INSERT_DAT_DETECTOR, "insert into dat (dat_idx, name, author, version) values (-1, :name, :author, :version)" },
    {  INSERT_DAT, "insert into dat (dat_idx, name, description, version) values (:dat_idx, :name, :description, :version)" },
    {  INSERT_FILE, "insert into file (game_id, file_type, file_idx, name, merge, status, location, size, crc, md5, sha1, sha256, missing) values (:game_id, :file_type, :file_idx, :name, :merge, :status, :location, :size, :crc, :md5, :sha1, :sha256, :missing)" },
    {  INSERT_GAME, "insert into game (name, description, dat_idx, parent) values (:name, :description, :dat_idx, :parent)" },
    {  INSERT_RULE, "insert into rule (rule_idx, start_offset, end_offset, operation) values (:rule_idx, :start_offset, :end_offset, :operation)" },
    {  INSERT_TEST, "insert into test (rule_idx, test_idx, type, offset, size, mask, value, result) values (:rule_idx, :test_idx, :type, :offset, :size, :mask, :value, :result)" },
    {  QUERY_CLONES, "select name from game where parent = :parent" },
    {  QUERY_DAT_DETECTOR, "select name, author, version from dat where dat_idx = -1" },
    {  QUERY_DAT, "select name, description, version from dat where dat_idx >= 0 order by dat_idx" },
    {  QUERY_FILE_FBN, "select g.name, f.file_idx from game g, file f where f.game_id = g.game_id and f.file_type = :file_type and f.name = :name" },
    {  QUERY_FILE, "select name, merge, status, location, size, crc, md5, sha1, sha256, missing from file where game_id = :game_id and file_type = :file_type order by file_idx" },
    {  QUERY_GAME_ID, "select game_id from game where name = :name" },
    {  QUERY_GAME, "select game_id, description, dat_idx, parent from game where name = :name" },
    {  QUERY_HAS_DISKS, "select file_idx from file where file_type = 1 limit 1" },
    {  QUERY_HASH_TYPE_CRC, "select name from file where file_type = :file_type and crc not null limit 1" },
    {  QUERY_HASH_TYPE_MD5, "select name from file where file_type = :file_type and md5 not null limit 1" },
    {  QUERY_HASH_TYPE_SHA1, "select name from file where file_type = :file_type and sha1 not null limit 1" },
    {  QUERY_HASH_TYPE_SHA256, "select name from file where file_type = :file_type and sha256 not null limit 1" },
    {  QUERY_LIST_DISK, "select distinct name from file where file_type = 1 order by name" },
    {  QUERY_LIST_GAME, "select name from game order by name" },
    {  QUERY_PARENT_BY_NAME, "select parent from game where name = :name" },
    {  QUERY_PARENT, "select parent from game where game_id = :game_id" },
    {  QUERY_RULE, "select rule_idx, start_offset, end_offset, operation from rule order by rule_idx" },
    {  QUERY_STATS_FILES, "select file_type, count(name) amount, sum(size) total_size from file group by file_type order by file_type" },
    {  QUERY_STATS_GAMES, "select count(name) as amount from game" },
    {  QUERY_TEST, "select type, offset, size, mask, value, result from test where rule_idx = :rule_idx order by test_idx" },
    {  UPDATE_FILE, "update file set location = :location where game_id = :game_id and file_type = :file_type and file_idx = :file_idx" },
    {  UPDATE_PARENT, "update game set parent = :parent where game_id = :game_id" }
};

std::unordered_map<int, std::string> RomDB::parameterized_queries = {
   {  QUERY_FILE_FBH, "select g.name as game_name, g.dat_idx, f.file_idx, f.name, f.size, f.crc, f.md5, f.sha1, f.sha256 from game g, file f where f.game_id = g.game_id and f.file_type = :file_type and f.status <> :status @HASH@" },

};

const RomDB::Statement RomDB::query_hash_type[] = { QUERY_HASH_TYPE_CRC, QUERY_HASH_TYPE_MD5, QUERY_HASH_TYPE_SHA1, QUERY_HASH_TYPE_SHA256 };


void RomDB::init2() {
    if (sqlite3_exec(db, init2_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw Exception("can't initialize DB");
    }
}


std::vector<std::string> RomDB::get_clones(const std::string &game_name) {
    auto stmt = get_statement(RomDB::QUERY_CLONES);
    
    stmt->set_string("parent", game_name);
    
    auto clones = std::vector<std::string>();
    
    while (stmt->step()) {
        clones.push_back(stmt->get_string("name"));
    }
    
    return clones;
}


Stats RomDB::get_stats() {
    Stats stats;

    auto stmt = get_statement(RomDB::QUERY_STATS_GAMES);
    
    // This statement always returns one row.
    (void)stmt->step();
    
    stats.games_total = stmt->get_uint64("amount");
    
    stmt = get_statement(RomDB::QUERY_STATS_FILES);
    
    int ft = -1;
    for (int i = 0; i < TYPE_MAX; i++) {
        if (ft < i) {
            if (stmt->step()) {
                ft = stmt->get_int("file_type");
            }
            else {
                ft = TYPE_MAX;
            }
        }
        
        if (ft != i) {
            continue;
        }
        
        stats.files[i].files_total = stmt->get_uint64("amount");
        stats.files[i].bytes_total = stmt->get_uint64("total_size");
    }
    
    return stats;
}


bool RomDB::has_disks() {
    auto stmt = get_statement(QUERY_HAS_DISKS);

    return stmt->step();
}


int RomDB::hashtypes(filetype_t type) {
    if (hashtypes_[type] == -1) {
        read_hashtypes(type);
    }
    
    return hashtypes_[type];
}


RomDB::RomDB(const std::string &name, int mode) : DB(format, name, mode) {
    for (size_t i = 0; i < TYPE_MAX; i++) {
	hashtypes_[i] = -1;
    }
    
    auto stmt = get_statement(QUERY_DAT_DETECTOR);

    while (stmt->step()) {
        auto detector_id = Detector::get_id(DetectorDescriptor(stmt->get_string("name"), stmt->get_string("version")));
        detectors[detector_id] = read_detector();
    }
}


void RomDB::read_hashtypes(filetype_t ft) {
    int type;

    hashtypes_[ft] = 0;

    for (type = 0; (1 << type) <= Hashes::TYPE_MAX; type++) {
        auto stmt = get_statement(query_hash_type[type]);

        stmt->set_int("file_type", ft);
        if (stmt->step()) {
	    hashtypes_[ft] |= (1 << type);
        }
    }
}


std::string RomDB::get_query(int name, bool parameterized) const {
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


std::vector<DatEntry> RomDB::read_dat() {
    auto stmt = get_statement(QUERY_DAT);

    std::vector<DatEntry> dat;

    while (stmt->step()) {
        DatEntry de;

        de.name = stmt->get_string("name");
        de.description = stmt->get_string("description");
        de.version = stmt->get_string("version");
        
        dat.push_back(de);
    }

    return dat;
}


DetectorPtr RomDB::get_detector(size_t id) {
    auto it = detectors.find(id);
    
    if (it == detectors.end()) {
        return nullptr;
    }
    return it->second;
}

DetectorPtr RomDB::read_detector() {
    auto stmt = get_statement(QUERY_DAT_DETECTOR);

    if (!stmt->step()) {
        return nullptr;
    }

    auto detector = std::make_shared<Detector>();

    detector->name = stmt->get_string("name");
    detector->author = stmt->get_string("author");
    detector->version = stmt->get_string("version");

    if (!read_rules(detector.get())) {
        return nullptr;
    }

    return detector;
}


bool RomDB::read_rules(Detector *detector) {
    auto stmt_rule = get_statement(QUERY_RULE);
    auto stmt_test = get_statement(QUERY_TEST);

    while (stmt_rule->step()) {
        Detector::Rule rule;
        
        auto idx = stmt_rule->get_int("rule_idx");
        rule.start_offset = stmt_rule->get_int64("start_offset", 0);
        rule.end_offset = stmt_rule->get_int64("end_offset", DETECTOR_OFFSET_EOF);
        rule.operation = static_cast<Detector::Operation>(stmt_rule->get_int("operation", Detector::OP_NONE));

        stmt_test->set_int("rule_idx", idx);

        while (stmt_test->step()) {
            Detector::Test test;

            // type, offset, size, mask, value, result
            test.type = static_cast<Detector::TestType>(stmt_test->get_int("type"));
            test.offset = stmt_test->get_int64("offset");
            test.result =stmt_test->get_int64("result");
            
            switch (test.type) {
                case Detector::TEST_DATA:
                case Detector::TEST_OR:
                case Detector::TEST_AND:
                case Detector::TEST_XOR:
                    test.mask = stmt_test->get_blob("mask");
                    test.value = stmt_test->get_blob("value");
                    if (!test.mask.empty() && test.mask.size() != test.value.size()) {
                        return false;
                    }
                    test.length = test.value.size();
                    break;
                    
                case Detector::TEST_FILE_EQ:
                case Detector::TEST_FILE_LE:
                case Detector::TEST_FILE_GR:
                    test.length = static_cast<uint64_t>(stmt_test->get_int64("size"));
                    break;
            }
            
            rule.tests.push_back(test);
        }
        stmt_test->reset();
        
        detector->rules.push_back(rule);
    }

    return true;
}


std::vector<RomLocation> RomDB::read_file_by_hash(filetype_t ft, const Hashes &hashes) {
    auto stmt = get_statement(QUERY_FILE_FBH, hashes, false);
    
    stmt->set_int("file_type", ft);
    stmt->set_int("status", Rom::NO_DUMP);
    stmt->set_hashes(hashes, false);

    std::vector<RomLocation> result;

    while (stmt->step()) {
        auto rom = Rom();
        rom.name = stmt->get_string("name");
        rom.hashes = stmt->get_hashes();
        rom.hashes.size = stmt->get_uint64("size", Hashes::SIZE_UNKNOWN);

        result.emplace_back(stmt->get_string("game_name"), get_detector_id_for_dat(stmt->get_uint64("dat_idx")), static_cast<size_t>(stmt->get_int("file_idx")), rom);
    }

    return result;
}


static std::string chd_extension = ".chd";

GamePtr RomDB::read_game(const std::string &name) {
    auto stmt = get_statement(QUERY_GAME);

    stmt->set_string("name", name);

    if (!stmt->step()) {
        return nullptr;
    }
    
    auto game = std::make_shared<Game>();
    game->id = stmt->get_uint64("game_id");
    game->name = name;
    game->description = stmt->get_string("description");
    game->dat_no = static_cast<unsigned int>(stmt->get_int("dat_idx"));
    game->cloneof[0] = stmt->get_string("parent");

    if (!game->cloneof[0].empty()) {
        stmt = get_statement(QUERY_PARENT_BY_NAME);
        stmt->set_string("name", game->cloneof[0]);

        if (stmt->step()) {
            game->cloneof[1] = stmt->get_string("parent");
        }
    }
    
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        read_files(game.get(), static_cast<filetype_t>(ft));
    }

    return game;
}


void RomDB::read_files(Game *game, filetype_t ft) {
    auto stmt = get_statement(QUERY_FILE);

    stmt->set_uint64("game_id", game->id);
    stmt->set_int("file_type", ft);

    while (stmt->step()) {
        Rom rom;

        rom.name = stmt->get_string("name");
        rom.merge = stmt->get_string("merge");
        rom.status = static_cast<Rom::Status>(stmt->get_int("status"));
        rom.where = static_cast<where_t>(stmt->get_int("location"));
        rom.missing = stmt->get_bool("missing");
        rom.hashes = stmt->get_hashes();
        rom.hashes.size = stmt->get_uint64("size", Hashes::SIZE_UNKNOWN);

        game->files[ft].push_back(rom);
    }
}


std::vector<std::string> RomDB::read_list(enum dbh_list type) {
    static const std::unordered_map<enum dbh_list, Statement> query_list = {
        { DBH_KEY_LIST_DISK, QUERY_LIST_DISK },
        { DBH_KEY_LIST_GAME, QUERY_LIST_GAME }
    };
    
    auto it = query_list.find(type);
    if (it == query_list.end()) {
        throw Exception("unknown type %d", type);
    }
    
    auto stmt = get_statement(it->second);

    std::vector<std::string> result;

    while (stmt->step()) {
        result.push_back(stmt->get_string("name"));
    }

    return result;
}


void RomDB::write_dat(const std::vector<DatEntry> &dats) {
    auto stmt = get_statement(INSERT_DAT);

    for (size_t i = 0; i < dats.size(); i++) {
        auto &dat = dats[i];

        stmt->set_int("dat_idx", static_cast<int>(i));
        stmt->set_string("name", dat.name);
        stmt->set_string("description", dat.description);
        stmt->set_string("version", dat.version);
        stmt->execute();
        stmt->reset();
    }
}


void RomDB::write_detector(const Detector &detector) {
    auto stmt = get_statement(INSERT_DAT_DETECTOR);

    stmt->set_string("name", detector.name);
    stmt->set_string("author", detector.author);
    stmt->set_string("version", detector.version);
    stmt->execute();

    write_rules(detector);
}


void RomDB::write_rules(const Detector &detector) {
    auto stmt_rule = get_statement(INSERT_RULE);
    auto stmt_test = get_statement(INSERT_TEST);

    for (size_t i = 0; i < detector.rules.size(); i++) {
        auto &rule = detector.rules[i];

        stmt_rule->set_int("rule_idx", static_cast<int>(i));
        stmt_rule->set_int64("start_offset", rule.start_offset, 0);
        stmt_rule->set_int64("end_offset", rule.end_offset, DETECTOR_OFFSET_EOF);
        stmt_rule->set_int("operation", rule.operation, Detector::OP_NONE);
        
        stmt_rule->execute();
        stmt_rule->reset();

        stmt_test->set_int("rule_idx", static_cast<int>(i));
        
        for (size_t j = 0; j < rule.tests.size(); j++) {
            auto &test = rule.tests[j];

            //        {  INSERT_TEST, "insert into test (rule_idx, test_idx, type, offset, size, mask, value, result) values (:rule_idx, :test_idx, :type, :offset, :size, :mask, :value, :result)" },

            stmt_test->set_int("test_idx", static_cast<int>(j));
            stmt_test->set_int("type", test.type);
            stmt_test->set_int64("offset", test.offset);
            stmt_test->set_int("result", test.result);
            
            switch (test.type) {
                case Detector::TEST_DATA:
                case Detector::TEST_OR:
                case Detector::TEST_AND:
                case Detector::TEST_XOR:
                    stmt_test->set_null("size");
                    stmt_test->set_blob("mask", test.mask);
                    stmt_test->set_blob("value", test.value);
                    break;

                case Detector::TEST_FILE_EQ:
                case Detector::TEST_FILE_LE:
                case Detector::TEST_FILE_GR:
                    stmt_test->set_int64("size", static_cast<int64_t>(test.length));
                    stmt_test->set_null("mask");
                    stmt_test->set_null("value");
                    break;
            }

            stmt_test->execute();
            stmt_test->reset();
        }
    }
}


void RomDB::delete_game(const std::string &name) {
    auto stmt = get_statement(QUERY_GAME_ID);

    stmt->set_string("name", name);

    if (!stmt->step()) {
        return;
    }

    auto id = stmt->get_int("game_id");

    std::string error;

    try {
        stmt = get_statement(DELETE_GAME);
        stmt->set_int("game_id", id);
        stmt->execute();
    }
    catch (Exception &e) {
        error = e.what();
    }

    stmt = get_statement(DELETE_FILE);
    stmt->set_int("game_id", id);
    stmt->execute();

    if (!error.empty()) {
        throw Exception(error);
    }
}


void RomDB::update_file_location(Game *game) {
    auto stmt = get_statement(UPDATE_FILE);

    //     {  UPDATE_FILE, "update file set location = :location where game_id = :game_id and file_type = :file_type and file_idx = :file_idx" },

    for (int ft = 0; ft < TYPE_MAX; ft++) {
        for (size_t i = 0; i < game->files[ft].size(); i++) {
            stmt->set_uint64("game_id", game->id);
            stmt->set_int("file_type", ft);

            auto &rom = game->files[ft][i];
            if (rom.where == FILE_INGAME) {
                continue;
            }

            stmt->set_int("location", rom.where);
            stmt->set_int("file_idx", static_cast<int>(i));
            stmt->execute();
            stmt->reset();
        }
    }
}


void RomDB::update_game_parent(const Game *game) {
    auto stmt = get_statement(UPDATE_PARENT);

    stmt->set_string("parent", game->cloneof[0]);
    stmt->set_uint64("game_id", game->id);
    stmt->execute();
}


void RomDB::write_game(Game *game) {
    delete_game(game);

    auto stmt = get_statement(INSERT_GAME);

    stmt->set_string("name", game->name);
    stmt->set_string("description", game->description);
    stmt->set_int("dat_idx", static_cast<int>(game->dat_no));
    stmt->set_string("parent", game->cloneof[0]);
    
    stmt->execute();

    game->id = static_cast<uint64_t>(stmt->get_rowid());

    try {
        for (size_t ft = 0; ft < TYPE_MAX; ft++) {
            write_files(game, static_cast<filetype_t>(ft));
        }
    }
    catch (Exception &e) {
        delete_game(game);
        throw e;
    }
}


void RomDB::write_files(Game *game, filetype_t ft) {
    auto stmt = get_statement(INSERT_FILE);

    for (size_t i = 0; i < game->files[ft].size(); i++) {
        auto &rom = game->files[ft][i];

        stmt->set_uint64("game_id", game->id);
        stmt->set_int("file_type", ft);
        stmt->set_int("file_idx", static_cast<int>(i));
        stmt->set_string("name", rom.name);
        stmt->set_string("merge", rom.merge);
        stmt->set_int("status", rom.status);
        stmt->set_int("location", rom.where);
        stmt->set_bool("missing", rom.missing);
        stmt->set_uint64("size", rom.hashes.size, Hashes::SIZE_UNKNOWN);
        stmt->set_hashes(rom.hashes, true);
        
        stmt->execute();
        stmt->reset();
    }
}

int RomDB::export_db(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out) {
    DatEntry de;

    if (out == nullptr) {
	/* TODO: split into original dat files */
	return 0;
    }

    auto db_dat = read_dat();

    /* TODO: export detector */

    de.merge(dat, (db_dat.size() == 1 ? &db_dat[0] : nullptr));
    out->header(&de);

    std::vector<std::string> list;
    
    try {
        list = read_list(DBH_KEY_LIST_GAME);
    }
    catch (Exception &e) {
        output.error("db error reading game list: %s", e.what());
	return -1;
    }

    for (const auto &name : list) {
        GamePtr game = read_game(name);
        if (!game) {
	    /* TODO: error */
	    continue;
	}
        
        if (exclude.find(game->name) == exclude.end()) {
	    out->game(game);
        }
    }

    return 0;
}


size_t RomDB::get_detector_id_for_dat(size_t dat_no) const {
    if (!has_detector()) {
        return 0;
    }
    
    // TODO: Fix once we support multiple detectors in one DB.
    return detectors.begin()->first;
}

bool RomDB::game_exists(const std::string &name) {
    auto stmt = get_statement(QUERY_GAME_ID);

    stmt->set_string("name", name);

    return stmt->step();
}
