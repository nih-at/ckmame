#ifndef HAD_CONFIGURATION_H
#define HAD_CONFIGURATION_H

/*
  Configuration.h -- configuration settings from file
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

#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#ifdef HAVE_TOMLPLUSPLUS
#include <toml++/toml.h>
#else
#include "toml.hpp"
#endif

#include "Commandline.h"
#include "TomlSchema.h"

class Configuration {
public:
    Configuration();

    /*
     fixdat none/auto/filename
     Complete-only yes/no
     Dbfile filename
     Search-dir directories
     Unknown ignore/move/delete
     Long move/delete
     Old-db filename
     Rom-db filename
     Rom-dir directory
     Stats yes/no
     Duplicate keep/delete (in old db)
     zipped yes/no
     Superfluous keep/delete
     Extra-used keep/delete
     Extra-unused keep/delete

     */

    static std::string user_config_file();
    static std::string local_config_file();

    static void add_options(Commandline &commandline, const std::unordered_set<std::string> &used_variables);

    void handle_commandline(const ParsedCommandline &commandline);
    void prepare(const std::string& set, const ParsedCommandline& commandline);

    std::string dat_game_name_suffix(const std::string& dat);
    bool dat_directory_use_central_cache_directory(const std::string& directory);
    bool dat_use_description_as_name(const std::string& dat);
    bool extra_directory_move_from_extra(const std::string& directory);
    bool extra_directory_use_central_cache_directory(const std::string& directory);

    std::set<std::string> sets;
    std::string set;

    // config variables
    bool complete_games_only; // only add ROMs to games if they are complete afterwards.
    std::string complete_list;
    bool create_fixdat;
    std::vector<std::string> dat_directories;
    std::vector<std::string> dats;
    std::string delete_unknown_pattern;
    std::vector<std::string> extra_directories;
    std::string fixdat_directory;
    bool keep_old_duplicate;
    std::string mia_games;
    std::string missing_list;
    bool move_from_extra; // remove files taken from extra directories, otherwise copy them and don't change extra directory.
    std::string old_db;
    bool report_changes; /* report changes to complete or missing lists */
    bool report_correct; /* report ROMs that are correct */
    bool report_correct_mia; /* report ROMs that are correct but marked as mia in ROM db. */
    bool report_detailed; /* one line for each ROM */
    bool report_fixable; /* report ROMs that are not correct but can be fixed */
    bool report_missing; /* report missing ROMs with good dumps that are not marked as mia in ROM db, one line per game if no own ROM found */
    bool report_missing_mia; /* report missing ROMs that are marked as mia in ROM db. */
    bool report_no_good_dump; /* report ROMs that are not correct and can not be fixed */
    bool report_summary; /* print statistics about ROM set at end of run */
    std::string rom_db;
    std::string rom_directory;
    bool roms_zipped;
    std::string saved_directory;
    std::string unknown_directory;
    bool update_database;
    bool use_central_cache_directory; // create CkmameDB and DatDB in $HOME/.cache/ckmame
    bool use_description_as_name; // in ROM database
    bool use_temp_directory; // create RomDB in temporary directory, then move into place
    bool use_torrentzip; // use TORRENTZIP format for zip archives in ROM set.
    bool verbose; // print all actions taken to fix ROM set

    // TODO: Are these needed? They have no command line options.
    /* file_correct */
    bool warn_file_known;   // files that are known but don't belong in this archive
    bool warn_file_unknown; // files that are not in ROM set

    // not in config files, per invocation
    bool fix_romset; // actually fix, otherwise no archive is changed

private:
    class DatDirectoryOptions {
      public:
        std::optional<bool> use_central_cache_directory;
    };

    class DatOptions {
      public:
        std::optional<std::string> game_name_suffix;
	std::optional<bool> use_description_as_name;
    };

    class ExtraDirectoryOptions {
      public:
	std::optional<bool> move_from_extra;
        std::optional<bool> use_central_cache_directory;
    };

    static TomlSchema::TypePtr dat_directories_schema;
    static TomlSchema::TypePtr dats_schema;
    static TomlSchema::TypePtr extra_directories_schema;
    static TomlSchema::TypePtr section_schema;
    static TomlSchema::TypePtr file_schema;

    static std::vector<Commandline::Option> commandline_options;
    static std::unordered_map<std::string, std::string> option_to_variable;

    static bool option_used(const std::string &option_name, const std::unordered_set<std::string> &used_variables);
    static bool read_config_file(std::vector<toml::table> &config_tables, const std::string &file_name, bool optional);
    void reset();
    static void set_bool(const toml::table &table, const std::string &name, bool &variable);
    static void set_bool_optional(const toml::table &table, const std::string &name, std::optional<bool>& variable);
    void set_string(const toml::table &table, const std::string &name, std::string &variable);
    void set_string_optional(const toml::table &table, const std::string &name, std::optional<std::string>& variable);
    void set_string_vector(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append);
    [[maybe_unused]] static void set_string_vector_from_file(const toml::table &table, const std::string &name, std::vector<std::string> &variable, bool append);

    void merge_config_file(const toml::table &file);
    void merge_config_table(const toml::table *table);

    void merge_dat_directories(const toml::table& table, const std::string& name, bool append);
    void merge_dats(const toml::table& table);
    void merge_extra_directories(const toml::table& table, const std::string& name, bool append);

    static bool validate_file(const toml::table& table, const std::string& file_name);

    [[nodiscard]] std::string replace_variables(std::string string) const;

    std::vector<toml::table> config_files;

    std::unordered_map<std::string, DatDirectoryOptions> dat_directory_options;
    std::unordered_map<std::string, DatOptions> dat_options;
    std::unordered_map<std::string, ExtraDirectoryOptions> extra_directory_options;
};

#endif // HAD_CONFIGURATION_H
