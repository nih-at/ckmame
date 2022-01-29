/*
  parse.c -- parser frontend
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
#include <utility>

#include "ParserCm.h"
#include "ParserRc.h"
#include "ParserXml.h"
#include "error.h"


#define CHECK_STATE(s)                                        \
    do {                                                           \
	if (state != (s)) {                                 \
	    myerror(ERRFILE, "%zu: in wrong state", lineno); \
	    return false;                                             \
	}                                                          \
    } while (0)


bool Parser::parse(const ParserSourcePtr& source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out, int flags) {
    std::unique_ptr<Parser> parser;

    auto c = source->peek();

    switch (c) {
        case '<':
            parser = std::unique_ptr<Parser>(new ParserXml(source, exclude, dat, out, flags));
            break;
        case '[':
            parser = std::unique_ptr<Parser>(new ParserRc(source, exclude, dat, out, flags));
            break;
        default:
            parser = std::unique_ptr<Parser>(new ParserCm(source, exclude, dat, out, flags));
    }

    auto ok = parser->parse();
    if (ok) {
        if (!parser->eof()) {
            myerror(ERRFILE, "trailing garbage");
            ok = false;
	}
    }

    return ok;
}


bool Parser::eof() {
    if (state == PARSE_IN_HEADER) {
        return header_end();
    }

    return true;
}


bool Parser::file_continue(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
	myerror(ERRFILE, "%zu: file continuation only supported for ROMs", lineno);
	return false;
    }

    if (flags & PARSE_FL_ROM_DELETED) {
        myerror(ERRFILE, "%zu: internal error: trying to continue deleted file", lineno);
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


bool Parser::file_status(filetype_t ft, const std::string &attr) {
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
        myerror(ERRFILE, "%zu: illegal status '%s'", lineno, attr.c_str());
	return false;
    }
 
    r[ft]->status = status;

    return true;
}


bool Parser::file_hash(filetype_t ft, int ht, const std::string &attr) {
    Hashes *h;

    CHECK_STATE(PARSE_IN_FILE);

    if (attr == "-") {
	/* some dat files record crc - for 0-byte files, so skip it */
	return true;
    }

    h = &r[ft]->hashes;

    if (h->set_from_string(attr) != ht) {
	myerror(ERRFILE, "%zu: invalid argument for %s", lineno, Hashes::type_name(ht).c_str());
	return false;
    }

    return true;
}


bool Parser::file_ignore(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
        myerror(ERRFILE, "%zu: file ignoring only supported for ROMs", lineno);
	return false;
    }

    flags |= PARSE_FL_ROM_IGNORE;

    return true;
}


bool Parser::file_merge(filetype_t ft, const std::string &attr) {
    CHECK_STATE(PARSE_IN_FILE);

    r[ft]->merge = attr;

    return true;
}


bool Parser::file_mtime(filetype_t ft, time_t mtime) {
    CHECK_STATE(PARSE_IN_FILE);
    
    r[ft]->mtime = mtime;

    return true;
}


