/*
Command.h --
Copyright (C) 2022-2024 Dieter Baron and Thomas Klausner

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

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <unordered_set>

#include "Commandline.h"

/**
 * Base class for classes implementing a command that can be run from the command line.
 */
class Command {
  public:
    /**
     * Create a new command.
     * 
     * @param name the name of the command
     * @param arguments a description of the non-option arguments accepted by the command
     * @param options command line options accepted by the command
     * @param used_variables the configuration variables used by the command, used for adding corresponding command line options
     */
    Command(std::string name, std::string arguments, std::vector<Commandline::Option> options,
            std::unordered_set<std::string> used_variables);
    virtual ~Command() = default;

    /**
     * Run the command with the given command line arguments.
     * 
     * @param argc the number of command line arguments
     * @param argv the command line arguments, including the program name as `argv[0]`
     * @return the exit code of the command, `0` for success and non-zero for failure
     */
    int run(int argc, char* const* argv);

  protected:
    /**
     * Perform global setup for the command.
     * 
     * This is called once per run before `setup()` is called for any set.
     * 
     * @param commandline the parsed command line arguments
     */
    virtual void global_setup(const ParsedCommandline& commandline) {}

    /**
     * Perform setup for a specific set.
     * 
     * This is called once per set, or once if no sets are specified.
     */
    virtual bool setup() { return true; }

    /**
     * Execute the command for a specific set.
     * 
     * This is called once per set, after `setup()` has been called.
     */
    virtual bool execute(const std::vector<std::string>& arguments) = 0;

    /**
     * Perform cleanup after executing for a specific set.
     * 
     * This is called once per set, after `execute()` has been called.
     */
    virtual bool cleanup() { return true; }

    /**
     * Perform global cleanup after executing for all sets.
     * 
     * This is called once per run after all sets have been processed.
     */
    virtual bool global_cleanup() { return true; }

    /// The name of the command.
    std::string name;

    /// The non-option arguments accepted by the command.
    std::string arguments;

    /// The command line options accepted by the command.
    std::vector<Commandline::Option> options;

    /// The configuration variables used by the command, used for adding corresponding command line options.
    std::unordered_set<std::string> used_variables;

  private:
    /**
     * Execute the command for a specific set, including setup and cleanup.
      * 
      * @param set the name of the set to execute for, or an empty string if no set is specified
      * @param arguments the parsed command line arguments
      * @param multi_set_invocation whether multiple sets are being processed in this run.
      * @return `true` if the command executed successfully, `false` otherwise
     */
    bool do_for(const std::string& set, const ParsedCommandline& arguments, bool multi_set_invocation = false);
};


#endif // COMMAND_H
