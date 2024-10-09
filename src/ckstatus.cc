/*
  ckstatus.cc -- print info about last runs (from data base)
  Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner

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

#include "CkStatus.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include "compat.h"

#include "Commandline.h"
#include "Exception.h"
#include "RomDB.h"
#include "Stats.h"
#include "globals.h"
#include "util.h"

std::vector<Commandline::Option> ckstatus_options = {Commandline::Option("runs", "list runs")};

std::unordered_set<std::string> ckstatus_used_variables = {"status_db"};

int main(int argc, char** argv) {
    auto command = CkStatus();

    return command.run(argc, argv);
}


CkStatus::CkStatus() : Command("ckstatus", "", ckstatus_options, ckstatus_used_variables) {}

void CkStatus::global_setup(const ParsedCommandline& commandline) {
    for (const auto& option : commandline.options) {
        if (option.name == "runs") {
            specials.insert(RUNS);
        }
    }
}


bool CkStatus::global_cleanup() { return true; }


bool CkStatus::execute(const std::vector<std::string>& arguments) {
    if (configuration.status_db == "none") {
        throw Exception("No status database configured.");
    }

    try {
        status_db = std::make_shared<StatusDB>(configuration.status_db, DBH_WRITE | DBH_CREATE);
    }
    catch (const std::exception& e) {
        throw Exception("Error opening status database: " + std::string(e.what()));
    }

    for (auto key : specials) {
        switch (key) {
        case RUNS:
            list_runs();
            break;
        }
    }
    return true;
}

void CkStatus::list_runs() {
    auto runs = status_db->list_runs();

    for (const auto& run : runs) {
        char date_string[20];
        strftime(date_string, sizeof(date_string), "%Y-%m-%d %H:%M:%S", localtime(&run.date));
        printf("%" PRId64 ": %s\n", run.run_id, date_string);
    }
}
