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

#include <algorithm>
#include <cstdlib>
#include <set>

#include "Exception.h"
#include "RomDB.h"
#include "util.h"
#include "error.h"

bool Configuration::read_config_file(std::vector<toml::table> &config_tables, const std::string &file_name, bool optional) {
    auto ok = true;

    if (!std::filesystem::exists(file_name)) {
	if (optional) {
	    return true;
	}
	else {
	    throw Exception("config file '%s' does not exist", file_name.c_str());
	}
    }

    try {
	auto string = slurp(file_name);
	auto table = toml::parse(string);
	ok = validate_file(table, file_name);
	config_tables.push_back(table);
    }
    catch (const std::exception &ex) {
	throw Exception("can't parse '%s': %s", file_name.c_str(), ex.what());
    }

    return ok;
}


std::unordered_map<std::string, Configuration::VariableType> Configuration::variable_types = {
    { "complete-games-only", BOOL },
    { "create-fixdat", BOOL },
    { "dat-directories", ARRAY_OF_STRINGS },
    { "dat-directories-append", ARRAY_OF_STRINGS },
    { "dats", ARRAY_OF_STRINGS },
    { "extra-directories", ARRAY_OF_STRINGS },
    { "extra-directories-append", ARRAY_OF_STRINGS },
    { "fixdat-directory", STRING },
    { "keep-old-duplicate", BOOL },
    { "move-from-extra", BOOL },
    { "old-db", STRING },
    { "profiles", ARRAY_OF_STRINGS },
    { "report-correct", BOOL },
    { "report-detailed", BOOL },
    { "report-fixable", BOOL },
    { "report-missing", BOOL },
    { "report-summary", BOOL },
    { "rom-directory", STRING },
    { "rom-db", STRING },
    { "roms-zipped", BOOL },
    { "sets", ARRAY_OF_STRINGS },
    { "sets-file", STRING },
    { "verbose", BOOL }
};

std::vector<Commandline::Option> Configuration::commandline_options = {
    Commandline::Option("complete-games-only", 'C', "only keep complete games in ROM set"),
    Commandline::Option("config", "file", "read configuration from file"),
    Commandline::Option("copy-from-extra", "keep used files in extra directories (default)"),
    Commandline::Option("create-fixdat", "write fixdat to 'fix_$NAME_OF_SET.dat'"),
    Commandline::Option("extra-directory", 'e', "dir", "search for missing files in directory dir (multiple directories can be specified by repeating this option)"),
    Commandline::Option("fixdat-directory", "directory", "create fixdats in directory"),
    Commandline::Option("keep-old-duplicate", "keep files in ROM set that are also in old ROMs"),
    Commandline::Option("list-sets", "list all known sets"),
    Commandline::Option("move-from-extra", 'j', "remove used files from extra directories"),
    Commandline::Option("no-complete-games-only", "keep partial games in ROM set (default)"),
    Commandline::Option("no-create-fixdat", "don't create fixdat (default)"),
    Commandline::Option("no-report-correct", "don't report status of ROMs that are correct (default)"),
    Commandline::Option("no-report-detailed", "don't report status of every ROM (default)"),
    Commandline::Option("no-report-fixable", "don't report status of ROMs that can be fixed"),
    Commandline::Option("no-report-missing", "don't report status of ROMs that are missing"),
    Commandline::Option("no-report-summary", "don't print summary of ROM set status (default)"),
    Commandline::Option("old-db", 'O', "dbfile", "use database dbfile for old ROMs"),
    Commandline::Option("report-correct", 'c', "report status of ROMs that are correct"),
    Commandline::Option("report-detailed", "report status of every ROM"),
    Commandline::Option("report-fixable", "report status of ROMs that can be fixed (default)"),
    Commandline::Option("report-missing", "report status of ROMs that are missing (default)"),
    Commandline::Option("report-summary", "print summary of ROM set status"),
    Commandline::Option("rom-db", 'D', "dbfile", "use ROM database dbfile"),
    Commandline::Option("rom-directory", 'R', "dir", "ROM set is in directory dir (default: 'roms')"),
    Commandline::Option("roms-unzipped", 'u', "ROMs are files on disk, not contained in zip archives"),
    Commandline::Option("set", "name", "check ROM set name"),
    Commandline::Option("verbose", 'v', "print fixes made")
};