bool Parser::file_name(filetype_t ft, const std::string &attr) {
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


bool Parser::file_size(filetype_t ft, const std::string &attr) {
    /* TODO: check for strol errors */
    return file_size(ft, std::stoull(attr, nullptr, 0));
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


bool
Parser::game_cloneof(const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->cloneof[0] = attr;

    return true;
}


bool Parser::game_description(const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->description = attr;

    return true;
}

bool Parser::game_end() {
    CHECK_STATE(PARSE_IN_GAME);
    
    auto ok = true;

    if (g->name.empty()) {
        myerror(ERRFILE, "%zu: game without name", lineno);
	ok = false;
    }
    else if (!ignore_game(g->name)) {
	/* omit description if same as name (to save space) */
        if (g->name == g->description) {
            g->description = "";
	}

        if (!g->cloneof[0].empty()) {
	    if (g->cloneof[0] == g->name) {
                g->cloneof[0] = "";
	    }
	}

        ok = output->game(g);
    }

    g = nullptr;

    state = PARSE_OUTSIDE;

    return ok;
}


bool Parser::game_name(const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    if (!g->name.empty()) {
	myerror(ERRFILE, "%zu: game has two 'name' tokens", lineno);
	return false;
    }

    g->name = attr;

    if (!full_archive_name) {
	/* slashes are directory separators on some systems, and at least
	 * one redump dat contained a slash in a rom name */
        std::replace(g->name.begin(), g->name.end(), '/', '-');
    }

    return true;
}


bool Parser::game_start() {
    if (state == PARSE_IN_HEADER) {
        if (!header_end()) {
            return false;
	}
    }

    CHECK_STATE(PARSE_OUTSIDE);

    if (g) {
	myerror(ERRFILE, "%zu: game inside game", lineno);
	return false;
    }

    g = std::make_shared<Game>();

    state = PARSE_IN_GAME;

    return true;
}


bool Parser::prog_description(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.description = attr;

    return true;
}


bool Parser::prog_header(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    if (detector) {
	myerror(ERRFILE, "%zu: warning: detector already defined, header '%s' ignored", lineno, attr.c_str());
	return true;
    }

    auto ok = true;
    
    ParserSourcePtr dps = ps->open(attr);
    if (!dps) {
        myerror(ERRFILESTR, "%zu: cannot open detector '%s'", lineno, attr.c_str());
	return false;
    }
#if defined(HAVE_LIBXML2)
    detector = Detector::parse(dps.get());
    if (!detector) {
        myerror(ERRFILE, "%zu: cannot parse detector '%s'", lineno, attr.c_str());
        ok = false;
    }
    else
#endif
    {
        if (!output->detector(detector.get())) {
            ok = false;
        }
    }

    return ok;
}


bool Parser::prog_name(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.name = attr;

    return true;
}


bool Parser::prog_version(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    de.version = attr;

    return true;
}


Parser::Parser(ParserSourcePtr source, std::unordered_set<std::string> exclude, const DatEntry *dat, OutputContext *output_, int flags) : lineno(0), header_only(false), ignore(std::move(exclude)), output(output_), ps(std::move(source)), flags(0), state(PARSE_IN_HEADER) {
    dat_default.merge(dat, nullptr);
    full_archive_name = flags & PARSER_FL_FULL_ARCHIVE_NAME;
    for (auto & i : r) {
        i = nullptr;
    }
}


bool Parser::header_end() {
    CHECK_STATE(PARSE_IN_HEADER);

    de.merge(&dat_default, &de);
    output->header(&de);

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


bool Parser::ignore_game(const std::string &name) {
    return std::any_of(ignore.begin(), ignore.end(), [&name](const std::string &pattern) {
	return fnmatch(pattern.c_str(), name.c_str(), 0) == 0;
    });
}


void Parser::rom_end(filetype_t ft) {
    size_t n = g->files[ft].size() - 1;

    if (r[ft]->hashes.size == 0) {
        /* some dats don't record crc for 0-byte files, so set it here */
        if (!r[ft]->hashes.has_type(Hashes::TYPE_CRC)) {
            r[ft]->hashes.set_crc(0);
        }
        
        /* some dats record all-zeroes md5 and sha1 for 0 byte files, fix */
        if (r[ft]->hashes.has_type(Hashes::TYPE_MD5) && r[ft]->hashes.is_zero(Hashes::TYPE_MD5)) {
            r[ft]->hashes.set_md5(reinterpret_cast<const unsigned char *>("\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e"));
        }
        if (r[ft]->hashes.has_type(Hashes::TYPE_SHA1) && r[ft]->hashes.is_zero(Hashes::TYPE_SHA1)) {
            r[ft]->hashes.set_sha1(reinterpret_cast<const unsigned char *>("\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09"));
        }
    }

    /* omit duplicates */
    auto deleted = false;

    if (flags & PARSE_FL_ROM_IGNORE) {
        deleted = true;
    }
    else if (flags & PARSE_FL_ROM_CONTINUED) {
        auto &rom2 = g->files[ft][n - 1];
        rom2.hashes.size += r[ft]->hashes.size;
	deleted = true;
    }
    else if (r[ft]->name.empty()) {
	myerror(ERRFILE, "%zu: ROM without name", lineno);
	deleted = true;
    }
    for (size_t j = 0; j < n && !deleted; j++) {
        auto &rom2 = g->files[ft][j];
        if (r[ft]->compare_name(rom2)) {
            if (r[ft]->compare_size_hashes(rom2)) {
                /* TODO: merge in additional hash types? */
                deleted = true;
                break;
	    }
            else {
                myerror(ERRFILE, "%zu: two different roms with same name (%s)", lineno, r[ft]->name.c_str());
                deleted = true;
                break;
            }
	}
    }
    if (!r[ft]->merge.empty() && g->cloneof[0].empty()) {
        myerror(ERRFILE, "%zu: rom '%s' has merge information but game '%s' has no parent", lineno, r[ft]->name.c_str(), g->name.c_str());
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
