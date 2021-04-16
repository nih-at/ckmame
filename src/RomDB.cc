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

#include "error.h"
#include "sq_util.h"

std::unique_ptr<RomDB> db;
std::unique_ptr<RomDB> old_db;

const dbh_stmt_t query_hash_type[] = {DBH_STMT_QUERY_HASH_TYPE_CRC, DBH_STMT_QUERY_HASH_TYPE_MD5, DBH_STMT_QUERY_HASH_TYPE_SHA1};

int RomDB::has_disks() {
    sqlite3_stmt *stmt = db.get_statement(DBH_STMT_QUERY_HAS_DISKS);
    if (stmt == NULL) {
	return -1;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	return 1;

    case SQLITE_DONE:
	return 0;

    default:
	return -1;
    }
}


int RomDB::hashtypes(filetype_t type) {
    if (hashtypes_[type] == -1) {
        read_hashtypes(type);
    }
    
    return hashtypes_[type];
}


RomDB::RomDB(const std::string &name, int mode) : db(name, mode) {
    for (size_t i = 0; i < TYPE_MAX; i++) {
	hashtypes_[i] = -1;
    }
}


void RomDB::read_hashtypes(filetype_t ft) {
    int type;
    sqlite3_stmt *stmt;

    hashtypes_[ft] = 0;

    for (type = 0; (1 << type) <= Hashes::TYPE_MAX; type++) {
        if ((stmt = db.get_statement(query_hash_type[type])) == NULL) {
            continue;
        }
        if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK) {
	    continue;
        }
        if (sqlite3_step(stmt) == SQLITE_ROW) {
	    hashtypes_[ft] |= (1 << type);
        }
    }
}


std::vector<DatEntry> RomDB::read_dat() {
    auto stmt = db.get_statement(DBH_STMT_QUERY_DAT);
    if (stmt == NULL) {
    /* TODO */
    return {};
    }

    std::vector<DatEntry> dat;

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        DatEntry de;

        de.name = sq3_get_string(stmt, 0);
        de.description = sq3_get_string(stmt, 1);
        de.version = sq3_get_string(stmt, 2);
        
        dat.push_back(de);
    }

    if (ret != SQLITE_DONE) {
    /* TODO */
        return {};
    }

    return dat;
}


DetectorPtr RomDB::read_detector() {
    auto stmt = db.get_statement(DBH_STMT_QUERY_DAT_DETECTOR);

    if (stmt == NULL) {
        return NULL;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
    return NULL;
    }

    auto detector = std::make_shared<Detector>();

    detector->name = sq3_get_string(stmt, 0);
    detector->author = sq3_get_string(stmt, 1);
    detector->version = sq3_get_string(stmt, 2);

    if (!read_rules(detector.get())) {
    return NULL;
    }

    return detector;
}


bool RomDB::read_rules(Detector *detector) {
    auto stmt_rule = db.get_statement(DBH_STMT_QUERY_RULE);
    auto stmt_test = db.get_statement(DBH_STMT_QUERY_TEST);

    if (stmt_rule == NULL || stmt_test == NULL) {
        return false;
    }

    int ret;
    while ((ret = sqlite3_step(stmt_rule)) == SQLITE_ROW) {
        Detector::Rule rule;

        auto idx = sqlite3_column_int(stmt_rule, 0);
        rule.start_offset = sq3_get_int64_default(stmt_rule, 1, 0);
    rule.end_offset = sq3_get_int64_default(stmt_rule, 2, DETECTOR_OFFSET_EOF);
        rule.operation = static_cast<Detector::Operation>(sq3_get_int_default(stmt_rule, 3, Detector::OP_NONE));

        if (sqlite3_bind_int(stmt_test, 1, idx) != SQLITE_OK) {
        return false;
        }

    while ((ret = sqlite3_step(stmt_test)) == SQLITE_ROW) {
            Detector::Test test;

            test.type = static_cast<Detector::TestType>(sqlite3_column_int(stmt_test, 0));
        test.offset = sqlite3_column_int64(stmt_test, 1);
        test.result = sqlite3_column_int64(stmt_test, 5);

        switch (test.type) {
                case Detector::TEST_DATA:
                case Detector::TEST_OR:
                case Detector::TEST_AND:
                case Detector::TEST_XOR:
                    test.mask = sq3_get_blob(stmt_test, 3);
                    test.value = sq3_get_blob(stmt_test, 4);
                    if (!test.mask.empty() && test.mask.size() != test.value.size()) {
                        return false;
                    }
                    test.length = test.value.size();
                    break;
                    
                case Detector::TEST_FILE_EQ:
                case Detector::TEST_FILE_LE:
                case Detector::TEST_FILE_GR:
                    test.length = static_cast<uint64_t>(sqlite3_column_int64(stmt_test, 2));
                    break;
            }
            
            rule.tests.push_back(test);
    }
        if (ret != SQLITE_DONE || sqlite3_reset(stmt_test) != SQLITE_OK) {
        return false;
        }
        
        detector->rules.push_back(rule);
    }

    return true;
}


