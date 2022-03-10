/*
Command.cc --
Copyright (C) 2022 Dieter Baron and Thomas Klausner

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

#include "Command.h"

#include <fnmatch.h>

#include "config.h"

#include "Exception.h"
#include "globals.h"

Command::Command(std::string name, std::string arguments, std::vector<Commandline::Option> options, std::unordered_set<std::string> used_variables) :
    name(std::move(name)),
    arguments(std::move(arguments)),
    options(std::move(options)),
    used_variables(std::move(used_variables)) {
}


int Command::run(int argc, char* const* argv) {
    setprogname(argv[0]);

    auto command_name = name;
    auto version = std::string(PACKAGE " " VERSION);
    if (name != PACKAGE) {
	command_name += std::string("(") + PACKAGE + ")";
	version = name + "(" + version + ")";
    }

    auto commandline = Commandline(options, arguments, name + " by Dieter Baron and Thomas Klausner", "Report bugs to " PACKAGE_BUGREPORT ".", version + "\nCopyright (C) 1999-2022 Dieter Baron and Thomas Klausner\\n\" PACKAGE \" comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\\n\"");

    Configuration::add_options(commandline, used_variables);

    commandline.add_option(Commandline::Option("all-sets", "execute command once for each set"));

    int exit_code = 0;

    try {
	auto arguments = commandline.parse(argc, argv);

	configuration.handle_commandline(arguments); // global, not merging config, not setting set

	setup(arguments);

	std::set<std::string> selected_sets;

	if (arguments.find_first("all-sets")) {
	    selected_sets = configuration.sets;
	}
	else {
	    for (const auto &option : arguments.options) {
		if (option.name == "set") {
		    if (option.argument.find_first_of("?*[")) {
			auto matched = false;
			for (const auto& set : configuration.sets) {
			    if (fnmatch(option.argument.c_str(), set.c_str(), 0) == 0) {
				selected_sets.insert(set);
				matched = true;
			    }
			}
			if (!matched) {
			    throw Exception("no set matches '" + option.argument + "'");
			}
		    }
		    else {
			if (configuration.sets.find(option.argument) == configuration.sets.end()) {
			    throw Exception("unknown set '" + option.argument + "'");
			}
			selected_sets.insert(option.argument);
		    }
		}
	    }
	}

	if (selected_sets.empty()) {
	    if (!do_for("", arguments, false)) {
		exit_code = 1;
	    }
	}
	else {
	    auto multi_set = selected_sets.size() > 1;
	    for (const auto& set : selected_sets) {
		if (!do_for(set, arguments, multi_set)) {
		    exit_code = 1;
		}
	    }
	}
    }
    catch (std::exception &ex) {
	fprintf(stderr, "%s: %s\n", getprogname(), ex.what());
	// TODO: handle error
	exit_code = 1;
    }

    try {
	if (!cleanup()) {
	    exit_code = 1;
	}
    }
    catch (std::exception &ex) {
	fprintf(stderr, "%s: %s\n", getprogname(), ex.what());
	// TODO: handle error
	exit_code = 1;
    }

    return exit_code;
}


bool Command::do_for(const std::string& set, const ParsedCommandline& arguments, bool multi_set_invocation) {
    try {
	if (multi_set_invocation) {
	    output.set_header("Set " + set);
	}
	configuration.prepare(set, arguments);
	return execute(arguments.arguments);
    }
    catch (std::exception &ex) {
	fprintf(stderr, "%s: %s\n", getprogname(), ex.what());
	// TODO: handle error
	return false;
    }
}
