#ifndef HAD_PARSER_RC_H
#define HAD_PARSER_RC_H

/*
  ParserRc.h -- parser for RomCenter dat files
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

#include <utility>

#include "Parser.h"

class ParserRc : public Parser {
  public:
    ParserRc(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *output, Options options) : Parser(std::move(source), exclude, dat, output, std::move(options)) {}
    ~ParserRc() override = default;

    bool parse() override;

  private:
    static const char separator;

    enum Section { RC_UNKNOWN = -1, RC_CREDITS, RC_DAT, RC_EMULATOR, RC_GAMES };

    class Field {
      public:
        Field(Section section_, const std::string &name_, bool (*cb_)(ParserRc *, const std::string &)) : section(section_), name(name_), cb(cb_) {}

        Section section;
        std::string name;
        bool (*cb)(ParserRc *, const std::string &);
    };

    static std::unordered_map<std::string, Section> sections;
    static std::vector<Field> fields;

    static bool parse_prog_description(ParserRc *ctx, const std::string &attr);
    static bool parse_prog_name(ParserRc *ctx, const std::string &attr);
    static bool parse_prog_version(ParserRc *ctx, const std::string &attr);
    static bool rc_plugin(ParserRc *ctx, const std::string &attr);

    class Tokenizer {
      public:
        explicit Tokenizer(const std::string &s) : string(s), position(0) {}

        std::string get();

      private:
        std::string string;
        size_t position;
    };

    bool process_romline(const std::string &line);
    void flush_romline();

    std::string gamename;
};

#endif // HAD_PARSER_RC_H
