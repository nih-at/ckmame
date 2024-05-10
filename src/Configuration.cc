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
#include <filesystem>
#include <set>

#include "Exception.h"
#include "RomDB.h"
#include "util.h"

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

TomlSchema::TypePtr Configuration::dat_directories_schema = TomlSchema::alternatives({
	TomlSchema::array(TomlSchema::string()),
	TomlSchema::table({}, TomlSchema::table({
            { "use-central-cache-directory", TomlSchema::boolean() } }, {}))
    }, "array or table");

TomlSchema::TypePtr Configuration::dats_schema = TomlSchema::alternatives({
	TomlSchema::array(TomlSchema::string()),
	TomlSchema::table({}, TomlSchema::table({
	    { "game-name-suffix", TomlSchema::string() },
	    { "use-description-as-name", TomlSchema::boolean() } }, {}))
    }, "array or table");

TomlSchema::TypePtr Configuration::extra_directories_schema = TomlSchema::alternatives({
        TomlSchema::array(TomlSchema::string()),
        TomlSchema::table({}, TomlSchema::table({
            { "use-central-cache-directory", TomlSchema::boolean() },
            { "move-from-extra", TomlSchema::boolean() } }, {}))
        }, "array or table");

TomlSchema::TypePtr Configuration::section_schema = TomlSchema::table({
    { "complete-games-only", TomlSchema::boolean() },
    { "complete-list", TomlSchema::string() },
    { "create-fixdat",  TomlSchema::boolean() },
    { "dat-directories", dat_directories_schema },
    { "dat-directories-append", dat_directories_schema },
    { "dats", dats_schema },
    { "delete-unknown-pattern", TomlSchema::string() },
    { "extra-directories", extra_directories_schema},
    { "extra-directories-append", extra_directories_schema},
    { "fixdat-directory",  TomlSchema::string() },
    { "keep-old-duplicate",  TomlSchema::boolean() },
    { "missing-list", TomlSchema::string() },
    { "move-from-extra",  TomlSchema::boolean() },
    { "old-db", TomlSchema::string() },
    { "profiles", TomlSchema::array(TomlSchema::string()) },
    { "report-changes",  TomlSchema::boolean() },
    { "report-correct",  TomlSchema::boolean() },
    { "report-detailed",  TomlSchema::boolean() },
    { "report-fixable",  TomlSchema::boolean() },
    { "report-missing",  TomlSchema::boolean() },
    { "report-no-good-dump",  TomlSchema::boolean() },
    { "report-summary",  TomlSchema::boolean() },
    { "rom-directory", TomlSchema::string() },
    { "rom-db", TomlSchema::string() },
    { "roms-zipped",  TomlSchema::boolean() },
    { "saved-directory", TomlSchema::string() },
    { "sets", TomlSchema::array(TomlSchema::string()) },
    { "sets-file", TomlSchema::string() },
    { "unknown-directory", TomlSchema::string() },
    { "update-database",  TomlSchema::boolean() },
    { "use-central-cache-directory", TomlSchema::boolean() },
    { "use-description-as-name",  TomlSchema::boolean() },
    { "use-temp-directory",  TomlSchema::boolean() },
    { "use-torrentzip",  TomlSchema::boolean() },
    { "verbose",  TomlSchema::boolean() },
    { "warn-file-known",  TomlSchema::boolean() },
    { "warn-file-unknown",  TomlSchema::boolean() }
}, {});


TomlSchema::TypePtr Configuration::file_schema = TomlSchema::table({
    { "profile", TomlSchema::table({}, section_schema) }
}, section_schema);


