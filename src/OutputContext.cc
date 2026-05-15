/*
  OutputContext.cc -- output game info
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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

#include "config.h"

#include "OutputContext.h"

#include <algorithm>
#include <string>

#include "globals.h"
#include "util.h"
#include "OutputContextCm.h"
#include "OutputContextDb.h"
#include "OutputContextMtree.h"
#include "OutputContextXml.h"


/**
 * Create a new OutputContext of the given format.
 * 
 * @param format The format to create.
 * @param fname The file name to write to, or empty for stdout.
 * @param flags Additional flags.
 * @return The created OutputContext, or nullptr on failure.
 */
OutputContextPtr OutputContext::create(OutputContext::Format format, const std::string& fname, int flags) {
    switch (format) {
    case FORMAT_CM:
        return std::make_shared<OutputContextCm>(fname, flags);

    case FORMAT_DB:
        return std::make_shared<OutputContextDb>(fname, flags);

#if defined(HAVE_LIBXML2)
    case FORMAT_DATAFILE_XML:
        return std::make_shared<OutputContextXml>(fname, flags);
#endif

    case FORMAT_MTREE:
        return std::make_shared<OutputContextMtree>(fname, flags);

    default:
        return nullptr;
    }
}


void OutputContext::cond_print_string(FILEPtr f, const std::string& pre, const std::string& str,
                                      const std::string& post) {
    if (str.empty()) {
        return;
    }

    std::string out;
    if (str.find_first_of(" \t") == std::string::npos) {
        out = pre + str + post;
    }
    else {
        out = pre + "\"" + str + "\"" + post;
    }

    fprintf(f.get(), "%s", out.c_str());
}


void OutputContext::cond_print_hash(FILEPtr f, const std::string& pre, int t, const Hashes* h,
                                    const std::string& post) {
    cond_print_string(f, pre, h->to_string(t), post);
}

/**
 * Set the dat info for the created dat.
 * 
 * @param dat The dat info to set.
 * @return true on success, false on failure.
 */
bool OutputContext::set_dat_info(const DatEntry& dat) {
    if (header) {
        output.error("dat info already set");
        return false;
    }
    header = dat;
    return true;
}

/**
 * Start a new dat. This is called by the parser once for each dat before any information from the dat is added.
 * 
 * @param options The options for the new dat.
 * @return true on success, false on failure.
 */
bool OutputContext::start_dat(DatOptions options, Output::FileInfo file_info) {
    dats.emplace_back(options, file_info);
    return true;
}

/**
 * Add a header for the current dat. This is called by the parser once for each dat. `start_dat` must have been called before.
 * 
 * @param dat The header info for the dat.
 * @return true on success, false on failure.
 */
bool OutputContext::add_header(const DatEntry& dat) {
    if (!dat_started()) {
        output.error("start_dat must be called before add_header");
        return false;
    }
    dats[current_dat_no()].dat = dat;
    return true;
}

/**
 * Add detector for the current dat. This is called by the parser for each detector found in the dat. `start_dat` must have been called before.
 * 
 * @param detector The detector to add.
 * @return true on success, false on failure.
 */
bool OutputContext::add_detector(const Detector& detector) {
    for (const auto& d : detectors) {
        if (d.name == detector.name && d.author == detector.author && d.version == detector.version) {
            return true;
        }
    }
    detectors.push_back(detector);
    return true;
}


/**
 * Add a game to the created dat.
 * 
 * This is called by the parser for each game found in the dat. `start_dat` must have been called before.
 * 
 * @param game The game to add.
 * @return true on success, false on failure.
 */
bool OutputContext::add_game(GamePtr game) {
    if (!dat_started()) {
        output.error("start_dat must be called before add_game");
        return false;
    }
    if (current_dat().options.only_last_duplicate) {
        games_by_name[game->name].clear();
    }
    else {
        for (const auto& g : games_by_name[game->name]) {
            if (*g == *game) {
                return true;
            }
        }
    }
    game->dat_no = current_dat_no();
    games_by_name[game->name].push_back(game);
    return true;
}

/**
 * Fix inconsistencies and write output. This is called after all parsing of all dats is finished.
 * 
 * @return true on success, false on failure.
 */
bool OutputContext::finish() {
    // TODO: call close() even if an error occurred.
    
    if (!ok) {
        return false;
    }

    if (header_only) {
        if (dats.size() == 1) {
            auto& dat = dats[0];
            if (dat.dat) {
                write_header(*dat.dat);
            }
        }
        else if (dats.size() > 1) {
            output.error("multiple dats found, but header-only mode enabled");
            return false;
        }
        return true;
    }

    // Fix duplicate names.
    for (const auto& [name, named_games] : games_by_name) {
        if (named_games.size() > 1) {
            std::unordered_map<std::string, int> counts;

            for (const auto& game : named_games) {
                auto suffix = dats[game->dat_no].options.duplicate_name_suffix();
                auto original_name = game->name;
                auto new_name = game->name;
                if (!suffix.empty()) {
                    new_name += " " + suffix;
                }
                if (counts[new_name] > 0) {
                    new_name += " (" + std::to_string(counts[new_name]) + ")";
                    output.error("warning: duplicate game '%s', renamed to '%s'", original_name.c_str(), new_name.c_str());
                }
                counts[new_name] += 1;
                game->name = new_name;
                games[game->name] = game;
                if (game->name != game->original_name) {
                    renamed_games[Name(game->dat_no, game->original_name)] = game->name;
                }
            }
        }
        else {
            auto game = named_games[0];
            games[game->name] = game;
            if (game->name != game->original_name) {
                renamed_games[Name(game->dat_no, game->original_name)] = game->name;
            }
        }
    }

    // Fix other game fields.
    for (const auto& [name, game] : games) {
        if (!fix_game(game.get())) {
            ok = false;
        }
    }

    if (!ok) {
        return false;
    }

    auto header = get_header();
    if (header) {
        write_header(*header);
    }
    else {
        output.error("no header information");
        ok = false;
    }
        
    size_t index = 0;
    for (const auto& dat : dats) {
        if (dat.dat) {
            write_dat(index, *dat.dat);
        }
        else {
            // TODO: How to handle missing header? Error or write empty header?
        }
        index++;
    }

    for (const auto& detector : detectors) {
        write_detector(detector);
    }

    // Write games.
    std::vector<GamePtr> sorted_games;
    for (const auto& [name, game] : games) {
        sorted_games.push_back(game);
    }
    std::sort(sorted_games.begin(), sorted_games.end(), [](const GamePtr& a, const GamePtr& b) {
        return a->name < b->name;
    });
    for (const auto& game : sorted_games) {
        write_game(game);
    }
    
    return close();
}


