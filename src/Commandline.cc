/*
  Commandline.cc -- parse command line options and arguments
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
#include <iostream>
#include <unordered_map>
#include <utility>

Commandline::Commandline(std::vector<Option> options_, std::string arguments_, std::string header_, std::string footer_, std::string version_) : options(std::move(options_)), arguments(std::move(arguments_)), header(std::move(header_)), footer(std::move(footer_)), version(std::move(version_)), options_sorted(false) {
    add_option(Option("help", 'h', "display this help message"));
    add_option(Option("version", 'V', "display version integer"));
}


ParsedCommandline Commandline::parse(int argc, char *const *argv) {
    program_name = argv[0];

    std::unordered_map<char, const Option*> short_options;
    std::unordered_map<std::string, const Option*> long_options;

    for (const auto& option: options) {
        if (option.short_name) {
            short_options[*option.short_name] = &option;
        }
        long_options[option.name] = &option;
    }

    auto parsed_commandline = ParsedCommandline();

    auto in_options = true;

    for (int index = 1; index < argc; index += 1) {
        auto argument = std::string(argv[index]);

        if (in_options) {
            if (argument.size() >=2 && argument[0] == '-' && argument[1] == '-' /* argument.starts_with("--") */) {
                auto equals = argument.find('=');
                auto option_name = argument.substr(2, equals == std::string::npos ? equals : equals - 2);
                auto it = long_options.find(option_name);
                if (it == long_options.end()) {
                    usage(false, stderr);
                    std::cerr << "unknown option '--" << option_name << "'\n";
                    exit(1);
                }
                auto option_argument = std::string{};
                if (equals != std::string::npos) {
                    if (it->second->has_argument()) {
                        parsed_commandline.add_option(option_name, argument.substr(equals + 1));
                    }
                    else {
                        usage(false, stderr);
                        std::cerr << "option '--" << option_name << "' doesn't take an argument\n";
                        exit(1);
                    }
                }
                else {
                    if (it->second->has_argument()) {
                        if (index == argc - 1) {
                            usage(false, stderr);
                            std::cerr << "missing argument for option '--" << option_name << "'\n";
                            exit(1);
                        }
                        index += 1;
                        parsed_commandline.add_option(option_name, argv[index]);
                    }
                    else {
                        parsed_commandline.add_option(option_name, "");
                    }
                }
            }
            else if (argument.size() >= 1 && argument[0] == '-' /* argument.starts_with('-') */) {
                for (size_t position = 1; position < argument.size(); position += 1) {
                    auto option_name = argument[position];
                    auto it = short_options.find(option_name);
                    if (it == short_options.end()) {
                        usage(false, stderr);
                        std::cerr << "unknown option '-" << option_name << "'\n";
                        exit(1);
                    }
                    if (it->second->has_argument()) {
                        position += 1;
                        if (position < argument.size()) {
                            parsed_commandline.add_option(it->second->name, argument.substr(position));
                        }
                        else {
                            if (index == argc - 1) {
                                usage(false, stderr);
                                std::cerr << "missing argument for option '-" << option_name << "'\n";
                                exit(1);
                            }
                            index += 1;
                            parsed_commandline.add_option(it->second->name, argv[index]);
                        }
                        break;
                    }
                    else {
                        parsed_commandline.add_option(it->second->name, "");
                    }
                }
            }
            else {
                in_options = false;
            }
        }

        if (!in_options) {
            parsed_commandline.arguments.emplace_back(argument);
        }
    }

    if (parsed_commandline.find_first("help").has_value()) {
        usage(true);
        exit(0);
    }
    if (parsed_commandline.find_first("version").has_value()) {
        std::cout << version << std::endl;
        exit(0);
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

[[maybe_unused]] std::optional<std::string> ParsedCommandline::find_last(const std::string &name) const {
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

    sort_options();

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

    fprintf(fout, "Usage: %s", program_name.c_str());
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
                fprintf(fout, "  -%c, ", option.short_name.value());
            }
            else {
                fprintf(fout, "      ");
            }
            size_t length = 8 + option.name.length();
            fprintf(fout, "--%s", option.name.c_str());
            if (option.has_argument()) {
                fprintf(fout, " %s", option.argument_name.c_str());
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


void Commandline::add_option(Commandline::Option option) {
    options.push_back(std::move(option));
}


void Commandline::sort_options() {
    if (!options_sorted) {
    std::sort(options.begin(), options.end());
    options_sorted = true;
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
            return compare_char(name[0], other.short_name.value()) < 0;
        }
        else {
            return std::lexicographical_compare(name.begin(), name.end(), other.name.begin(), other.name.end());
        }
    }
}
