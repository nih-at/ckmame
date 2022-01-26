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
#include "util.h"

#ifdef HAVE_TOMLPLUSPLUS
#include <toml++/toml.h>
#else
#include "toml.hpp"
#endif

std::string Configuration::local_config_file() {
    return ".ckmamerc";
}


std::string Configuration::user_config_file() {
    auto home = getenv("HOME");
    
    if (home == nullptr) {
        return "";
    }
    
    return std::string(home) + "/.config/ckmame/ckmamerc";
}


Configuration::Configuration() :
    romdb_name(RomDB::default_name()),
    olddb_name(RomDB::default_old_name()),
    rom_directory("roms"),
    roms_zipped(true),
    keep_old_duplicate(false),
    fix_romset(false),
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
    if (value != nullptr) {
        romdb_name = value;
    }
    value = getenv("MAMEDB_OLD");
    if (value != nullptr) {
        olddb_name = value;
    }
}

static void set_bool(const toml::table &table, const std::string &name, bool &variable);
static void set_string(const toml::table &table, const std::string &name, std::string &variable);
static void set_string_vector(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append);


bool Configuration::merge_config_file(const std::string &fname, const std::string &set, bool optional) {
    std::string string;
    
    auto set_known = false;
    
    try {
        string = slurp(fname);
    }
    catch (const std::exception &ex) {
        if (optional) { // TODO: only if file doesn't exist
            return false;
        }
        throw ex; // TODO: convert to Exception
    }

    auto table = toml::parse(string);
    
    auto global_table = table["global"].as_table();
    if (global_table != nullptr) {
        merge_config_table(global_table);
        
        if (!set.empty()) {
            auto known_sets = (*global_table)["sets"].as_array();
            if (known_sets != nullptr) {
                for (const auto &value : *known_sets) {
                    auto name = value.value<std::string>();
                    if (name.has_value() && set == name.value()) {
                        set_known = true;
                        break;
                    }
                }
            }
        }
    }
    
    if (!set.empty()) {
        auto set_table = table[set].as_table();
        
        if (set_table != nullptr) {
            merge_config_table(set_table);
            set_known = true;
        }
    }
    else {
        set_known = true;
    }
    
    return set_known;
}


void Configuration::merge_config_table(void *p) {
    const auto &table = *reinterpret_cast<toml::table *>(p);

    // TODO: autofixdat
    set_bool(table, "complete-games-only", complete_games_only);
    set_string(table, "db", romdb_name);
    set_string_vector(table, "extra-directory", extra_directories, false);
    set_string_vector(table, "extra-directory-append", extra_directories, true);
    set_string(table, "fixdat", fixdat);
    set_bool(table, "keep-old-duplicate", keep_old_duplicate);
    set_bool(table, "move-from-extra", move_from_extra);
    set_string(table, "old-db", olddb_name);
    set_bool(table, "report-correct", report_correct);
    set_bool(table, "report-detailed", report_detailed);
    set_bool(table, "report-fixable", report_fixable);
    set_bool(table, "report-missing", report_missing);
    set_bool(table, "report-summary", report_summary);
    set_string(table, "rom-dir", rom_directory);
    set_bool(table, "roms-unzipped", roms_zipped);
    set_bool(table, "verbose", verbose);
    // set_bool(table, "warn-file-known", warn_file_known);
    // set_bool(table, "warn-file-unknown", warn_file_unknown);
}


// TODO: exceptions for type mismatch

static void set_bool(const toml::table &table, const std::string &name, bool &variable) {
    auto value = table[name].value<bool>();
    if (value.has_value()) {
        variable = value.value();
    }
}


static void set_string(const toml::table &table, const std::string &name, std::string &variable) {
    auto value = table[name].value<std::string>();
    if (value.has_value()) {
        variable = value.value();
    }
}


static void set_string_vector(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append) {
    auto array = table[name].as_array();
    if (array != nullptr) {
        if (!append) {
            variable.clear();
        }
        for (const auto &element : *array) {
            auto value = element.value<std::string>();
            if (value.has_value()) {
                variable.push_back(value.value());
            }
        }
    }
}