std::vector<Commandline::Option> Configuration::commandline_options = {
    Commandline::Option("complete-games-only", 'C', "only keep complete games in ROM set"),
    Commandline::Option("complete-list", "file", "write list of complete games to file"),
    Commandline::Option("config", "file", "read configuration from file"),
    Commandline::Option("copy-from-extra", "keep used files in extra directories (default)"),
    Commandline::Option("create-fixdat", "write fixdat to 'fix_$NAME_OF_SET.dat'"),
    Commandline::Option("delete-unknown-pattern", "pattern", "delete unknown files matching 'pattern'"),
    Commandline::Option("extra-directory", 'e', "dir", "search for missing files in directory dir (multiple directories can be specified by repeating this option)"),
    Commandline::Option("fixdat-directory", "directory", "create fixdats in directory"),
    Commandline::Option("keep-old-duplicate", "keep files in ROM set that are also in old ROMs"),
    Commandline::Option("list-sets", "list all known sets"),
    Commandline::Option("missing-list", "file", "write list of missing games to file"),
    Commandline::Option("move-from-extra", 'j', "remove used files from extra directories"),
    Commandline::Option("no-complete-games-only", "keep partial games in ROM set (default)"),
    Commandline::Option("no-create-fixdat", "don't create fixdat (default)"),
    Commandline::Option("no-report-changes", "don't report changes to correct and missing lists (default)"),
    Commandline::Option("no-report-correct", "don't report status of ROMs that are correct (default)"),
    Commandline::Option("no-report-detailed", "don't report status of every ROM (default)"),
    Commandline::Option("no-report-fixable", "don't report status of ROMs that can be fixed"),
    Commandline::Option("no-report-missing", "don't report status of ROMs that are missing"),
    Commandline::Option("no-report-no-good-dump", "don't report status of ROMs for which no good dump exists (default)"),
    Commandline::Option("no-report-summary", "don't print summary of ROM set status (default)"),
    Commandline::Option("no-update-database", "don't update ROM database (default)"),
    Commandline::Option("old-db", 'O', "dbfile", "use database dbfile for old ROMs"),
    Commandline::Option("report-changes", "report changes to correct and missing lists"),
    Commandline::Option("report-correct", 'c', "report status of ROMs that are correct"),
    Commandline::Option("report-detailed", "report status of every ROM"),
    Commandline::Option("report-fixable", "report status of ROMs that can be fixed (default)"),
    Commandline::Option("report-missing", "report status of ROMs that are missing (default)"),
    Commandline::Option("report-no-good-dump", "don't suppress reporting status of ROMs for which no good dump exists"),
    Commandline::Option("report-summary", "print summary of ROM set status"),
    Commandline::Option("rom-db", 'D', "dbfile", "use ROM database dbfile"),
    Commandline::Option("rom-directory", 'R', "dir", "ROM set is in directory dir (default: 'roms')"),
    Commandline::Option("roms-unzipped", "ROMs are files on disk, not contained in zip archives"),
    Commandline::Option("saved-directory", "directory", "save needed ROMs in directory (default: 'saved')"),
    Commandline::Option("set", "pattern", "check ROM sets matching pattern"),
    Commandline::Option("unknown-directory", "directory", "save unknown files in directory (default: 'unknown')"),
    Commandline::Option("update-database", "update ROM database if dat files changed"),
    Commandline::Option("use-description-as-name", "use description as name of games in ROM database"),
    Commandline::Option("use-temp-directory", 't', "create output in temporary directory, move when done"),
    Commandline::Option("use-torrentzip", "use TORRENTZIP format for zip archives in ROM set"),
    Commandline::Option("verbose", 'v', "print fixes made"),
    Commandline::Option("warn-file-known", "report status of extra files that are known (default)"),
    Commandline::Option("warn-file-unknown", "report status of extra files that are unknown (default)")
};