std::vector<FileLocation> RomDB::read_file_by_hash(filetype_t ft, const Hashes *hash) {
    auto stmt = db.get_statement(DBH_STMT_QUERY_FILE_FBH, hash, 0);
    if (stmt == NULL) {
        return {};
    }

    if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK || sqlite3_bind_int(stmt, 2, STATUS_NODUMP) != SQLITE_OK || sq3_set_hashes(stmt, 3, hash, 0) != SQLITE_OK) {
        return {};
    }

    std::vector<FileLocation> result;

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        result.push_back(FileLocation(sq3_get_string(stmt, 0), static_cast<size_t>(sqlite3_column_int(stmt, 1))));
    }

    if (ret != SQLITE_DONE) {
        return {};
    }

    return result;
}


static std::string chd_extension = ".chd";

GamePtr RomDB::read_game(const std::string &name) {
    auto stmt = db.get_statement(DBH_STMT_QUERY_GAME);
    if (stmt == NULL) {
        return NULL;
    }

    if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
        return NULL;
    }

    auto game = std::make_shared<Game>();
    game->id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    game->name = name;
    game->description = sq3_get_string(stmt, 1);
    game->dat_no = static_cast<unsigned int>(sqlite3_column_int(stmt, 2));
    game->cloneof[0] = sq3_get_string(stmt, 3);

    if (!game->cloneof[0].empty()) {
        if ((stmt = db.get_statement(DBH_STMT_QUERY_PARENT_BY_NAME)) == NULL || sq3_set_string(stmt, 1, game->cloneof[0]) != SQLITE_OK) {
            return NULL;
        }
        
        int ret;
        if ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            game->cloneof[1] = sq3_get_string(stmt, 0);
        }
        
        if (ret != SQLITE_ROW && ret != SQLITE_DONE) {
            return NULL;
        }
    }
    
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        if (!read_files(game.get(), static_cast<filetype_t>(ft))) {
            // TODO: use error reporting function
            printf("can't read files for %s\n", name.c_str());
            return NULL;
        }
    }

    return game;
}


bool RomDB::read_files(Game *game, filetype_t ft) {
    auto stmt = db.get_statement(DBH_STMT_QUERY_FILE);
    if (stmt == NULL) {
        return false;
    }

    if (sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(game->id)) != SQLITE_OK || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK) {
        return false;
    }

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        File rom;

        rom.name = sq3_get_string(stmt, 0);
        rom.merge = sq3_get_string(stmt, 1);
        rom.status = static_cast<status_t>(sqlite3_column_int(stmt, 2));
        rom.where = static_cast<where_t>(sqlite3_column_int(stmt, 3));
        rom.size = sq3_get_uint64_default(stmt, 4, SIZE_UNKNOWN);
        sq3_get_hashes(&rom.hashes, stmt, 5);
        if (ft == TYPE_DISK) {
            rom.filename_extension = chd_extension;
        }
        
        game->files[ft].push_back(rom);
    }
    
    return ret == SQLITE_DONE;
}


std::vector<std::string> RomDB::read_list(enum dbh_list type) {
    static const std::unordered_map<enum dbh_list, dbh_stmt_t> query_list = {
        { DBH_KEY_LIST_DISK, DBH_STMT_QUERY_LIST_DISK },
        { DBH_KEY_LIST_GAME, DBH_STMT_QUERY_LIST_GAME }
    };
    
    auto it = query_list.find(type);
    if (it == query_list.end()) {
        return {};
    }
    
    auto stmt = db.get_statement(it->second);
    if (stmt == NULL) {
        return {};
    }

    std::vector<std::string> result;

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        result.push_back(sq3_get_string(stmt, 0));
    }

    if (ret != SQLITE_DONE) {
    return {};
    }

    return result;
}


