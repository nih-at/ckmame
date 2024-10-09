/*
Command.h --
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

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <unordered_set>

#include "Commandline.h"

class Command {
  public:
    Command(std::string name, std::string arguments, std::vector<Commandline::Option> options, std::unordered_set<std::string> used_variables);
    virtual ~Command() = default;

    int run(int argc, char *const *argv);

  protected:
    virtual void global_setup(const ParsedCommandline& commandline) { }
    virtual bool setup() { return true; }
    virtual bool execute(const std::vector<std::string>& arguments) = 0;
    virtual bool cleanup() { return true; }
    virtual bool global_cleanup() { return true; }

    std::string name;
    std::string arguments;
    std::vector<Commandline::Option> options;
    std::unordered_set<std::string> used_variables;

  private:
    bool do_for(const std::string& set, const ParsedCommandline& arguments, bool multi_set_invocation = false);
};


#endif // COMMAND_H