std::unordered_map<std::string, std::string> Configuration::option_to_variable = {
    { "copy-from-extra", "move_from_extra" },
    { "extra-directory", "extra_directories" },
    { "no-complete-games-only", "complete_games_only" },
    { "no-create-fixdat", "create_fixdat" },
    { "no-report-changes", "report_changes" },
    { "no-report-correct", "report_correct" },
    { "no-report-detailed", "report_detailed" },
    { "no-report-fixable", "report_fixable" },
    { "no-report-missing", "report_missing" },
    { "no-report-summary", "report_summary" },
    { "no-report-no-good-dump", "report_no_good_dump" },
    { "no-update-database", "update_database" },
    { "no-warn-file-known", "warn-file-known"},
    { "no-warn-file-unknown", "warn-file-unknown"},
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


Configuration::Configuration() : fix_romset(false) {
    reset();
}

void Configuration::reset() {
    complete_games_only = false;
    complete_list = "";
    create_fixdat = false;
    delete_unknown_pattern = "";
    keep_old_duplicate = false;
    missing_list = "";
    move_from_extra = false;
    old_db = RomDB::default_old_name();
    report_correct = false;
    report_changes = false;
    report_detailed = false;
    report_fixable = true;
    report_missing = true;
    report_no_good_dump = false;
    report_summary = false;
    rom_db = RomDB::default_name();
    rom_directory = "roms";
    roms_zipped = true;
    saved_directory = "saved";
    if (!set.empty()) {
        saved_directory += "/" + set;
    }
    unknown_directory = "unknown";
    update_database = false;
    use_central_cache_directory = false;
    use_description_as_name = false;
    use_temp_directory = false;
    use_torrentzip = false;
    verbose = false;
    warn_file_known = true;
    warn_file_unknown = true;
    dat_directories.clear();
    dats.clear();
    extra_directories.clear();
    fixdat_directory = "";

    auto value = getenv("MAMEDB");
    if (value != nullptr) {
	rom_db = value;
    }
    value = getenv("MAMEDB_OLD");
    if (value != nullptr) {
	old_db = value;
    }
}


void Configuration::merge_config_file(const toml::table &file) {
    std::string string;

    auto global_table = file["global"].as_table();
    if (global_table != nullptr) {
        merge_config_table(global_table);
    }

    if (!set.empty()) {
        auto set_table = file[set].as_table();

        if (set_table != nullptr) {
            merge_config_table(set_table);
        }
    }
}


void Configuration::handle_commandline(const ParsedCommandline &args) {
    auto ok = true;

    try {
	ok = read_config_file(config_files, user_config_file(), true) && ok;
	ok = read_config_file(config_files, local_config_file(), true) && ok;


	for (const auto &option : args.options) {
	    if (option.name == "config") {
		ok = read_config_file(config_files, option.argument, false) && ok;
	    }
	}
    } catch (std::exception &ex) {
	throw Exception(std::string(ex.what()));
    }

    if (!ok) {
	exit(1);
    }

    for (const auto &file : config_files) {
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
	    sets.insert(std::string(entry.first));
	}
    }

    if (args.find_first("list-sets").has_value()) {
	for (const auto &set_name : sets) {
	    printf("%s\n", set_name.c_str());
	}

	exit(0);
    }
}