std::strong_ordering OutputContext::Name::operator<=>(const Name& other) const {
    if (dat_no != other.dat_no) {
        return dat_no <=> other.dat_no;
    }
    return name <=> other.name;
}


/**
 * Get the final name for a game.
 * 
 * @param dat_no The dat number of the game, used to distinguish games with the same name in different dats.
 * @param name The original name of the game.
 * @return The final name of the game.
 */
const std::string& OutputContext::final_game_name(size_t dat_no, const std::string& name) const {
    if (name.empty()) {
        return name;
    }
    auto it = renamed_games.find(Name(dat_no, name));
    if (it != renamed_games.end()) {
        return it->second;
    }
    return name;
}


/**
 * Create a new DatOptions object for the given dat name using configuration settings.
 * 
 * @param dat_name The name of the dat.
 */
DatOptions::DatOptions(std::optional<std::string> dat_name) {
    if (dat_name) {
        game_name_suffix = configuration.dat_game_name_suffix(*dat_name);
        use_description_as_name = configuration.dat_use_description_as_name(*dat_name);
    }
    else {
        use_description_as_name = configuration.use_description_as_name;
    }
    if (!configuration.mia_games.empty()) {
        auto list = slurp_lines(configuration.mia_games);
        mia_games.insert(list.begin(), list.end());
    }
}


bool OutputContext::fix_game(Game* game, std::unordered_set<Game*> fixing) {
    if (fixed_where_games.contains(game)) {
        return true;
    } 
    fixed_where_games.insert(game);

    const auto& error_info = dats[game->dat_no].error_info;

    if (fixing.contains(game)) {
        output.file_info_error(error_info, "circular cloneof detected for game '%s'", game->name.c_str());
        return false;
    }
    fixing.insert(game);

    if (game->original_name == game->name) {
        game->original_name = "";
    }

    std::vector<Game*> parents;
    for (size_t i = 0; i < 2; i++) {
        game->cloneof[i] = final_game_name(game->dat_no, game->cloneof[i]);
        if (!game->cloneof[i].empty()) {
            auto it = games.find(game->cloneof[i]);
            if (it != games.end()) {
                auto parent = it->second.get();
                if (!fix_game(parent, fixing)) {
                    return false;
                }
                parents.push_back(it->second.get());
                for (size_t j = 0; j < i; j++) {
                    if (!parent->cloneof[j].empty()) {
                        if (i + j > 2) {
                            output.file_info_error(error_info, "game '%s' has more than 2 ancestors, which is not supported", game->name.c_str());
                            return false;
                        }
                        if (game->cloneof[i + j].empty()) {
                            game->cloneof[i + j] = parent->cloneof[j];
                        }
                        else if (game->cloneof[i + j] != parent->cloneof[j]) {
                            output.file_info_error(error_info, "game '%s' has inconsistent cloneof fields with parent '%s'", game->name.c_str(), parent->name.c_str());
                            return false;
                        }
                    }
                }
            }
            else {
                output.file_info_error(error_info, "inconsistency: %s has non-existent parent %s", game->name.c_str(),
                                game->cloneof[i].c_str());
                game->cloneof[i] = "";
                parents.push_back(nullptr);
            }
        }
        else {
            parents.push_back(nullptr);
        }
    }

    // If parent doesn't exist but grandparent does, move it up to parent.
    if (game->cloneof[0].empty() && !game->cloneof[1].empty()) {
        game->cloneof[0] = game->cloneof[1];
        game->cloneof[1] = "";
        parents.erase(parents.begin());
    }

    if (!game->cloneof[0].empty()) {
        /* Look for files in ancestors. */
        for (size_t ft = 0; ft < TYPE_MAX; ft++) {
            for (auto& file : game->files[ft]) {
                for (size_t i = 0; i < 2; i++) {
                    if (parents[i]) {
                        auto mergeable_file = parents[i]->find_mergeable_file(static_cast<filetype_t>(ft), &file);
                        if (mergeable_file) {
                            file.where = static_cast<where_t>(mergeable_file->where + i + 1);
                            break;
                        }
                    }
                }

                if (file.where == FILE_INGAME && !file.merge.empty()) {
                    output.file_info_error(error_info, "In game '%s': '%s': merged from '%s', but ancestors don't contain matching file",
                                    game->name.c_str(), file.name.c_str(), file.merge.c_str());
                }
            }
        }
    }

    return true;
}

const DatEntry* OutputContext::get_header() const {
    if (header) {
        return &*header;
    }
    else if (!dats.empty() && dats[0].dat) {
        return &*dats[0].dat;
    }
    else {
        return nullptr;
    }
}