bool RomDB::write_dat(const std::vector<DatEntry> &dats) {
    auto stmt = db.get_statement(DBH_STMT_INSERT_DAT);
    if (stmt == NULL) {
        return false;
    }

    for (size_t i = 0; i < dats.size(); i++) {
        if (sqlite3_bind_int(stmt, 1, static_cast<int>(i)) != SQLITE_OK || sq3_set_string(stmt, 2, dats[i].name) != SQLITE_OK || sq3_set_string(stmt, 3, dats[i].description) != SQLITE_OK || sq3_set_string(stmt, 4, dats[i].version) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK)
        return false;
    }

    return true;
}


bool RomDB::write_detector(const Detector *detector) {
    auto stmt = db.get_statement(DBH_STMT_INSERT_DAT_DETECTOR);
    if (stmt == NULL) {
        return false;
    }

    if (sq3_set_string(stmt, 1, detector->name) != SQLITE_OK || sq3_set_string(stmt, 2, detector->author) != SQLITE_OK || sq3_set_string(stmt, 3, detector->version) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return write_rules(detector);
}


bool RomDB::write_rules(const Detector *detector) {
    auto stmt_rule = db.get_statement(DBH_STMT_INSERT_RULE);
    auto stmt_test = db.get_statement(DBH_STMT_INSERT_TEST);
    if (stmt_rule == NULL || stmt_test == NULL) {
        return false;
    }

    for (size_t i = 0; i < detector->rules.size(); i++) {
        auto &rule = detector->rules[i];

        if (sqlite3_bind_int(stmt_rule, 1, static_cast<int>(i)) != SQLITE_OK || sqlite3_bind_int(stmt_test, 1, static_cast<int>(i)) != SQLITE_OK || sq3_set_int64_default(stmt_rule, 2, rule.start_offset, 0) != SQLITE_OK || sq3_set_int64_default(stmt_rule, 3, rule.end_offset, DETECTOR_OFFSET_EOF) != SQLITE_OK || sq3_set_int_default(stmt_rule, 4, rule.operation, Detector::OP_NONE) != SQLITE_OK || sqlite3_step(stmt_rule) != SQLITE_DONE || sqlite3_reset(stmt_rule) != SQLITE_OK) {
            return false;
        }
        
        for (size_t j = 0; j < rule.tests.size(); j++) {
            auto &test = rule.tests[j];

            if (sqlite3_bind_int(stmt_test, 2, static_cast<int>(j)) != SQLITE_OK || (sqlite3_bind_int(stmt_test, 3, test.type) != SQLITE_OK) || sqlite3_bind_int64(stmt_test, 4, test.offset) != SQLITE_OK || (sqlite3_bind_int(stmt_test, 8, test.result) != SQLITE_OK)) {
                return false;
            }

            switch (test.type) {
                case Detector::TEST_DATA:
                case Detector::TEST_OR:
                case Detector::TEST_AND:
                case Detector::TEST_XOR:
                    if (sqlite3_bind_null(stmt_test, 5) != SQLITE_OK || sq3_set_blob(stmt_test, 6, test.mask) != SQLITE_OK || sq3_set_blob(stmt_test, 7, test.value) != SQLITE_OK) {
                        return false;
                    }
                    break;

                case Detector::TEST_FILE_EQ:
                case Detector::TEST_FILE_LE:
                case Detector::TEST_FILE_GR:
                    if ((sqlite3_bind_int64(stmt_test, 5, static_cast<int64_t>(test.length)) != SQLITE_OK) || sqlite3_bind_null(stmt_test, 6) != SQLITE_OK || sqlite3_bind_null(stmt_test, 7) != SQLITE_OK) {
                        return false;
                    }
                    break;
            }
            
            if (sqlite3_step(stmt_test) != SQLITE_DONE || sqlite3_reset(stmt_test) != SQLITE_OK) {
                return false;
            }
        }
    }
    
    return true;
}


bool RomDB::delete_game(const std::string &name) {
    auto stmt = db.get_statement(DBH_STMT_QUERY_GAME_ID);
    if (stmt == NULL) {
        return false;
    }

    if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
        return false;
    }

    int ret;
    int id;
    if ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    if (ret == SQLITE_DONE) {
        return true;
    }
    else if (ret != SQLITE_ROW) {
        return false;
    }

    bool ok = true;

    if ((stmt = db.get_statement(DBH_STMT_DELETE_GAME)) == NULL) {
        return false;
    }
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        ok = false;
    }

    if ((stmt = db.get_statement(DBH_STMT_DELETE_FILE)) == NULL) {
        return false;
    }
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        ok = false;
    }

    return ok;
}


