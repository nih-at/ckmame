#ifndef HAD_CONFIGURATION_H
#define HAD_CONFIGURATION_H

/*
  Configuration.h -- configuration settings from file
  Copyright (C) 2021-2025 Dieter Baron and Thomas Klausner

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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef HAVE_TOMLPLUSPLUS
#include <toml++/toml.h>
#else
#include "toml.hpp"
#endif

#include "Commandline.h"
#include "TomlSchema.h"

/**
 * Configuration settings from file and command line.
 * 
 * This class is responsible for reading the configuration file and providing access to the configuration settings.
 */
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

    /**
     * Get the name of the user configuration file. This is a file in the user's home directory that can be used to set configuration options for all invocations of ckmame by the user.
     * 
     * @return the name of the user configuration file
     */
    static std::string user_config_file();

    /**
     * Get the name of the local configuration file. This is a file in the current working directory that can be used to set configuration options for all invocations of ckmame in the current directory.
     * 
     * @return the name of the local configuration file
     */
    static std::string local_config_file();

    /**
     * Add command line options for configuration variables. This should be called when initializing the command line parser.
     * 
     * @param commandline the command line parser to add options to
     * @param used_variables the set of configuration variables that are used by the command. Options for these variables will be added to the command line parser.
     */
    static void add_options(Commandline& commandline, const std::unordered_set<std::string>& used_variables);

    void handle_commandline(const ParsedCommandline& commandline);
    void prepare(const std::string& set, const ParsedCommandline& commandline);

    /**
     * Get `allow-empty-dat` setting for the given dat. This controls whether RomDB will be updated from an empty dat.
     * 
     * @param dat the name of the dat to get the setting for
     * @return `true` if RomDB should use an empty dat, `false` otherwise
     */
    bool dat_allow_empty_dat(const std::string& dat);

    /**
     * Get `create-fixdat` setting for the given dat. This controls whether a fixdat should be created for the dat.
     * 
     * @param dat the name of the dat to get the setting for
     * @return `true` if a fixdat should be created for the dat, `false` otherwise
     */
    bool dat_create_fixdat(const std::string& dat);

    /**
     * Get `game-name-suffix` setting for the given dat. This is a suffix that will be added to game names from the dat.
     * 
     * @param dat the name of the dat to get the setting for
     * @return the suffix to be added to game names from the dat
     */
    std::string dat_game_name_suffix(const std::string& dat);

    /**
     * Get `use-central-cache-directory` setting for the given dat. This controls whether the central cache directory will be used for the dat directory.
     * 
     * @param directory the name of the dat to get the setting for
     * @return `true` if the central cache directory should be used for the dat directory, `false` otherwise
     */
    bool dat_directory_use_central_cache_directory(const std::string& directory);

    /**
     * Get `suffix-only-duplicates` setting for the given dat. This controls whether the `game-name-suffix` will only be added to game names that are duplicates.
     * 
     * @param dat the name of the dat to get the setting for
     * @return `true` if the `game-name-suffix` should only be added to duplicates, `false` if it should be added to all game names
     */
    bool dat_suffix_only_duplicates(const std::string& dat);

    /**
     * Get `use-description-as-name` setting for the given dat. This controls whether the description from the dat will be used as the game name instead of the name from the dat.
     * 
     * @param dat the name of the dat to get the setting for
     * @return `true` if the description should be used as the game name, `false` otherwise
     */
    bool dat_use_description_as_name(const std::string& dat);

    /**
     * Get `move-from-extra` setting for the given extra directory. This controls whether files taken from the extra directory will be moved instead of copied.
     * 
     * @param directory the name of the extra directory to get the setting for
     * @return `true` if files should be moved from the extra directory, `false` if they should be copied.
     */
    bool extra_directory_move_from_extra(const std::string& directory);

    /**
     * Get `use-central-cache-directory` setting for the given extra directory. This controls whether the central cache directory will be used for the extra directory.
     * 
     * @param directory the name of the extra directory to get the setting for
     * @return `true` if the central cache directory should be used for the extra directory, `false` otherwise
     */ 
    bool extra_directory_use_central_cache_directory(const std::string& directory);

    /**
     * Check if any sets are defined in the configuration.
     * 
     * @return `true` if any sets are defined, `false` otherwise.
     */
    bool has_sets() const { return !sets.empty(); }

    /// The defined sets.
    std::set<std::string> sets;

    /**
     * The set currently being processed, empty string if no set is being processed.
     * 
     * Any configuration variables will reflect the configuration for this set.
     */
    std::string set;

    // config variables
    bool complete_games_only; // only add ROMs to games if they are complete afterwards.
    std::string complete_list;

    /// Whether to create a fixdat for each dat. This can be overridden for each dat with the `create-fixdat` setting in the dat section.
    bool create_fixdat;

    /// Command line override for `create_fixdat`. This will take precedence over the `create-fixdat` setting in the dat section and the global `create_fixdat` setting.
    std::optional<bool> create_fixdat_override;

    /// Directories to search for dat files, in order.
    std::vector<std::string> dat_directories;

    /// Names of dat files to use, in order.
    std::vector<std::string> dats;

    /// Unknown files in the ROM directory matching the given shell glob pattern will be deleted instead of moved to the unknown directory.
    std::string delete_unknown_pattern;

    /// Directories to search for extra files.
    std::vector<std::string> extra_directories;

    /// Directory to create fixdats in.
    std::string fixdat_directory;

    bool keep_old_duplicate;
    std::string mia_games;
    std::string missing_list;


    /// Whether to move files taken from extra directories instead of copying them. This can be overridden with the `move-from-extra` setting in the extra directory section.
    bool move_from_extra;

    std::string old_db;

    /// Whether to report changes to the complete or missing lists.
    bool report_changes;     /* report changes to complete or missing lists */

    /// Whether to report ROMs that are correct, excluding those marked as mia.
    bool report_correct;     /* report ROMs that are correct */

    /// Whether to also report correct ROMs that are as mia.
    bool report_correct_mia;

    /// Whether to report the status of each ROM on a separate line.
    bool report_detailed;

    /// Whether to report ROMs that are not correct but can be fixed.
    bool report_fixable;

    /// Whether to report ROMs that are missing, excluding those without good dumps or marked as mia.
    bool report_missing;

    /// Whether to also report ROMs that are missing but marked as mia.
    bool report_missing_mia;

    /// Whether to report ROMs that are missing and don't have a good dump.
    bool report_no_good_dump;

    bool report_status;       /* report status of set in ckstatus --all-sets */
    bool report_summary;      /* print statistics about ROM set at end of run */

    /// Name of the ROM database file to use.
    std::string rom_db;

    /// Directory containing the ROM files.
    std::string rom_directory;

    /// Whether the ROM files are in zip archives or subdirectories.
    bool roms_zipped;
    
    /// Directory to save known ROM files for later use.
    std::string saved_directory;

    /// Name of the status database file to use.
    std::string status_db;

    /// Keep runs in the status database that are not older than number of days.
    std::optional<int> status_db_keep_days;

    /// Keep at least the given number of most recent runs in the status database.
    std::optional<int> status_db_keep_runs;

    /// Directory to move unknown files to.
    std::string unknown_directory;

    /// Whether to update the ROM database.
    bool update_database;

    /// Whether to use a central cache directory for cache databases. This can be overridden for dat directories and extra directories with the `use-central-cache-directory` setting in their respective sections.
    bool use_central_cache_directory; // create CkmameDB and DatDB in $HOME/.cache/ckmame

    /// Whether to use the description from the dat as the game name instead of the name from the dat. This can be overridden for each dat with the `use-description-as-name` setting in the dat section.
    bool use_description_as_name;

    /// Whether to create RomDB in a temporary directory and then move it into place.
    bool use_temp_directory;

    /// Whether to use the TORRENTZIP format for zip archives in the ROM set.
    bool use_torrentzip;              // use TORRENTZIP format for zip archives in ROM set.

    /// Whether to print all actions taken to fix the ROM set.
    bool verbose;

    /// Whether to allow updating RomDB from an empty dat. This can be overridden for each dat with the `allow-empty-dat` setting in the dat section.
    bool allow_empty_dat;             // Update RomDB even if dat is empty.

    /// Whether to only add the `game-name-suffix` to game names from the dat that are duplicates. This can be overridden for each dat with the `suffix-only-duplicates` setting in the dat section.
    bool suffix_only_duplicates;

    // TODO: Are these needed? They have no command line options.
    /* file_correct */

    /// Output warning for files in the ROM directory that are known but don't belong in this archive.
    bool warn_file_known;

    /// Output warning for files in the ROM directory that are not in the ROM set.
    bool warn_file_unknown;

    /**
     * Whether to actually fix the ROM set, or just report its current state. 
     * 
     * This is not in the config file but is set from the command line.
     */
    bool fix_romset; // actually fix, otherwise no archive is changed

  private:
    /**
     * Options for individual dat directories.
     */
    class DatDirectoryOptions {
      public:
        /// Whether to use a central cache directory for the dat directory. This overrides the global `use_central_cache_directory` setting.
        std::optional<bool> use_central_cache_directory;
    };

    /**
     * Options for individual dats.
     */
    class DatOptions {
      public:
        /// Whether to allow updating RomDB from an empty dat for the dat. This overrides the global `allow_empty_dat` setting.
        std::optional<bool> allow_empty_dat;

        /// A suffix that will be added to game names from the dat. This overrides the global `game_name_suffix` setting.
        std::optional<std::string> game_name_suffix;

        /// Whether to create a fixdat for the dat. This overrides the global `create_fixdat` setting.
        std::optional<bool> create_fixdat;

        /// Whether to only add the `game-name-suffix` to game names from the dat that are duplicates. This overrides the global `suffix_only_duplicates` setting.
        std::optional<bool> suffix_only_duplicates;

        /// Whether to use the description from the dat as the game name instead of the name from the dat for the dat. This overrides the global `use_description_as_name` setting.
        std::optional<bool> use_description_as_name;
    };

    /**
     * Options for individual extra directories.
     */
    class ExtraDirectoryOptions {
      public:
        /// Whether to move files from the extra directory. This overrides the global `move_from_extra` setting.
        std::optional<bool> move_from_extra;

        /// Whether to use a central cache directory for the extra directory. This overrides the global `use_central_cache_directory` setting.
        std::optional<bool> use_central_cache_directory;
    };

    /// Schema for validate the dat directories sections of the configuration file.
    static TomlSchema::TypePtr dat_directories_schema;

    /// Schema for validate the dats sections of the configuration file.
    static TomlSchema::TypePtr dats_schema;

    /// Schema for validate the extra directories sections of the configuration file.
    static TomlSchema::TypePtr extra_directories_schema;

    /// Schema for validate the global, set, or profile sections of the configuration file.
    static TomlSchema::TypePtr section_schema;

    /// Schema for validate the entire configuration file.
    static TomlSchema::TypePtr file_schema;

    /// Command line options to override configuration settings. The command line options correspond to the configuration variables with `_` replaced by `-`.
    static std::vector<Commandline::Option> commandline_options;

    /// Mapping from command line option names to configuration variable names for command line options that don't directly correspond to a configuration variable.
    static std::unordered_map<std::string, std::string> option_to_variable;

    /**
     * Check if a command line option is used by the command.
     * 
     * @param option_name the name of the command line option to check, without leading dashes
     * @param used_variables the set of configuration variables that are used by the command
     * @return `true` if the command line option is used by the command, `false` otherwise
     */
    static bool option_used(const std::string& option_name, const std::unordered_set<std::string>& used_variables);

    static bool read_config_file(std::vector<toml::table>& config_tables, const std::string& file_name, bool optional);

    void reset();

    /**
     * Set a boolean configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    static void set_bool(const toml::table& table, const std::string& name, bool& variable);

    /**
     * Set an optional boolean configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    static void set_bool_optional(const toml::table& table, const std::string& name, std::optional<bool>& variable);

    /**
     * Set an integer configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    static void set_integer(const toml::table& table, const std::string& name, int& variable);

    /**
     * Set an optional integer configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    static void set_integer_optional(const toml::table& table, const std::string& name, std::optional<int>& variable);

    /**
     * Set an integer configuration variable from a configuration file table, or set it to a special `all` value.
     * 
     * For `all`, the variable is set to no value.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    static void set_integer_or_all(const toml::table& table, const std::string& name, std::optional<int>& variable);
    // TODO: If there is no value in the table, variable will also be set to no value.

    /**
     * Set a string configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    void set_string(const toml::table& table, const std::string& name, std::string& variable);

    /**
     * Set an optional string configuration variable from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     */
    void set_string_optional(const toml::table& table, const std::string& name, std::optional<std::string>& variable);

    /**
     * Set a vector of string configuration variables from a configuration file table.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     * @param append whether to append to the existing vector or replace it
     */
    void set_string_vector(const toml::table& table, const std::string& name, std::vector<std::string>& variable,
                           bool append);

    /**
     * Set a vector of string configuration variables from a a file named in the configuration file table. The file should contain one value per line.
     * 
     * @param table the configuration file table to read the variable from
     * @param name the name of the variable in the configuration file
     * @param variable the variable to set
     * @param append whether to append to the existing vector or replace it
     */
    [[maybe_unused]] static void set_string_vector_from_file(const toml::table& table, const std::string& name,
                                                             std::vector<std::string>& variable, bool append);

    /**
     * Merge a configuration file into the current configuration.
     * 
     * @param file the configuration file to merge
     */
    void merge_config_file(const toml::table& file);

    /**
     * Merge a configuration table into the current configuration.
     * 
     * @param table the configuration table to merge
     */
    void merge_config_table(const toml::table* table);

    void merge_dat_directories(const toml::table& table, const std::string& name, bool append);
    void merge_dats(const toml::table& table);
    void merge_extra_directories(const toml::table& table, const std::string& name, bool append);

    /**
     * Validate a configuration file against the configuration schema.
     * 
     * @param table the configuration file to validate
     * @param file_name the name of the configuration file, used for error messages
     * @return `true` if the configuration file is valid, `false` otherwise
     */
    static bool validate_file(const toml::table& table, const std::string& file_name);

    [[nodiscard]] std::string replace_variables(std::string string) const;

    std::vector<toml::table> config_files;

    std::unordered_map<std::string, DatDirectoryOptions> dat_directory_options;
    std::unordered_map<std::string, DatOptions> dat_options;
    std::unordered_map<std::string, ExtraDirectoryOptions> extra_directory_options;
};

#endif // HAD_CONFIGURATION_H
