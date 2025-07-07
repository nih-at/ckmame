#ifndef _HAD_ROMDB_H
#define _HAD_ROMDB_H

/*
  RomDB.h -- mame.db sqlite3 data base
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

#include <unordered_set>

#include "DB.h"
#include "RomLocation.h"
#include "OutputContext.h"
#include "Stats.h"

class RomDB : public DB {
public:
    enum Statement {
        DELETE_FILE,
        DELETE_GAME,
        INSERT_DAT_DETECTOR,
        INSERT_DAT,
        INSERT_FILE,
        INSERT_GAME,
        INSERT_RULE,
        INSERT_TEST,
        QUERY_CLONES,
        QUERY_DAT_DETECTOR,
        QUERY_DAT,
        QUERY_FILE_FBN,
        QUERY_FILE,
        QUERY_GAME_ID,
        QUERY_GAME,
        QUERY_HAS_FILE_TYPE,
        QUERY_HASH_TYPE_CRC,
        QUERY_HASH_TYPE_MD5,
        QUERY_HASH_TYPE_SHA1,
        QUERY_HASH_TYPE_SHA256,
        QUERY_LIST_DISK,
        QUERY_LIST_GAME,
        QUERY_LIST_MIA,
        QUERY_PARENT_BY_NAME,
        QUERY_PARENT,
        QUERY_RULE,
        QUERY_STATS_FILES,
        QUERY_STATS_GAMES,
        QUERY_TEST,
        UPDATE_FILE,
        UPDATE_PARENT
    };
    
    enum ParameterizedStatement {
        QUERY_FILE_FBH
    };

    class FileTypeIterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = filetype_t;

            FileTypeIterator(const bool* types, int file_type);

            FileTypeIterator& operator++();
            FileTypeIterator operator++(int) {auto next=*this; ++*this; return next;}
            filetype_t operator*() const {return static_cast<filetype_t>(file_type);}
            bool operator==(const FileTypeIterator& other) const {return file_type == other.file_type;}
            bool operator!=(const FileTypeIterator& other) const {return file_type != other.file_type;}

        private:
            void skip_missing();
            const bool* types;
            int file_type;
    };

    class FileTypes {
        public:
            FileTypes(const RomDB& db): types{db.has_types} {}

            FileTypeIterator begin() {return {types, 0};}
            FileTypeIterator end() {return {types, TYPE_MAX};}

        private:
           const bool *types;
    };
    
    RomDB(const std::string &name, int mode);
    ~RomDB() override = default;
    void init2();
    
    static const DBFormat format;
    // These can't be static members, since they are initialized after the global Configuration. Thanks a lot, C++.
    static std::string default_name() { return "mame.db"; }
    static std::string default_old_name() { return "old.db"; }
    
    std::unordered_map<size_t, DetectorPtr> detectors;

    Stats get_stats();
    std::vector<std::string> get_clones(const std::string &game_name);
    void delete_game(const Game *game) { delete_game(game->name); }
    void delete_game(const std::string &name);
    bool has_type(filetype_t type) const {return has_types[type];}
    bool has_disks() const {return has_type(TYPE_DISK);}

    bool has_detector() const { return !detectors.empty(); }
    DetectorPtr get_detector(size_t id);
    size_t get_detector_id_for_dat(size_t dat_no) const;
    [[nodiscard]] bool game_exists(const std::string &name);
    
    std::vector<DatEntry> read_dat();
    std::vector<RomLocation> read_file_by_hash(filetype_t ft, const Hashes &hashes);
    GamePtr read_game(const std::string &name);
    int hashtypes(filetype_t);
    FileTypes filetypes();
    std::vector<std::string> read_list(enum dbh_list type);
    void update_file_location(Game *game);
    void update_game_parent(const Game *game);
    void write_dat(const std::vector<DatEntry> &dats);
    void write_detector(const Detector &detector);
    void write_game(Game *game);
    void write_hashtypes(int, int);
    int export_db(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out);

    bool has_types[TYPE_MAX];

protected:
    std::string get_query(int name, bool parameterized) const override;
    
private:
    int hashtypes_[TYPE_MAX];

    static const std::string init2_sql;
    static const Statement query_hash_type[];
    static std::unordered_map<int, std::string> queries;
    static std::unordered_map<int, std::string> parameterized_queries;

    DBStatement *get_statement(Statement name) { return get_statement_internal(name); }
    DBStatement *get_statement(ParameterizedStatement name, const Hashes &hashes, bool have_size) { return get_statement_internal(name, hashes, have_size); }

    bool get_has_type(int type);
    DetectorPtr read_detector();
    void read_files(Game *game, filetype_t ft);
    void read_hashtypes(filetype_t type);
    bool read_rules(Detector *detector);
    void write_files(Game *game, filetype_t ft);
    void write_rules(const Detector &detector);
};

extern std::unique_ptr<RomDB> db;
extern std::unique_ptr<RomDB> old_db;

#endif // _HAD_ROMDB_H
