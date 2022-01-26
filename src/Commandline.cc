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

#include <algorithm>
#include <cctype>
#include <sstream>
#include <strings.h>
#include <unordered_map>

#include "compat.h"

#include "Exception.h"

extern int optind;

Commandline::Commandline(const std::vector<Option> &options_, const std::string &arguments_, const std::string &header_, const std::string &footer_) : options(options_), arguments(arguments_), header(header_), footer(footer_) {
    std::sort(options.begin(), options.end());
}


ParsedCommandline Commandline::parse(int argc, char *const *argv) {
    std::string short_options;
    std::vector<struct option> long_options;
    std::unordered_map<int, size_t> option_indices;
    int next_index = 256;
    
    for (const auto &option : options) {
//#define DEBUG_OPTIONS
#ifdef DEBUG_OPTIONS
        printf("option '%s'", option.name.c_str());
        if (option.short_name.has_value()) {
            printf("/'%c'", option.short_name.value());
        }
        if (option.has_argument()) {
            printf(", argument '%s'", option.argument_name.c_str());
        }
        printf(", description '%s'\n", option.description.c_str());
#endif
        int index;
        if (option.short_name.has_value()) {
            index = option.short_name.value();
            short_options += option.short_name.value();
            if (option.has_argument()) {
                short_options += ":";
            }
        }
        else {
            index = next_index++;
        }
        
        option_indices[index] = long_options.size();
        struct option long_option = {
            option.name.c_str(), option.has_argument() ? 1 : 0, 0, index
        };
        long_options.push_back(long_option);
    }
    
#ifdef DEBUG_OPTIONS
    printf("short options: '%s'\n", short_options.c_str());
#endif
    struct option terminator = { nullptr, 0, 0, 0 };
    long_options.push_back(terminator);
        
    auto parsed_commandline = ParsedCommandline();
    opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, short_options.c_str(), long_options.data(), nullptr)) != EOF) {
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
        const auto &option = long_options[it->second];
        
        parsed_commandline.options.push_back(ParsedCommandline::OptionValue(option.name, option.has_arg ? optarg : ""));
    }
    
    for (auto i = optind; i < argc; i++) {
        parsed_commandline.arguments.push_back(argv[i]);
    }
    
    return parsed_commandline;
}


std::optional<std::string> ParsedCommandline::find_first(const std::string &name) const {
    for (const auto &option : options) {
        if (option.name == name) {
            return option.argument;
        }
    }
    
    return {};
}

std::optional<std::string> ParsedCommandline::find_last(const std::string &name) const {
    for (auto it = options.rbegin(); it != options.rend(); it++) {
        const auto &option = *it;
        
        if (option.name == name) {
            return option.argument;
        }
    }
    
    return {};
}


void Commandline::usage(bool full, FILE *fout) {
    std::stringstream short_options_without_argument;
    std::stringstream short_options_with_argument;

    for (const auto &option : options) {
        if (option.short_name.has_value()) {
            if (option.has_argument()) {
                short_options_with_argument << " [-" << option.short_name.value() << " " << option.argument_name << "]";
            }
            else {
                short_options_without_argument << option.short_name.value();
            }
        }
    }
    
    if (full) {
        fprintf(fout, "%s\n\n", header.c_str());
    }

    fprintf(fout, "Usage: %s", getprogname());
    if (!short_options_without_argument.str().empty()) {
        fprintf(fout, " [-%s]", short_options_without_argument.str().c_str());
    }
    if (!short_options_with_argument.str().empty()) {
        fprintf(fout, "%s", short_options_with_argument.str().c_str());
    }
    if (!arguments.empty()) {
        fprintf(fout, " %s", arguments.c_str());
    }
    fprintf(fout, "\n");

    if (full) {
        fprintf(fout, "\n");

        size_t max_length = 0;
        for (const auto &option : options) {
            size_t length = 8 + option.name.length();
            if (option.has_argument()) {
                length += option.argument_name.length() + 1;
            }
            if (length > max_length) {
                max_length = length;
            }
        }
        for (const auto &option : options) {
            if (option.short_name.has_value()) {
                printf("  -%c, ", option.short_name.value());
            }
            else {
                printf("      ");
            }
            size_t length = 8 + option.name.length();
            printf("--%s", option.name.c_str());
            if (option.has_argument()) {
                printf(" %s", option.argument_name.c_str());
                length += option.argument_name.length() + 1;
            }
            while (length < max_length) {
                length += 1;
                fputc(' ', fout);
            }
            fprintf(fout, "  %s\n", option.description.c_str());
        }
        
        fprintf(fout, "\n%s\n", footer.c_str());
    }
}

static int compare_char(char a, char b) {
    auto cmp = std::tolower(a) - std::tolower(b);
    
    if (cmp != 0) {
        return cmp;
    }
    else {
        return a - b;
    }
}


bool Commandline::Option::operator<(const Option &other) const {
    if (short_name.has_value()) {
        if (other.short_name.has_value()) {
            return compare_char(short_name.value(), other.short_name.value()) < 0;
        }
        else {
            return compare_char(short_name.value(), other.name[0]) <= 0;
        }
    }
    else {
        if (other.short_name.has_value()) {
            return compare_char(name[0], other.short_name.value());
        }
        else {
            return strcasecmp(name.c_str(), other.name.c_str()) < 0;
        }
    }
}
