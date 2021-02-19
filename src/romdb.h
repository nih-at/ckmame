#ifndef _HAD_ROMDB_H
#define _HAD_ROMDB_H

/*
  romdb.h -- mame.db sqlite3 data base
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

#include "dbh.h"
#include "file_location.h"
#include "output.h"

class RomDB {
public:
    RomDB(const std::string &name, int mode);

    DB db;

    bool delete_game(const Game *game) { return delete_game(game->name); }
    bool delete_game(const std::string &name);
    int has_disks();

    std::vector<DatEntry> read_dat();
    DetectorPtr read_detector();
    std::vector<FileLocation> read_file_by_hash(filetype_t ft, const Hashes *hash);
    GamePtr read_game(const std::string &name);
    int hashtypes(filetype_t);
    std::vector<std::string> read_list(enum dbh_list type);
    bool update_file_location(Game *game);
    bool update_game_parent(const Game *game);
    bool write_dat(const std::vector<DatEntry> &dats);
    bool write_detector(const Detector *detector);
    bool write_game(Game *game);
    bool write_hashtypes(int, int);
    int export_db(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out);
private:
    int hashtypes_[TYPE_MAX];
    
    bool read_files(Game *game, filetype_t ft);
    void read_hashtypes(filetype_t type);
    bool read_rules(Detector *detector);
    bool write_files(Game *game, filetype_t ft);
    bool write_rules(const Detector *detector);
};

extern std::unique_ptr<RomDB> db;
extern std::unique_ptr<RomDB> old_db;

#endif /* romdb.h */
