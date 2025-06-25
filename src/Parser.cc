/*
  Parser.cc -- parser frontend
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

#include "config.h"
// fnmatch
#include "compat.h"

#include "Parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include "ParserCm.h"
#include "ParserRc.h"
#include "ParserXml.h"
#include "globals.h"
#include "util.h"

static const std::string unicode_bom = "\xEF\xBB\xBF";

std::unordered_map<std::string, Parser::Format> Parser::format_start = {
    {"<?xml ",      XML       },
    {"BEGIN",       CLRMAMEPRO},
    {"[CREDITS]",   ROMCENTER },
    {"[DAT]",       ROMCENTER },
    {"[EMULATOR]",  ROMCENTER },
    {"clrmamepro ", CLRMAMEPRO},
    {"emulator ",   CLRMAMEPRO}
};


#define CHECK_STATE(s)                                        \
    do {                                                      \
        if (state != (s)) {                                   \
            output.line_error(lineno, "state is %s, expected %s", state_name(state).c_str(), state_name(s).c_str()); \
            error = true;                                     \
            return false;                                     \
        }                                                     \
    } while (0)


std::string Parser::state_name(parser_state_t state) {
    switch (state) {
    case PARSE_IN_HEADER:
        return "in header";
    case PARSE_IN_GAME:
        return "in game";
    case PARSE_IN_FILE:
        return "in file";
    case PARSE_OUTSIDE:
        return "outside";
    }
    return "invalid";
}

ParserPtr Parser::create(const ParserSourcePtr& source, const std::unordered_set<std::string>& exclude,
                         const DatEntry* dat, OutputContext* output_context, const Options& options) {
    size_t length = 0;
    for (const auto& pair : format_start) {
        length = std::max(pair.first.length(), length);
    }

    auto start = source->peek(length + unicode_bom.length());
    auto format = NONE;

    if (string_starts_with(start, unicode_bom)) {
        start = start.substr(unicode_bom.length());
        char buffer[unicode_bom.length()];
        source->read(buffer, unicode_bom.length());
    }

    for (const auto& pair : format_start) {
        if (string_starts_with(start, pair.first)) {
            format = pair.second;
        }
    }

    switch (format) {
    case NONE:
        return {};

    case CLRMAMEPRO:
        return std::shared_ptr<Parser>(new ParserCm(source, exclude, dat, output_context, options));
    case ROMCENTER:
        return std::shared_ptr<Parser>(new ParserRc(source, exclude, dat, output_context, options));
    case XML:
        return std::shared_ptr<Parser>(new ParserXml(source, exclude, dat, output_context, options));
    }

    return {};
}

bool Parser::parse(const ParserSourcePtr& source, const std::unordered_set<std::string>& exclude, const DatEntry* dat,
                   OutputContext* out, const Options& options) {
    auto parser = create(source, exclude, dat, out, options);

    if (!parser) {
        output.file_error("unknown dat format");
        return false;
    }

    parser->error = false;

    return parser->do_parse() && !parser->error;
}


bool Parser::do_parse() {
    auto ok = parse();
    if (ok) {
        eof();
    }
    return ok;
}

void Parser::eof() {
    if (state == PARSE_IN_HEADER && header_set) {
        header_end();
    }
}


bool Parser::file_continue(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
        output.line_error(lineno, "file continuation only supported for ROMs");
        error = true;
        return false;
    }

    if (flags & PARSE_FL_ROM_DELETED) {
        output.line_error(lineno, "internal error: trying to continue deleted file");
        error = true;
        return false;
    }

    flags |= PARSE_FL_ROM_CONTINUED;

    return true;
}


bool Parser::file_end(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        disk_end();
    }
    else {
        rom_end(ft);
    }

    state = PARSE_IN_GAME;

    return true;
}


bool Parser::file_status(filetype_t ft, const std::string& attr) {
    Rom::Status status;

    CHECK_STATE(PARSE_IN_FILE);

    if (attr == "good" || attr == "verified") {
        status = Rom::OK;
    }
    else if (attr == "baddump") {
        status = Rom::BAD_DUMP;
    }
    else if (attr == "nodump") {
        status = Rom::NO_DUMP;
    }
    else {
        output.line_error(lineno, "illegal status '%s'", attr.c_str());
        error = true;
        return false;
    }

    r[ft]->status = status;

    return true;
}


bool Parser::file_hash(filetype_t ft, int ht, const std::string& attr) {
    Hashes* h;

    CHECK_STATE(PARSE_IN_FILE);

    if (attr == "-") {
        /* some dat files record crc - for 0-byte files, so skip it */
        return true;
    }

    if (attr.empty()) {
        /* some dat files have empty strings for hashes, skip them */
        return true;
    }

    h = &r[ft]->hashes;

    if (h->set_from_string(attr) != ht) {
        output.line_error(lineno, "invalid argument for %s: '%s'", Hashes::type_name(ht).c_str(), attr.c_str());
        error = true;
        return false;
    }

    return true;
}