std::unordered_map<std::string, std::string> Configuration::option_to_variable = {
    { "copy-from-extra", "move_from_extra" },
    { "extra-directory", "extra_directories" },
    { "no-complete-games-only", "complete_games_only" },
    { "no-create-fixdat", "create_fixdat" },
    { "no-report-correct", "report_correct" },
    { "no-report-detailed", "report_detailed" },
    { "no-report-fixable", "report_fixable" },
    { "no-report-missing", "report_missing" },
    { "no-report-summary", "report_summary" },
    { "roms-unzipped", "roms_zipped" }
};


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


bool Configuration::option_used(const std::string &option_name, const std::unordered_set<std::string> &used_variables) {
    if (option_name == "config" || option_name == "set" || option_name == "list-sets") {
	return true;
    }

    auto variable_name = option_name;
    std::replace(variable_name.begin(), variable_name.end(), '-', '_');

    if (used_variables.find(variable_name) != used_variables.end()) {
	return true;
    }

    auto it = option_to_variable.find(option_name);
    if (it != option_to_variable.end() && used_variables.find(it->second) != used_variables.end()) {
	return true;
    }

    return false;
}


Configuration::Configuration() : rom_db(RomDB::default_name()), old_db(RomDB::default_old_name()),
    rom_directory("roms"),
    create_fixdat(false),
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
	rom_db = value;
    }
    value = getenv("MAMEDB_OLD");
    if (value != nullptr) {
	old_db = value;
    }
}


void Configuration::merge_config_file(const toml::table &file, const std::vector<toml::table> &config_files) {
    std::string string;

    auto global_table = file["global"].as_table();
    if (global_table != nullptr) {
        merge_config_table(global_table, config_files);
    }
    
    if (!set.empty()) {
        auto set_table = file[set].as_table();
        
        if (set_table != nullptr) {
            merge_config_table(set_table, config_files);
        }
    }
}