bool RomDB::update_file_location(Game *game) {
    auto stmt = db.get_statement(DBH_STMT_UPDATE_FILE);
    if (stmt == NULL) {
        return false;
    }

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (size_t i = 0; i < game->files[ft].size(); i++) {
            if (sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(game->id)) != SQLITE_OK || sqlite3_bind_int(stmt, 3, static_cast<int>(ft)) != SQLITE_OK) {
                return false;
            }

            File &rom = game->files[ft][i];
            if (rom.where == FILE_INGAME) {
                continue;
            }

            if (sqlite3_bind_int(stmt, 1, rom.where) != SQLITE_OK || sqlite3_bind_int(stmt, 4, static_cast<int>(i)) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
                return false;
            }
            
            sqlite3_reset(stmt);
        }
    }

    return true;
}


bool RomDB::update_game_parent(const Game *game) {
    auto stmt = db.get_statement(DBH_STMT_UPDATE_PARENT);

    if (stmt  == NULL || sq3_set_string(stmt, 1, game->cloneof[0]) != SQLITE_OK || sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(game->id)) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return true;
}


bool RomDB::write_game(Game *game) {
    delete_game(game);

    auto stmt = db.get_statement(DBH_STMT_INSERT_GAME);
    if (stmt == NULL) {
        return false;
    }

    if (sq3_set_string(stmt, 1, game->name) != SQLITE_OK || sq3_set_string(stmt, 2, game->description) != SQLITE_OK || sqlite3_bind_int(stmt, 3, static_cast<int>(game->dat_no)) != SQLITE_OK || sq3_set_string(stmt, 4, game->cloneof[0]) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    game->id = static_cast<uint64_t>(sqlite3_last_insert_rowid(db.db));

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        if (!write_files(game, static_cast<filetype_t>(ft))) {
            delete_game(game);
            return false;
        }
    }
    
    return true;
}


bool RomDB::write_files(Game *game, filetype_t ft) {
    auto stmt = db.get_statement(DBH_STMT_INSERT_FILE);
    if (stmt == NULL) {
        return false;
    }

    for (size_t i = 0; i < game->files[ft].size(); i++) {
        const File &rom = game->files[ft][i];

        if (sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(game->id)) != SQLITE_OK || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK || sqlite3_bind_int(stmt, 3, static_cast<int>(i)) != SQLITE_OK || sq3_set_string(stmt, 4, rom.name) != SQLITE_OK || sq3_set_string(stmt, 5, rom.merge) != SQLITE_OK || sqlite3_bind_int(stmt, 6, rom.status) != SQLITE_OK || sqlite3_bind_int(stmt, 7, rom.where) != SQLITE_OK || sq3_set_uint64_default(stmt, 8, rom.size, SIZE_UNKNOWN) != SQLITE_OK || sq3_set_hashes(stmt, 9, &rom.hashes, 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_reset(stmt);
    }

    return true;
}

int RomDB::export_db(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out) {
    DatEntry de;

    if (out == NULL) {
	/* TODO: split into original dat files */
	return 0;
    }

    auto db_dat = read_dat();

    /* TODO: export detector */

    de.merge(dat, (db_dat.size() == 1 ? &db_dat[0] : NULL));
    out->header(&de);

    auto list = read_list(DBH_KEY_LIST_GAME);
    if (list.empty()) {
	myerror(ERRDEF, "db error reading game list");
	return -1;
    }

    for (size_t i = 0; i < list.size(); i++) {
        GamePtr game = read_game(list[i]);
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
