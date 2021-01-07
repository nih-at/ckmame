/*
  parse-rc.c -- parse Romcenter format files
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"


#define RC_SEP static_cast<char>(0xac)

enum section { RC_UNKNOWN = -1, RC_CREDITS, RC_DAT, RC_EMULATOR, RC_GAMES };

std::unordered_map<std::string, enum section> sections = {
    { "[CREDITS]", RC_CREDITS },
    { "[DAT]", RC_DAT },
    { "[EMULATOR]", RC_EMULATOR },
    { "[GAMES]", RC_GAMES },
    { "[RESOURCES]", RC_GAMES }
};

static bool parse_prog_description(ParserContext *ctx, const std::string &attr);
static bool parse_prog_name(ParserContext *ctx, const std::string &attr);
static bool parse_prog_version(ParserContext *ctx, const std::string &attr);
static bool rc_plugin(ParserContext *ctx, const std::string &attr);

struct {
    enum section section;
    std::string name;
    bool (*cb)(ParserContext *, const std::string &);
} fields[] = {
    { RC_CREDITS, "version", parse_prog_version },
    { RC_DAT, "plugin", rc_plugin },
    { RC_EMULATOR, "refname", parse_prog_name},
    { RC_EMULATOR, "version", parse_prog_description}
};

size_t nfields = sizeof(fields) / sizeof(fields[0]);

class RcTokenizer {
public:
    RcTokenizer(const std::string &s) : string(s), position(0) { }
    
    std::string get();
    
private:
    std::string string;
    size_t position;
};

class ROMLine {
public:
    ROMLine(ParserContext *ctx_) : ctx(ctx_) { }
    
    bool process(const std::string &line);
    void flush();

private:
    ParserContext *ctx;
    std::string gamename;
};


bool ParserContext::parse_rc() {
    lineno = 0;
    enum section sect = RC_UNKNOWN;

    auto romline = ROMLine(this);

    std::optional<std::string> l;
    while ((l = ps->getline()).has_value()) {
        lineno++;
        auto line = l.value();

        if (line[0] == '[') {
            auto it = sections.find(line);
            if (it != sections.end()) {
                sect = it->second;
            }
            else {
                sect = RC_UNKNOWN;
            }
	    continue;
	}

	if (sect == RC_GAMES) {
            if (!romline.process(line)) {
                myerror(ERRFILE, "%d: cannot parse ROM line, skipping", lineno);
            }
        }
        else {
            auto position = line.find('=');
            if (position == std::string::npos) {
                myerror(ERRFILE, "%d: no `=' found", lineno);
                continue;
            }
            auto key = line.substr(0, position);
            auto value = line.substr(position + 1);

            for (size_t i = 0; i < nfields; i++) {
                if (fields[i].section == sect && key == fields[i].name) {
                    fields[i].cb(this, value);
		    break;
		}
	    }
	}
    }

    return 0;
}

std::string RcTokenizer::get() {
    if (position == std::string::npos) {
        return "";
    }
    
    auto sep = string.find(RC_SEP, position);
    
    std::string field;
    
    if (sep == std::string::npos) {
        field = string.substr(position);
        position = std::string::npos;
    }
    else {
        field = string.substr(position, sep - position);
        position = sep + 1;
    }
    
    return field;
}


static bool parse_prog_description(ParserContext *ctx, const std::string &attr) {
    return ctx->prog_description(attr);
}

static bool parse_prog_name(ParserContext *ctx, const std::string &attr) {
    return ctx->prog_name(attr);
}

static bool parse_prog_version(ParserContext *ctx, const std::string &attr) {
    return ctx->prog_version(attr);
}

static bool
rc_plugin(ParserContext *ctx, const std::string &attr) {
    myerror(ERRFILE, "%d: warning: RomCenter plugins not supported,", ctx->lineno);
    myerror(ERRFILE, "%d: warning: DAT won't work as expected.", ctx->lineno);
    return false;
}


void ROMLine::flush() {
    if (!gamename.empty()) {
        ctx->game_end();
    }
    gamename = "";
}

bool ROMLine::process(const std::string &line) {
    auto tokenizer = RcTokenizer(line);

    if (!tokenizer.get().empty()) {
        return false;
    }

    auto parent = tokenizer.get();
    (void)tokenizer.get();
    auto name = tokenizer.get();
    auto desc = tokenizer.get();

    if (name.empty()) {
        return false;
    }

    if (gamename != name) {
        flush();

        gamename = name;
        ctx->game_start();
        ctx->game_name(name);
        if (!desc.empty()) {
            ctx->game_description(desc);
        }
        if (!parent.empty() && parent != name) {
            ctx->game_cloneof(TYPE_ROM, parent);
        }
    }

    auto token = tokenizer.get();
    if (token.empty()) {
        return false;
    }

    ctx->file_start(TYPE_ROM);
    ctx->file_name(TYPE_ROM, token);
    token = tokenizer.get();
    if (!token.empty()) {
        ctx->file_hash(TYPE_ROM, Hashes::TYPE_CRC, token);
    }
    token = tokenizer.get();
    if (!token.empty()) {
        ctx->file_size(TYPE_ROM, token);
    }
    (void)tokenizer.get();
    token = tokenizer.get();
    if (!token.empty()) {
        ctx->file_merge(TYPE_ROM, token);
    }
    ctx->file_end(TYPE_ROM);

    return true;
}