void Configuration::handle_commandline(const ParsedCommandline &args) {
    std::vector<toml::table> config_files;

    auto ok = true;

    ok = read_config_file(config_files, user_config_file(), true) && ok;
    ok = read_config_file(config_files, local_config_file(), true) && ok;


    for (const auto &option : args.options) {
	if (option.name == "config") {
	    ok = read_config_file(config_files, option.argument, false) && ok;
	}
    }

    if (!ok) {
	exit(1);
    }

    for (const auto& file : config_files) {
	auto known_sets = file["global"]["sets"].as_array();
	if (known_sets != nullptr) {
	    for (const auto &value : *known_sets) {
		auto name = value.value<std::string>();
		if (name.has_value()) {
		    sets.insert(name.value());
		}
	    }
	}
	auto sets_file = file["global"]["sets-file"].value<std::string>();
	if (sets_file.has_value()) {
	    auto lines = slurp_lines(sets_file.value());
	    sets.insert(lines.begin(), lines.end());
	}

	for (const auto &entry : file) {
	    if (entry.first == "global" || entry.first == "profile") {
		continue;
	    }
	    sets.insert(entry.first);
	}
    }

    if (args.find_first("list-sets").has_value()) {
	for (const auto &set_name : sets) {
	    printf("%s\n", set_name.c_str());
	}

	exit(0);
    }

    for (const auto &option : args.options) {
	if (option.name == "set") {
	    set = option.argument;
	    if (sets.find(set) == sets.end()) {
		throw Exception("unknown set '" + set + "'");
	    }
	}
    }

    for (const auto& file : config_files) {
	merge_config_file(file, config_files);
    }

    auto extra_directory_specified = false;

    for (const auto &option : args.options) {
	if (option.name == "complete-games-only") {
	    complete_games_only = true;
	}
	else if (option.name == "copy-from-extra") {
	    move_from_extra = false;
	}
	else if (option.name == "create-fixdat") {
	    create_fixdat = true;
	}
	else if (option.name == "extra-directory") {
	    if (!extra_directory_specified) {
		extra_directories.clear();
		extra_directory_specified = true;
	    }
	    std::string name = option.argument;
	    auto last = name.find_last_not_of('/');
	    if (last == std::string::npos) {
		name = "/";
	    }
	    else {
		name.resize(last + 1);
	    }
	    extra_directories.push_back(name);
	}
	else if (option.name == "fixdat-directory") {
	    fixdat_directory = option.argument;
	}
	else if (option.name == "keep-old-duplicate") {
	    keep_old_duplicate = true;
	}
	else if (option.name == "move-from-extra") {
	    move_from_extra = true;
	}
	else if (option.name == "old-db") {
	    old_db = option.argument;
	}
	else if (option.name == "no-complete-games-only") {
	    complete_games_only = false;
	}
	else if (option.name == "no-create-fixdat") {
	    create_fixdat = false;
	}
	else if (option.name == "no-report-correct") {
	    report_correct = false;
	}
	else if (option.name == "no-report-detailed") {
	    report_detailed = false;
	}
	else if (option.name == "no-report-fixable") {
	    report_fixable = false;
	}
	else if (option.name == "no-report-missing") {
	    report_missing = false;
	}
	else if (option.name == "no-report-summary") {
	    report_summary = false;
	}
	else if (option.name == "report-correct") {
	    report_correct = true;
	}
	else if (option.name == "report-detailed") {
	    report_detailed = true;
	}
	else if (option.name == "report-fixable") {
	    report_fixable = true;
	}
	else if (option.name == "report-missing") {
	    report_missing = true;
	}
	else if (option.name == "report-summary") {
	    report_summary = true;
	}
	else if (option.name == "rom-db") {
	    rom_db = option.argument;
	}
	else if (option.name == "rom-directory") {
	    rom_directory = option.argument;
	}
	else if (option.name == "roms-unzipped") {
	    roms_zipped = false;
	}
	else if (option.name == "verbose") {
	    verbose = true;
	}
    }
}


void Configuration::merge_config_table(const toml::table *table_pointer, const std::vector<toml::table> &config_files) {
    const auto& table = *table_pointer;

    const auto profiles = table["profiles"].as_array();
    if (profiles != nullptr) {
	for (const auto &value : *profiles) {
	    auto profile_value = value.value<std::string>();
	    if (!profile_value.has_value()) {
		continue;
	    }
	    const auto& profile = profile_value.value();

	    auto found = false;

	    for (const auto &file : config_files) {
		auto profile_table = file["profile"][profile].as_table();
		if (profile_table == nullptr) {
		    continue;
		}
		merge_config_table(profile_table, config_files);
		found = true;
	    }

	    if (!found) {
		throw("unknown profile '%s'", profile.c_str());
	    }
	}
    }

    set_bool(table, "complete-games-only", complete_games_only);
    set_bool(table, "create-fixdat", create_fixdat);
    set_string_vector(table, "dat-directories", dat_directories, false);
    set_string_vector(table, "dat-directories-append", dat_directories, true);
    set_string_vector(table, "dats", dats, false);
    set_string(table, "rom-db", rom_db);
    set_string_vector(table, "extra-directories", extra_directories, false);
    set_string_vector(table, "extra-directories-append", extra_directories, true);
    set_string(table, "fixdat-directory", fixdat_directory);
    set_bool(table, "keep-old-duplicate", keep_old_duplicate);
    set_bool(table, "move-from-extra", move_from_extra);
    set_string(table, "old-db", old_db);
    set_bool(table, "report-correct", report_correct);
    set_bool(table, "report-detailed", report_detailed);
    set_bool(table, "report-fixable", report_fixable);
    set_bool(table, "report-missing", report_missing);
    set_bool(table, "report-summary", report_summary);
    set_string(table, "rom-directory", rom_directory);
    set_bool(table, "roms-zipped", roms_zipped);
    set_bool(table, "verbose", verbose);
    // set_bool(table, "warn-file-known", warn_file_known);
    // set_bool(table, "warn-file-unknown", warn_file_unknown);
}


