
#ifndef HAD_COMMANDLINE_H
#define HAD_COMMANDLINE_H

/*
 Commandline.h -- parse command line options and arguments
 Copyright (C) 2021 Dieter Baron and Thomas Klausner

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

#include <optional>
#include <string>
#include <vector>

class Commandline {
public:
    
    class Option {
    public:
        Option(const std::string &name_, char short_name_, const std::string &argument_name_, const std::string &description_) : name(name_), short_name(short_name_), argument_name(argument_name_), description(description_) { }
        Option(const std::string &name_, const std::string &argument_name_, const std::string &description_) : name(name_), argument_name(argument_name_), description(description_) { }
        Option(const std::string &name_, char short_name_, const std::string &description_) : name(name_), short_name(short_name_), description(description_) { }
        Option(const std::string &name_, const std::string &description_) : name(name_), description(description_) { }

        
        std::string name;
        std::optional<char> short_name;
        std::string argument_name;
        std::string description;

        bool has_argument() const { return !argument_name.empty(); }
    };
    
    class OptionValue {
    public:
        OptionValue(const std::string &name_, const std::string &argument_) : name(name_), argument(argument_) { }
        
        std::string name;
        std::string argument; // empty string for options without argument
    };

    std::vector<OptionValue> options;
    std::vector<std::string> arguments;

    Commandline(const std::vector<Option> &options, int argc, char * const argv[]);
    
    std::optional<std::string> find_first(const std::string &name) const;
    std::optional<std::string> find_last(const std::string &name) const;
    
    static void usage(const std::vector<Option> &options, const std::string &arguments, bool full = false, FILE *fout = stdout);
};

#endif // HAD_COMMANDLINE_H
