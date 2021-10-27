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

#include "ParserRc.h"

#include "error.h"

const char ParserRc::separator = static_cast<char>(0xac);

std::unordered_map<std::string, ParserRc::Section> ParserRc::sections = {
    { "[CREDITS]", RC_CREDITS },
    { "[DAT]", RC_DAT },
    { "[EMULATOR]", RC_EMULATOR },
    { "[GAMES]", RC_GAMES },
    { "[RESOURCES]", RC_GAMES }
};

std::vector<ParserRc::Field> ParserRc::fields = {
    Field(RC_CREDITS, "version", parse_prog_version),
    Field(RC_DAT, "plugin", rc_plugin),
    Field(RC_EMULATOR, "refname", parse_prog_name),
    Field(RC_EMULATOR, "version", parse_prog_description)
};


bool ParserRc::parse() {
    lineno = 0;
    auto sect = RC_UNKNOWN;

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
            if (!process_romline(line)) {
                myerror(ERRFILE, "%zu: cannot parse ROM line, skipping", lineno);
            }
        }
        else {
            auto position = line.find('=');
            if (position == std::string::npos) {
                myerror(ERRFILE, "%zu: no `=' found", lineno);
                continue;
            }
            auto key = line.substr(0, position);
            auto value = line.substr(position + 1);

            for (auto &field : fields) {
                if (field.section == sect && key == field.name) {
                    field.cb(this, value);
		    break;
		}
	    }
	}
    }

    return true;
}

std::string ParserRc::Tokenizer::get() {
    if (position == std::string::npos) {
        return "";
    }
    
    auto sep = string.find(separator, position);
    
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


bool ParserRc::parse_prog_description(ParserRc *ctx, const std::string &attr) {
    return ctx->prog_description(attr);
}

bool ParserRc::parse_prog_name(ParserRc *ctx, const std::string &attr) {
    return ctx->prog_name(attr);
}

bool ParserRc::parse_prog_version(ParserRc *ctx, const std::string &attr) {
    return ctx->prog_version(attr);
}

bool ParserRc::rc_plugin(ParserRc *ctx, const std::string &attr) {
    myerror(ERRFILE, "%zu: warning: RomCenter plugins not supported,", ctx->lineno);
    myerror(ERRFILE, "%zu: warning: DAT won't work as expected.", ctx->lineno);
    return false;
}


void ParserRc::flush_romline() {
    if (!gamename.empty()) {
        game_end();
    }
    gamename = "";
}

bool ParserRc::process_romline(const std::string &line) {
    auto tokenizer = Tokenizer(line);

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
        flush_romline();

        gamename = name;
        game_start();
        game_name(name);
        if (!desc.empty()) {
            game_description(desc);
        }
        if (!parent.empty() && parent != name) {
            game_cloneof(TYPE_ROM, parent);
        }
    }

    auto token = tokenizer.get();
    if (token.empty()) {
        return false;
    }

    file_start(TYPE_ROM);
    file_name(TYPE_ROM, token);
    token = tokenizer.get();
    if (!token.empty()) {
        file_hash(TYPE_ROM, Hashes::TYPE_CRC, token);
    }
    token = tokenizer.get();
    if (!token.empty()) {
        file_size(TYPE_ROM, token);
    }
    (void)tokenizer.get();
    token = tokenizer.get();
    if (!token.empty()) {
        file_merge(TYPE_ROM, token);
    }
    file_end(TYPE_ROM);

    return true;
}