void Configuration::prepare(const std::string &current_set, const ParsedCommandline &commandline) {
    set = current_set;

    reset();

    for (const auto& file : config_files) {
	merge_config_file(file);
    }

    auto extra_directory_specified = false;

    for (const auto &option : commandline.options) {
        if (option.name == "complete-games-only") {
            complete_games_only = true;
        }
        else if (option.name == "complete-list") {
            complete_list = option.argument;
        }
        else if (option.name == "copy-from-extra") {
            move_from_extra = false;
        }
        else if (option.name == "create-fixdat") {
            create_fixdat = true;
        }
        else if (option.name == "delete-unknown-pattern") {
            delete_unknown_pattern = option.argument;
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
        else if (option.name == "missing-list") {
            missing_list = option.argument;
        }
        else if (option.name == "move-from-extra") {
            move_from_extra = true;
        }
        else if (option.name == "no-complete-games-only") {
            complete_games_only = false;
        }
        else if (option.name == "no-create-fixdat") {
            create_fixdat = false;
        }
        else if (option.name == "no-report-changes") {
            report_changes = false;
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
        else if (option.name == "no-report-no-good-dump") {
            report_no_good_dump = false;
        }
        else if (option.name == "no-update-database") {
            update_database = false;
        }
        else if (option.name == "no-warn-file-known") {
            warn_file_known = false;
        }
        else if (option.name == "no-warn-file-unknown") {
            warn_file_unknown = false;
        }
        else if (option.name == "old-db") {
            old_db = option.argument;
        }
        else if (option.name == "report-changes") {
            report_changes = true;
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
        else if (option.name == "report-no-good-dump") {
            report_no_good_dump = true;
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
        else if (option.name == "saved-directory") {
            saved_directory = option.argument;
        }
        else if (option.name == "unknown-directory") {
            unknown_directory = option.argument;
        }
        else if (option.name == "update-database") {
            update_database = true;
        }
        else if (option.name == "use-description-as-name") {
            use_description_as_name = true;
        }
        else if (option.name == "use-temp-directory") {
            use_temp_directory = true;
        }
        else if (option.name == "use-torrentzip") {
            use_torrentzip = true;
        }
        else if (option.name == "verbose") {
            verbose = true;
        }
        else if (option.name == "warn-file-known") {
            warn_file_known = true;
        }
        else if (option.name == "warn-file-unknown") {
            warn_file_unknown = true;
        }
    }
}


void Configuration::merge_config_table(const toml::table *table_pointer) {
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
		merge_config_table(profile_table);
		found = true;
	    }

	    if (!found) {
		throw Exception("unknown profile '%s'", profile.c_str());
	    }
	}
    }

    set_bool(table, "complete-games-only", complete_games_only);
    set_string(table, "complete-list", complete_list);
    set_bool(table, "create-fixdat", create_fixdat);
    merge_dat_directories(table, "dat-directories", false);
    merge_dat_directories(table, "dat-directories-append", true);
    merge_dats(table);
    set_string(table, "delete-unknown-pattern", delete_unknown_pattern);
    set_string(table, "rom-db", rom_db);
    merge_extra_directories(table, "extra-directories", false);
    merge_extra_directories(table, "extra-directories-append", true);
    set_string(table, "fixdat-directory", fixdat_directory);
    set_bool(table, "keep-old-duplicate", keep_old_duplicate);
    set_string(table, "missing-list", missing_list);
    set_bool(table, "move-from-extra", move_from_extra);
    set_string(table, "old-db", old_db);
    set_bool(table, "report-changes", report_changes);
    set_bool(table, "report-correct", report_correct);
    set_bool(table, "report-detailed", report_detailed);
    set_bool(table, "report-fixable", report_fixable);
    set_bool(table, "report-missing", report_missing);
    set_bool(table, "report-summary", report_summary);
    set_bool(table, "report-no-good-dump", report_no_good_dump);
    set_string(table, "rom-directory", rom_directory);
    set_bool(table, "roms-zipped", roms_zipped);
    set_string(table, "saved-directory", saved_directory);
    set_string(table, "unknown-directory", unknown_directory);
    set_bool(table, "update-database", update_database);
    set_bool(table, "use-central-cache-directory", use_central_cache_directory);
    set_bool(table, "use-description-as-name", use_description_as_name);
    set_bool(table, "use-temp-directory", use_temp_directory);
    set_bool(table, "use-torrentzip", use_torrentzip);
    set_bool(table, "verbose", verbose);
    set_bool(table, "warn-file-known", warn_file_known);
    set_bool(table, "warn-file-unknown", warn_file_unknown);
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


void Configuration::set_bool_optional(const toml::table &table, const std::string &name, std::optional<bool> &variable){
    auto value = table[name].value<bool>();
    if (value.has_value()) {
	variable = value.value();
    }
}


void Configuration::set_string(const toml::table &table, const std::string &name, std::string &variable) {
    auto value = table[name].value<std::string>();
    if (value.has_value()) {
        if (!set.empty() || value.value().find("$set") == std::string::npos) {
            variable = replace_variables(value.value());
        }
    }
}


void Configuration::set_string_optional(const toml::table &table, const std::string &name, std::optional<std::string> &variable){
    auto value = table[name].value<std::string>();
    if (value.has_value()) {
        if (!set.empty() || value.value().find("$set") == std::string::npos) {
            variable = replace_variables(value.value());
        }
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
                if (!set.empty() || element_value.value().find("$set") == std::string::npos) {
                    variable.push_back(replace_variables(element_value.value()));
                }
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


bool Configuration::validate_file(const toml::table& table, const std::string& file_name) {
    auto validator = TomlSchema(file_schema);

    return validator.validate(table, file_name);
}


std::string Configuration::dat_game_name_suffix(const std::string &dat) {
    auto it = dat_options.find(dat);
    if (it == dat_options.end() || !it->second.game_name_suffix.has_value()) {
	return "";
    }
    return it->second.game_name_suffix.value();
}


bool Configuration::dat_directory_use_central_cache_directory(const std::string &directory){
    auto it = dat_directory_options.find(directory);
    if (it == dat_directory_options.end() || !it->second.use_central_cache_directory.has_value()) {
        return use_central_cache_directory;
    }
    return it->second.use_central_cache_directory.value();
}


bool Configuration::dat_use_description_as_name(const std::string &dat){
    auto it = dat_options.find(dat);
    if (it == dat_options.end() || !it->second.use_description_as_name.has_value()) {
	return use_description_as_name;
    }
    return it->second.use_description_as_name.value();
}


bool Configuration::extra_directory_move_from_extra(const std::string &directory){
    auto it = extra_directory_options.find(directory);
    if (it == extra_directory_options.end() || !it->second.move_from_extra.has_value()) {
	return move_from_extra;
    }
    return it->second.move_from_extra.value();
}


bool Configuration::extra_directory_use_central_cache_directory(const std::string &directory){
    auto it = extra_directory_options.find(directory);
    if (it == extra_directory_options.end() || !it->second.use_central_cache_directory.has_value()) {
        return use_central_cache_directory;
    }
    return it->second.use_central_cache_directory.value();
}


void Configuration::merge_dat_directories(const toml::table &table, const std::string &name, bool append){
    auto node = table[name];

    if (node.is_array()) {
        set_string_vector(table, name, dat_directories, append);
    }
    else if (node.is_table()) {
        if (!append) {
            dat_directories.clear();
        }
        for (const auto &pair : (*node.as_table())) {
            if (set.empty() && pair.first.str().find("$set") != std::string::npos) {
                continue;
            }
            auto directory = replace_variables(std::string(pair.first));
            dat_directories.emplace_back(directory);
            auto options_table = pair.second.as_table();
            if (options_table != nullptr && !options_table->empty()) {
                auto parsed_options = DatDirectoryOptions();

                set_bool_optional(*options_table, "use-central-cache-directory", parsed_options.use_central_cache_directory);
                dat_directory_options[directory] = parsed_options;
            }
        }
    }
}

void Configuration::merge_dats(const toml::table& table) {
    auto node = table["dats"];

    if (node.is_array()) {
	set_string_vector(table, "dats", dats, false);
    }
    else if (node.is_table()) {
	dats.clear();
	for (const auto &pair : (*node.as_table())) {
            if (set.empty() && pair.first.str().find("$set") != std::string::npos) {
                continue;
            }
            auto dat = replace_variables(std::string(pair.first));
	    dats.emplace_back(dat);
	    auto options_table = pair.second.as_table();
	    if (options_table != nullptr && !options_table->empty()) {
		auto parsed_options = DatOptions();

		set_string_optional(*options_table, "game-name-suffix", parsed_options.game_name_suffix);
		set_bool_optional(*options_table, "use-description-as-name", parsed_options.use_description_as_name);
		dat_options[dat] = parsed_options;
	    }
	}
    }
}


void Configuration::merge_extra_directories(const toml::table &table, const std::string &name, bool append){
    auto node = table[name];

    if (node.is_array()) {
	set_string_vector(table, name, extra_directories, append);
    }
    else if (node.is_table()) {
	if (!append) {
	    extra_directories.clear();
	}
	for (const auto &pair : (*node.as_table())) {
            if (set.empty() && pair.first.str().find("$set") != std::string::npos) {
                continue;
            }
            auto directory = replace_variables(std::string(pair.first));
	    extra_directories.emplace_back(directory);
	    auto options_table = pair.second.as_table();
	    if (options_table != nullptr && !options_table->empty()) {
		auto parsed_options = ExtraDirectoryOptions();

		set_bool_optional(*options_table, "move-from-extra", parsed_options.move_from_extra);
                set_bool_optional(*options_table, "use-central-cache-directory", parsed_options.use_central_cache_directory);
		extra_directory_options[directory] = parsed_options;
	    }
	}
    }
}
