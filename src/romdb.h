#ifndef _HAD_ROMDB_H
#define _HAD_ROMDB_H

/*
  romdb.h -- mame.db sqlite3 data base
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include "dbh.h"

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
    bool write_detector(const detector_t *detector);
    bool write_game(Game *game);
    bool write_hashtypes(int, int);

private:
    int hashtypes_[TYPE_MAX];
    
    bool read_disks(Game *game);
    void read_hashtypes(filetype_t type);
    bool read_roms(Game *game);
    bool read_rules(Detector *detector);
    bool write_disks(Game *game);
    bool write_roms(Game *game);
    bool write_rules(const Detector *detector);
};

typedef class RomDB romdb_t;

<<<<<<< Updated upstream
int romdb_close(romdb_t *);
int romdb_delete_game(romdb_t *, const char *);
int romdb_has_disks(romdb_t *);
romdb_t *romdb_open(const char *, int);
std::vector<DatEntry> romdb_read_dat(romdb_t *db);
DetectorPtr romdb_read_detector(romdb_t *db);
array_t *romdb_read_file_by_hash(romdb_t *, filetype_t, const Hashes *);
array_t *romdb_read_file_by_name(romdb_t *, filetype_t, const char *);
GamePtr romdb_read_game(romdb_t *db, const std::string &name);
int romdb_hashtypes(romdb_t *, filetype_t);
parray_t *romdb_read_list(romdb_t *, enum dbh_list);
int romdb_update_file_location(romdb_t *, Game *);
int romdb_update_game_parent(romdb_t *, const Game *);
int romdb_write_dat(romdb_t *db, const std::vector<DatEntry> &dats);
int romdb_write_detector(romdb_t *db, const detector_t *);
int romdb_write_file_by_hash_parray(romdb_t *, filetype_t, const Hashes *, parray_t *);
int romdb_write_game(romdb_t *, Game *);
int romdb_write_hashtypes(romdb_t *, int, int);
int romdb_write_list(romdb_t *, const char *, const parray_t *);
=======
>>>>>>> Stashed changes

#endif /* romdb.h */
