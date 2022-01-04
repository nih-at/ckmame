/*
  Configuration.cc -- configuration settings from file
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

#include "Configuration.h"

#include <cstdlib>

#include "config.h"
#include "RomDB.h"

#ifdef HAVE_TOMLPLUSPLUS
#include <toml++/toml.h>
#else
#include "toml.hpp"
#endif

Configuration::Configuration() :
    romdb_name(RomDB::default_name()),
    olddb_name(RomDB::default_old_name()),
    rom_directory("roms"),
    roms_zipped(true),
    fix_romset(false),
    keep_old_duplicate(false),
    verbose(false),
    complete_games_only(false),
    move_from_extra(false),
    report_correct(false),
    report_detailed(false),
    report_fixable(true),
    report_missing(true),
    report_summary(false),
    warn_file_known(true), // ???
    warn_file_unknown(true) {
    auto value = getenv("MAMEDB");
    if (value != NULL) {
        romdb_name = value;
    }
    value = getenv("MAMEDB_OLD");
    if (value != NULL) {
        olddb_name = value;
    }
}
