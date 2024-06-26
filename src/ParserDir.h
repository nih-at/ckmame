#ifndef HAD_PARSER_DIR_H
#define HAD_PARSER_DIR_H

/*
  ParserDir.h -- parser for files in directories
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

#include "Archive.h"
#include "Parser.h"

class ParserDir : public Parser {
public:
    ParserDir(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *output, const Options& options, std::string dname, int hashtypes_, bool runtest_ = false) : Parser(std::move(source), exclude, dat, output, options), directory_name(std::move(dname)), hashtypes(hashtypes_), runtest(runtest_) { }
    ~ParserDir() override = default;
    
    bool parse() override;
        
private:
    bool parse_archive(filetype_t filetype, Archive *a);
    void end_game();
    void start_game(const std::string &name, const std::string &top_directory);

    std::string directory_name;
    int hashtypes;
    bool runtest;
    
    std::string current_game;
};

#endif // HAD_PARSER_DIR_H