void Configuration::add_options(Commandline &commandline, const std::unordered_set<std::string> &used_variables) {
    for (const auto &option : commandline_options) {
	if (option_used(option.name, used_variables)) {
	    commandline.add_option(option);
	}
    }
}


// TODO: exceptions for type mismatch

void Configuration::set_bool(const toml::table &table, const std::string &name, bool &variable) {
    auto value = table[name].value<bool>();
    if (value.has_value()) {
	variable = value.value();
    }
}


void Configuration::set_string(const toml::table &table, const std::string &name, std::string &variable) {
    auto value = table[name].value<std::string>();
    if (value.has_value()) {
	variable = replace_variables(value.value());
    }
}


void Configuration::set_string_vector(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append) {
    auto array = table[name].as_array();
    if (array != nullptr) {
        if (!append) {
            variable.clear();
        }
        for (const auto &element : *array) {
            auto element_value = element.value<std::string>();
            if (element_value.has_value()) {
		variable.push_back(replace_variables(element_value.value()));
            }
        }
    }
}


[[maybe_unused]] void Configuration::set_string_vector_from_file(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append) {
    auto file_name = table[name].value<std::string>();
    auto lines = slurp_lines(file_name.value());

    if (!append) {
	variable.clear();
    }

    variable.insert(variable.end(), lines.begin(), lines.end());
}


std::string Configuration::replace_variables(std::string string) const {
    auto start = string.find("$set");
    if (start != std::string::npos) {
	string.replace(start, 4, set);
    }
    return string;
}


bool Configuration::validate_tables(const toml::table& table, const std::string &prefix) {
    auto ok = true;

    for (const auto& pair : table) {
	const auto& name = pair.first;
	auto section_table = pair.second.as_table();

	auto sub_prefix = prefix + name;

	if (section_table == nullptr) {
	    myerror(ERRDEF, "%s: expected table", sub_prefix.c_str());
	    ok = false;
	    continue;
	}
	if (name == "profile") {
	    ok = validate_tables(*section_table, sub_prefix + ".") && ok;
	}
	else {
	    ok = validate_table(section_table, sub_prefix) && ok;
	}
    }

    return ok;
}


bool Configuration::validate_table(const toml::table *table, const std::string &prefix) {
    auto ok = true;

    for (const auto& pair : *table) {
	const auto& name = pair.first;
	const auto& value = pair.second;

	auto it = variable_types.find(name);

	if (it == variable_types.end()) {
	    myerror(ERRDEF, "%s.%s: unknown variable", prefix.c_str(), name.c_str());
	    ok = false;
	    continue;
	}

	switch (it->second) {
	case ARRAY_OF_STRINGS:
	    if (!(value.is_array() && value.is_homogeneous(toml::node_type::string))) {
		myerror(ERRDEF, "%s.%s: expected array of strings", prefix.c_str(), name.c_str());
		ok = false;
	    }
	    break;

	case BOOL:
	    if (!value.is_boolean()) {
		myerror(ERRDEF, "%s.%s: expected boolean", prefix.c_str(), name.c_str());
		ok = false;
	    }
	    break;

	case INTEGER:
	    if (!value.is_integer()) {
		myerror(ERRDEF, "%s.%s: expected integer", prefix.c_str(), name.c_str());
		ok = false;
	    }
	    break;

	case NUMBER:
	    if (!value.is_number()) {
		myerror(ERRDEF, "%s.%s: expected number", prefix.c_str(), name.c_str());
		ok = false;
	    }
	    break;

	case STRING:
	    if (!value.is_string()) {
		myerror(ERRDEF, "%s.%s: expected string", prefix.c_str(), name.c_str());
		ok = false;
	    }
	    break;
	}
    }

    return ok;
}
