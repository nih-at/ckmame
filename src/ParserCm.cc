/*
  parse-cm.c -- parse listinfo/CMpro format files
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

#include "ParserCm.h"

#include "globals.h"


bool ParserCm::parse() {
    auto ok = true;
    lineno = 0;
    auto parse_state = TOP;

    std::optional<std::string> line;

    while ((line = ps->getline()).has_value()) {
        lineno++;

        auto tokenizer = Tokenizer(line.value());

        auto cmd = tokenizer.get();

        if (cmd.empty()) {
            continue;
        }

        ignoring_line = false;

        switch (parse_state) {
        case TOP:
            /* game/resource for MAME/Raine, machine for MESS */
            if (cmd == "game" || cmd == "machine" || cmd == "resource") {
                game_start();
                parse_state = GAME;
                auto brace = tokenizer.get();
                if (brace != "(") {
                    output.line_error(lineno, "expected '(', got '%s'", brace.c_str());
                    ok = false;
                    break;
                }
            }
            else if (cmd == "emulator" || cmd == "clrmamepro") {
                parse_state = PROG;
                auto brace = tokenizer.get();
                if (brace != "(") {
                    output.line_error(lineno, "expected '(', got '%s'", brace.c_str());
                    ok = false;
                    break;
                }
            }
            else if (cmd == "BEGIN" || cmd == "END") {
                /* TODO: beginning/end of file, ignored for now */
            }
            else {
                warn_unknown_keyword(cmd);
                ignoring_line = true;
            }
            break;

        case GAME:
            if (cmd == "name") {
                game_name(tokenizer.get());
            }
            else if (cmd == "description") {
                game_description(tokenizer.get());
            }
            else if (cmd == "romof") {
                game_cloneof(tokenizer.get());
            }
            else if (cmd == "sampleof" || cmd == "sourcefile") {
                /* skip value */
                tokenizer.get();
            }
            else if (cmd == "rom") {
                auto brace = tokenizer.get();
                if (brace != "(") {
                    output.line_error(lineno, "expected '(', got '%s'", brace.c_str());
                    ok = false;
                    break;
                }
                auto name = tokenizer.get();
                if (name != "name") {
                    output.line_error(lineno, "expected 'name', got '%s'", name.c_str());
                    ok = false;
                    break;
                }
                file_start(TYPE_ROM);
                file_name(TYPE_ROM, tokenizer.get());

                /* read remaining tokens and look for known tokens */
                std::string token;
                while (!(token = tokenizer.get()).empty()) {
                    if (token == "baddump" || token == "nodump") {
                        if (!file_status(TYPE_ROM, token)) {
                            continue;
                        }
                    }
                    else if (token == "crc" || token == "crc32") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token crc missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_hash(TYPE_ROM, Hashes::TYPE_CRC, token)) {
                            continue;
                        }
                    }
                    else if (token == "flags") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token flags missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_status(TYPE_ROM, token)) {
                            continue;
                        }
                    }
                    else if (token == "merge") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token merge missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_merge(TYPE_ROM, token)) {
                            continue;
                        }
                    }
                    else if (token == "md5") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token md5 missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_hash(TYPE_ROM, Hashes::TYPE_MD5, token)) {
                            continue;
                        }
                    }
                    else if (token == "sha1") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token sha1 missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_hash(TYPE_ROM, Hashes::TYPE_SHA1, token)) {
                            continue;
                        }
                    }
                    else if (token == "size") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token size missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_size(TYPE_ROM, token)) {
                            continue;
                        }
                    }
                    else if (token == ")") {
                        break;
                    }
                    else {
                        output.line_error(lineno, "warning: ignoring unknown token '%s'", token.c_str());
                    }
                }

                file_end(TYPE_ROM);
            }
            else if (cmd == "disk") {
                auto brace = tokenizer.get();
                if (brace != "(") {
                    output.line_error(lineno, "expected '(', got '%s'", brace.c_str());
                    ok = false;
                    break;
                }
                auto name = tokenizer.get();
                //                    if (tokenizer.get() != "name") {
                if (name != "name") {
                    output.line_error(lineno, "expected token 'name' not found ('%s', '%s')", brace.c_str(), name.c_str());
                    ok = false;
                    break;
                }

                file_start(TYPE_DISK);
                file_name(TYPE_DISK, tokenizer.get());

                /* read remaining tokens and look for known tokens */
                std::string token;
                while (!(token = tokenizer.get()).empty()) {
                    if (token == "sha1") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token sha1 missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_hash(TYPE_DISK, Hashes::TYPE_SHA1, token)) {
                            continue;
                        }
                    }
                    else if (token == "md5") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token md5 missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_hash(TYPE_DISK, Hashes::TYPE_MD5, token)) {
                            continue;
                        }
                    }
                    else if (token == "merge") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token merge missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_merge(TYPE_DISK, token)) {
                            continue;
                        }
                    }
                    else if (token == "flags") {
                        if ((token = tokenizer.get()).empty()) {
                            output.line_error(lineno, "token flags missing argument");
                            ok = false;
                            continue;
                        }
                        if (!file_status(TYPE_DISK, token)) {
                            continue;
                        }
                    }
                    else if (token == ")") {
                        break;
                    }
                    else {
                        warn_unknown_keyword(token);
                    }
                }
                file_end(TYPE_DISK);
            }
            else if (cmd == "archive") {
                /* TODO: archive names */
            }
            else if (cmd == "sample") {
                /* skip value */
                tokenizer.get();
            }
            else if (cmd == ")") {
                game_end();
                parse_state = TOP;
            }
            else if (cmd == "manufacturer" || cmd == "year") {
                /* skip value */
                tokenizer.get();
            }
            else {
                warn_unknown_keyword(cmd);
            }
            break;

        case PROG:
            if (cmd == "name") {
                prog_name(tokenizer.get());
            }
            else if (cmd == "description") {
                prog_description(tokenizer.get());
            }
            else if (cmd == "version") {
                prog_version(tokenizer.get());
            }
            else if (cmd == "header") {
                prog_header(tokenizer.get());
            }
            else if (cmd == ")") {
                // TODO: this shouldn't be necessary
                header_end();
                parse_state = TOP;
                if (header_only) {
                    return true;
                }
            }
            else if (cmd == "author" || cmd == "category" || cmd == "comment" || cmd == "date" ||
                     cmd == "forcemerging" || cmd == "forcenodump" || cmd == "forcepacking") {
                /* skip value */
                tokenizer.get();
            }
            else {
                warn_unknown_keyword(cmd);
            }
            break;
        }
        if (!ignoring_line) {
            std::string leftover = tokenizer.get();
            while (!leftover.empty()) {
                output.line_error(lineno, "ignoring unknown token '%s'", leftover.c_str());
                leftover = tokenizer.get();
            }
        }
    }

    return ok;
}


