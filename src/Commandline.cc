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

#include "Commandline.h"

#include <unordered_map>

#include "compat.h"

#include "Exception.h"

extern int optind;

Commandline::Commandline(const std::vector<Option> &defined_options, int argc, char * const argv[]) {
    std::string short_options;
    std::vector<struct option> long_options;
    std::unordered_map<int, size_t> option_indices;
    int next_index = 256;
    
    for (auto const &option : defined_options) {
        int index;
        if (option.short_name.has_value()) {
            index = option.short_name.value();
            short_options += option.short_name.value();
            if (option.has_argument) {
                short_options += ":";
            }
        }
        else {
            index = next_index++;
        }
        
        option_indices[index] = long_options.size();
        struct option long_option = {
            option.name.c_str(), option.has_argument ? 1 : 0, 0, index
        };
        long_options.push_back(long_option);
    }
    
    struct option terminator = { NULL, 0, 0, 0 };
    long_options.push_back(terminator);
        
    opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, short_options.c_str(), long_options.data(), NULL)) != EOF) {
        if (c == '?') {
            throw Exception("invalid option");
        }
        if (c == ':') {
            throw Exception("option missing argument");
        }
        
        auto it = option_indices.find(c);
        if (it == option_indices.end()) {
            throw Exception("invalid option");
        }
        auto const &option = long_options[it->second];
        
        options.push_back(OptionValue(option.name, option.has_arg ? optarg : ""));
    }
    
    for (auto i = optind; i < argc; i++) {
        arguments.push_back(argv[i]);
    }
}


std::optional<std::string> Commandline::find_first(const std::string &name) const {
    for (auto const &option : options) {
        if (option.name == name) {
            return option.argument;
        }
    }
    
    return {};
}

std::optional<std::string> Commandline::find_last(const std::string &name) const {
    for (auto it = options.rbegin(); it != options.rend(); it++) {
        auto const &option = *it;
        
        if (option.name == name) {
            return option.argument;
        }
    }
    
    return {};
}
