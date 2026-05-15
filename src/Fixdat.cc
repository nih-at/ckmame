/*
  Fixdat.cc -- write fixdat
  Copyright (C) 2012-2014 Dieter Baron and Thomas Klausner

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

#include "Fixdat.h"

#include "RomDB.h"
#include "globals.h"
#include "util.h"

std::vector<std::optional<Fixdat>> Fixdat::fixdats;

void Fixdat::begin() {
    auto dats = db->read_dat();

    for (const auto& dat : dats) {
        if (configuration.dat_create_fixdat(dat.name)) {
            fixdats.emplace_back(dat);
        }
        else {
            fixdats.emplace_back(std::nullopt);
        }
    }
}

void Fixdat::end() {
    for (auto& fixdat : fixdats) {
        if (fixdat) {
            fixdat->cleanup();
        }
    }

    fixdats.clear(); 
}

void Fixdat::cleanup() {
    if (output) {
        output->finish();
        output = nullptr;
    }

    auto directory = configuration.fixdat_directory.empty() ? std::filesystem::current_path() : std::filesystem::path(configuration.fixdat_directory);
    if (!std::filesystem::is_directory(directory)) {
        return;
    }
    auto current_filename = empty ? "" : fixdat_filename();
    auto prefix = fixdat_filename_prefix();
    for (auto file: std::filesystem::directory_iterator(directory)) {
        auto fname = file.path().filename().string();
        if (file.is_regular_file() && fname.starts_with(prefix) && fname.ends_with(".dat") && fname != current_filename && fname.substr(prefix.size(), fname.size() - prefix.size() - 4).find('(') == std::string::npos) {
            std::filesystem::remove(file);
        }
    }
}

void Fixdat::write_entry(const Game* game, const Result* result) { 
    if (fixdats[game->dat_no]) {
        fixdats[game->dat_no]->write(game, result);
    }
}

void Fixdat::write(const Game* game, const Result* result) {
    if (failed) {
        return;
    }
    if (!GS_HAS_MISSING(result->game)) {
        return;
    }

    auto gm = std::make_shared<Game>();
    gm->name = game->name;

    auto has_missing = false;

    for (auto ft : db->filetypes()) {
        for (size_t i = 0; i < game->files[ft].size(); i++) {
            auto& match = result->game_files[ft][i];
            auto& rom = game->files[ft][i];

            /* no use requesting zero byte files */
            if (rom.hashes.size == 0) {
                continue;
            }

            if (match.quality != Match::MISSING || rom.status == Rom::NO_DUMP || rom.where != FILE_INGAME) {
                continue;
            }

            gm->files[ft].push_back(rom);
            has_missing = true;
        }
    }

    if (has_missing) {
        empty = false;
        if (ensure_output()) {
            output->add_game(gm);
        }
    }
}

std::string Fixdat::fixdat_filename_prefix() const {
    return "fixdat_" + dat.name + " ";
}
std::string Fixdat::fixdat_filename() const {
    return fixdat_filename_prefix() + "(" + dat.version + ").dat";
} 

bool Fixdat::ensure_output() {
    if (output) {
        return true;
    }

    if (failed) {
        return false;
    }

    auto fixdat_fname = fixdat_filename();
    if (!configuration.fixdat_directory.empty()) {
        ensure_dir(configuration.fixdat_directory, false);
        fixdat_fname = configuration.fixdat_directory + "/" + fixdat_fname;
    }

    DatEntry de;

    de.name = "Fixdat for " + dat.name + " (" + dat.version + ")";
    de.description = "Fixdat by ckmame";
    de.version = format_time("%Y-%m-%d %H:%M:%S", time(nullptr));

    if ((output = OutputContext::create(OutputContext::FORMAT_DATAFILE_XML, fixdat_fname, 0)) == nullptr) {
        failed = true;
        return false;
    }

    DatOptions options;
    options.only_last_duplicate = true;
    if (!output->start_dat(options, Output::FileInfo()) || !output->add_header(de)) {
        output = nullptr;
        failed = true;
        return false;
    }

    return true;
}
