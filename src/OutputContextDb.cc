/*
  OutputContextDb.cc -- write games to DB
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

#include "OutputContextDb.h"

#include <algorithm>
#include <filesystem>

#include "file_util.h"
#include "globals.h"


struct fbh_context {
    sqlite3* db;
    filetype_t ft;
};


OutputContextDb::OutputContextDb(const std::string& dbname, int flags) : file_name(dbname) {
    temp_file_name = file_name + "-mkmamedb";
    if (configuration.use_temp_directory) {
        auto tmpdir = getenv("TMPDIR");
        std::filesystem::path basename = temp_file_name;
        basename = basename.filename();
        temp_file_name = std::string(tmpdir ? tmpdir : "/tmp") + "/" + basename.string();
    }
    temp_file_name = make_unique_name(temp_file_name, "");

    db = std::make_unique<RomDB>(temp_file_name, DBH_NEW);
}


OutputContextDb::~OutputContextDb() {
    try {
        close();
    }
    catch (...) {
    }
}



bool OutputContextDb::close() {
    if (db) {
        db->init2();

        db = nullptr;

        if (ok) { // TODO: and no previous errors
            rename_or_move(temp_file_name, file_name);
        }
        else {
            std::filesystem::remove(temp_file_name);
        }
    }

    return ok;
}


bool OutputContextDb::write_detector(const Detector& detector) {
    db->write_detector(detector);

    return true;
}


bool OutputContextDb::write_game(const GamePtr game) {
    db->write_game(game.get());

    return true;
}


bool OutputContextDb::write_header(const DatEntry& entry) {
    return true;
}


bool OutputContextDb::write_dat(size_t index, const DatEntry& entry) {
    db->write_dat(index, entry);

    return true;
}
