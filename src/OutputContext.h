#ifndef HAD_OUTPUT_CONTEXT_H
#define HAD_OUTPUT_CONTEXT_H

/*
  OutputContext.h -- output game info
  Copyright (C) 2006-2024 Dieter Baron and Thomas Klausner

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

#include <map>
#include <memory>
#include <string>

#include "DatEntry.h"
#include "Detector.h"
#include "Game.h"
#include "Hashes.h"
#include "Output.h"
#include "SharedFile.h"

class OutputContext;

typedef std::shared_ptr<OutputContext> OutputContextPtr;

#define OUTPUT_FL_RUNTEST 1

/**
 * Options for parsing dat files.
 * 
 * @member full_archive_names If set, keep directory names in archive names, instead of only the base name. (default: false)
 * @member game_name_suffix A suffix to add to game names. (default: empty)
 * @member suffix_only_duplicates If set, `game_name_suffix` will only be added to game names that are duplicates. (default: false)
 * @member use_description_as_name If set, use the description as the game name. (default: false)
 * @member mia_games A set of game names that should be considered MIA. (default: empty)
 * @member only_last_duplicate If set, only the last game with duplicate name will be kept. (default: false)
 */
class DatOptions {
  public:
    DatOptions(std::optional<std::string> dat_name);
    DatOptions() = default;

    bool full_archive_names = false;
    std::string game_name_suffix;
    bool suffix_only_duplicates = false;
    bool use_description_as_name = false;
    std::unordered_set<std::string> mia_games;
    bool only_last_duplicate = false;

    /**
     * Get the suffix to add to a game with duplicate name.
     *
     * @return The suffix to add to the game name.
     */

    std::string name_suffix() const { return suffix_only_duplicates ? "" : game_name_suffix; }
    std::string duplicate_name_suffix() const {return suffix_only_duplicates ? game_name_suffix : "";}
};

/**
 * OutputContext is used to collect information and write the actual dat file.
 * 
 * Interface for parser:
 * - set_dat_info: Optional, must be called first. Set info for created dat, otherwise use info from first dat.
 * - start_dat: Must be called once for each dat. Start a new dat with the given options.
 * - add_header: Must be called once for each dat. Set header info for current dat.
 * - add_detector: Optional, can be called multiple times. Add a detector to the dat.
 * - add_game: Optional, can be called multiple times. Add a game to the dat.
 * - found_game: Optional. Only called when the parser is run with header-only mode.
 * - error_occurred: Optional. Signal that an error occurred during parsing.
 * - finish: Must be called last. Write output.
 * 
 * Methods that must be implemented by subclasses, called in this order:
 * - write_header: Will be called once. Write header info for current dat.
 * - write_dat: Will be called once for each dat. Write dat info for current dat.
 * - write_detector: Will be called once for each detector. Write a detector to the dat.
 * - write_game: Will be called once for each game, sorted by name. Write a game to the dat.
 */
class OutputContext {
  public:
    enum Format { FORMAT_CM, FORMAT_DATAFILE_XML, FORMAT_DB, FORMAT_MTREE };

    virtual ~OutputContext() = default;

    static OutputContextPtr create(Format format, const std::string& fname, int flags);

    // Called by Parser.
    bool set_dat_info(const DatEntry& dat);
    bool add_detector(const Detector& detector);
    bool add_game(GamePtr game);
    bool add_header(const DatEntry& dat);
    void error_occurred() { ok = false; }
    virtual bool found_game() { return true; }
    bool finish();
    bool start_dat(DatOptions options, Output::FileInfo file_info);


  protected:
    // Provided by subclasses.
    virtual bool close() { return true; }
    virtual bool write_detector(const Detector& detector) { return true; }
    virtual bool write_game(const GamePtr game) = 0;
    virtual bool write_header(const DatEntry& dat) { return true; }
    virtual bool write_dat(size_t index, const DatEntry& dat) { return true; }

    // Utility methods for subclasses.
    void cond_print_string(FILEPtr f, const std::string& pre, const std::string& str, const std::string& post);
    void cond_print_hash(FILEPtr f, const std::string& pre, int t, const Hashes* h, const std::string& post);

    /**
     * Set to false if an error occurred during parsing.
     */
    bool ok = true;

    /**
     * If set, the parser will only parse the header and whether it's empty.
     */
    bool header_only = false;

  private:
    class Dat {
        public:
            Dat(DatOptions options, Output::FileInfo error_info) : options(std::move(options)), error_info(std::move(error_info)) {}
    
            std::optional<DatEntry> dat;
            DatOptions options;
            Output::FileInfo error_info;
    };

    class Name {
      public:
        explicit Name(size_t dat_no, std::string name) : dat_no(dat_no), name(std::move(name)) {}

        size_t dat_no;
        std::string name;

        std::strong_ordering operator<=>(const Name& other) const;
    };

    const std::string& final_game_name(size_t dat_no, const std::string& name) const;

    const DatEntry* get_header() const;
    bool fix_game(Game* game, std::unordered_set<Game*> fixing = {});

    /**
     * Get the current dat number. Must not be called if no dat has been started.
     * 
     * @return The current dat number.
     */
    size_t current_dat_no() const { return dats.size() - 1; }

    /**
     * Get info for the current dat. Must not be called if no dat has been started.
     * 
     * @return The current dat info.
     */
    const Dat& current_dat() const { return dats.back(); }

    /**
     * Check if a dat has been started. Must be called before calling any method that requires a dat to be started.
     * 
     * @return true if a dat has been started, false otherwise.
     */
    bool dat_started() const { return !dats.empty(); }

    /**
     * The header information for the dat file. If not set explicitly, the header information from the first dat will be used.
     */
    std::optional<DatEntry> header;

    /**
     * Header information for each dat file.
     */
    std::vector<Dat> dats;

    /**
     * Detectors for the dat file.
     */
    std::vector<Detector> detectors;

    /**
     * Games, grouped by name. Games with the same name will be renamed to avoid duplicates.
     */
    std::unordered_map<std::string, std::vector<GamePtr>> games_by_name;

    /**
     * Map from original game name to renamed game name, used to adjust cloneof fields.
     */
    std::map<Name, std::string> renamed_games;

    /**
     * Games for which the where fields have been fixed.
     */
    std::unordered_set<Game*> fixed_where_games;

    /**
     * Final list of games.
     */
    std::unordered_map<std::string, GamePtr> games;
};

#endif // HAD_OUTPUT_CONTEXT_H