std::string ParserCm::Tokenizer::get() {
    if (position == std::string::npos) {
        return "";
    }

    auto s = string.find_first_not_of(" \t", position);
    if (s == std::string::npos) {
        position = std::string::npos;
        return "";
    }

    size_t e;

    switch (string[s]) {
    case '\0':
    case '\n':
    case '\r':
        position = std::string::npos;
        return "";

    case '\"':
        s++;
        e = string.find_first_of("\\\"", s);
        if (e == std::string::npos) {
            // TODO: treat missing closing quote as error?
            break;
        }
        if (string[e] == '\\') {
            std::string token = string.substr(s, e - s);

            while (string[e] == '\\') {
                switch (string[e + 1]) {
                case '\0':
                    // TODO: treat trailing \\ as error?
                    token += '\\';
                    position = std::string::npos;
                    return token;

                    // TODO: other C style escapes like \n?

                default:
                    token += string[e + 1];
                }

                e += 2;

                auto next = string.find_first_of("\\\"", e);
                if (next == std::string::npos) {
                    // TODO: treat missing closing " as error?
                    position = next;
                    token += string.substr(e);
                    return token;
                }
                token += string.substr(e, next - e);
                e = next;
            }
            position = e + 1;
            return token;
        }
        break;

    default:
        e = string.find_first_of(" \t\n\r", s);
        break;
    }

    if (e == std::string::npos) {
        e = string.size();
    }
    position = e;
    if (string[position] != '\0') {
        position++;
    }

    return string.substr(s, e - s);
}

void ParserCm::warn_unknown_keyword(const std::string& keyword) {
    if (warned_keywords.find(keyword) == warned_keywords.end()) {
        output.line_error(lineno, "unexpected token '%s'", keyword.c_str());
        warned_keywords.insert(keyword);
    }
    ignoring_line = true;
}