bool Parser::file_ignore(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
        output.line_error(lineno, "file ignoring only supported for ROMs");
        error = true;
        return false;
    }

    flags |= PARSE_FL_ROM_IGNORE;

    return true;
}


bool Parser::file_merge(filetype_t ft, const std::string& attr) {
    CHECK_STATE(PARSE_IN_FILE);

    r[ft]->merge = attr;

    return true;
}


bool Parser::file_missing(filetype_t ft, bool attr){
    CHECK_STATE(PARSE_IN_FILE);

    r[ft]->mia = attr;

    return true;
}


bool Parser::file_mtime(filetype_t ft, time_t mtime) {
    CHECK_STATE(PARSE_IN_FILE);

    r[ft]->mtime = mtime;

    return true;
}


bool Parser::file_name(filetype_t ft, const std::string& attr) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_ROM) {
        auto name = std::string(attr);

        /* TODO: warn about broken dat file? */
        std::replace(name.begin(), name.end(), '\\', '/');

        r[ft]->name = name;
    }
    else {
        r[ft]->name = attr;
    }

    return true;
}


bool Parser::file_size(filetype_t ft, const std::string& attr) {
    /* TODO: check for strol errors */
    try {
        return file_size(ft, std::stoull(attr, nullptr, 0));
    }
    catch (...) {
        output.line_error(lineno, "invalid size '%s'", attr.c_str());
        error = true;
        return false;
    }
}


bool Parser::file_size(filetype_t ft, uint64_t size) {
    CHECK_STATE(PARSE_IN_FILE);

    r[ft]->hashes.size = size;
    return true;
}


bool Parser::file_start(filetype_t ft) {
    CHECK_STATE(PARSE_IN_GAME);

    g->files[ft].emplace_back();
    r[ft] = &g->files[ft][g->files[ft].size() - 1];

    state = PARSE_IN_FILE;

    return true;
}


bool Parser::game_cloneof(const std::string& attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->cloneof[0] = attr;

    return true;
}


bool Parser::game_description(const std::string& attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->description = attr;

    if (options.use_description_as_name) {
        set_game_name(attr);
    }

    return true;
}

bool Parser::game_end() {
    CHECK_STATE(PARSE_IN_GAME);

    auto ok = true;

    if (g->name.empty()) {
        output.line_error(lineno, "game without name");
        ok = false;
    }
    else if (!ignore_game(g->name)) {
        /* omit description if same as name (to save space) */
        if (g->name == g->description) {
            g->description = "";
        }

        if (g->cloneof[0] == g->name) {
            g->cloneof[0] = "";
        }

        if (options.mia_games.contains(g->name)) {
            // TODO: disks
            for (auto& rom: g->files[TYPE_ROM]) {
                if (!configuration.delete_unknown_pattern.empty() &&
                    fnmatch(configuration.delete_unknown_pattern.c_str(), rom.name.c_str(), 0) == 0) {
                    continue;
                }
                rom.mia = true;
            }
        }

        ok = output_context->game(g, g->name == original_game_name ? "" : original_game_name);
    }

    g = nullptr;

    state = PARSE_OUTSIDE;

    return ok;
}


bool Parser::game_name(const std::string& attr) {
    CHECK_STATE(PARSE_IN_GAME);

    if (!options.use_description_as_name) {
        set_game_name(attr);
    }
    original_game_name = attr;

    return true;
}

void Parser::set_game_name(std::string name) {
    g->name = std::move(name);

    if (!options.full_archive_names) {
        /* slashes are directory separators on some systems, and at least
         * one redump dat contained a slash in a rom name */
        std::replace(g->name.begin(), g->name.end(), '/', '-');
    }

    if (!options.game_name_suffix.empty()) {
        g->name += options.game_name_suffix;
    }
}


bool Parser::game_start() {
    if (state == PARSE_IN_HEADER) {
        if (!header_end()) {
            error = true;
            return false;
        }
    }

    CHECK_STATE(PARSE_OUTSIDE);

    if (g) {
        output.line_error(lineno, "game inside game");
        error = true;
        return false;
    }

    g = std::make_shared<Game>();

    state = PARSE_IN_GAME;

    return true;
}


bool Parser::prog_description(const std::string& attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.description = attr;
    header_set = true;

    return true;
}


bool Parser::prog_header(const std::string& attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    if (detector) {
        output.line_error(lineno, "warning: detector already defined, header '%s' ignored", attr.c_str());
        return true;
    }

    if (header_only) {
        return true;
    }

    auto ok = true;

    ParserSourcePtr dps = ps->open(attr);
    if (!dps) {
        output.line_error_system(lineno, "cannot open detector '%s'", attr.c_str());
        error = true;
        return false;
    }
#if defined(HAVE_LIBXML2)
    detector = Detector::parse(dps.get());
    if (!detector) {
        ok = false;
    }
    else
#endif
    {
        if (!output_context->detector(detector.get())) {
            ok = false;
        }
    }

    return ok;
}


