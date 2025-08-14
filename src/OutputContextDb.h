#ifndef HAD_OUTPUT_DB_H
#define HAD_OUTPUT_DB_H

/*
  OutputContextDb.h -- write games to DB
  Copyright (C) 2006-2020 Dieter Baron and Thomas Klausner

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

#include "OutputContext.h"
#include "RomDB.h"


class OutputContextDb : public OutputContext {
public:
    OutputContextDb(const std::string &fname, int flags);
    ~OutputContextDb() override;
    
    bool close() override;
    bool detector(Detector *detector) override;
    bool game(GamePtr game, const std::string &original_name) override;
    bool header(DatEntry *dat) override;
    void error_occurred() override { ok = false; }

    RomDB* get_db() const {return db.get();}

private:
    std::string file_name;
    std::string temp_file_name;

    std::unique_ptr<RomDB> db;

    std::vector<DatEntry> dat;

    std::vector<std::string> lost_children;

    bool ok;
    
    void familymeeting(Game *parent, Game *child);
    std::string get_game_name(const std::string& original_name);
    bool handle_lost();
    bool lost(Game *);

    std::unordered_map<std::string, std::string> renamed_games;
};

#endif // HAD_OUTPUT_DB_H