bool Parser::prog_name(const std::string& attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.name = attr;
    header_set = true;

    return true;
}


bool Parser::prog_version(const std::string& attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.version = attr;
    header_set = true;

    return true;
}


Parser::Parser(ParserSourcePtr source, std::unordered_set<std::string> exclude, const DatEntry* dat,
               OutputContext* output_context_, const Options& options)
    : options(options),
      lineno(0),
      header_only(false),
      ignore(std::move(exclude)),
      output_context(output_context_),
      ps(std::move(source)),
      flags(0),
      header_set(false),
      error(false),
      state(PARSE_IN_HEADER) {
    dat_default.merge(dat, nullptr);
    for (auto& i : r) {
        i = nullptr;
    }
}


bool Parser::header_end() {
    CHECK_STATE(PARSE_IN_HEADER);

    if (ps) {
        de.crc = ps->get_crc();
    }
    de.merge(&dat_default, &de);
    if (de.version[0] == '#') {
        de.name += " (Numbered)";
    }
    output_context->header(&de);

    state = PARSE_OUTSIDE;

    return true;
}


void Parser::disk_end() {
    if (r[TYPE_DISK]->hashes.empty() && r[TYPE_DISK]->status == Rom::OK) {
        r[TYPE_DISK]->status = Rom::NO_DUMP;
    }

    if (r[TYPE_DISK]->name == r[TYPE_DISK]->merge) {
        r[TYPE_DISK]->merge = "";
    }
}


bool Parser::ignore_game(const std::string& name) {
    return std::any_of(ignore.begin(), ignore.end(),
                       [&name](const std::string& pattern) { return fnmatch(pattern.c_str(), name.c_str(), 0) == 0; });
}


void Parser::rom_end(filetype_t ft) {
    size_t n = g->files[ft].size() - 1;

    if (r[ft]->hashes.size == 0) {
        auto& hashes = r[ft]->hashes;

        if (hashes.compare(Hashes::zero) == Hashes::MISMATCH) {
            output.line_error(lineno, "warning: zero-size ROM '%s' with wrong checksums, corrected", r[ft]->name.c_str());
            hashes.set_crc(Hashes::zero.crc);
            if (hashes.has_type(Hashes::TYPE_MD5)) {
                hashes.set_md5(Hashes::zero.md5);
            }
            if (hashes.has_type(Hashes::TYPE_SHA1)) {
                hashes.set_sha1(Hashes::zero.sha1);
            }
        }

        hashes.set_crc(Hashes::zero.crc);
    }

    /* omit duplicates */
    auto deleted = false;

    if (flags & PARSE_FL_ROM_IGNORE) {
        deleted = true;
    }
    else if (flags & PARSE_FL_ROM_CONTINUED) {
        auto& rom2 = g->files[ft][n - 1];
        rom2.hashes.size += r[ft]->hashes.size;
        deleted = true;
    }
    else if (r[ft]->name.empty()) {
        output.line_error(lineno, "ROM without name");
        deleted = true;
    }
    for (size_t j = 0; j < n && !deleted; j++) {
        auto& rom2 = g->files[ft][j];
        if (r[ft]->compare_name(rom2)) {
            if (r[ft]->compare_size_hashes(rom2)) {
                /* TODO: merge in additional hash types? */
                deleted = true;
                break;
            }
            else {
                std::string name;
                size_t i = 1;
                while (true) {
                    auto path = std::filesystem::path(r[ft]->name);
                    name = path.stem().string() + " (" + std::to_string(i) + ")" + path.extension().string();
                    if (std::none_of(g->files[ft].begin(), g->files[ft].end(),
                                     [&name](const Rom& rom) { return name == rom.name; })) {
                        break;
                    }
                    i += 1;
                }
                output.line_error(lineno, "warning: two different ROMs with same name '%s', renamed to '%s'",
                                  r[ft]->name.c_str(), name.c_str());
                r[ft]->name = name;
                break;
            }
        }
    }
    if (!r[ft]->merge.empty() && g->cloneof[0].empty()) {
        output.line_error(lineno, "warning: ROM '%s' has merge information but game '%s' has no parent",
                          r[ft]->name.c_str(), g->name.c_str());
    }
    if (deleted) {
        flags = (flags & PARSE_FL_ROM_CONTINUED) ? 0 : PARSE_FL_ROM_DELETED;
        g->files[ft].pop_back();
    }
    else {
        flags = 0;
        if (!r[ft]->merge.empty() && r[ft]->merge == r[ft]->name) {
            r[ft]->merge = "";
        }
    }
}

Parser::Options::Options(std::optional<std::string> dat_name, bool use_configuration) {
    if (!use_configuration) {
        return;
    }

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