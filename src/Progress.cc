/*
Progress.cc -- progress reporting
Copyright (C) 2024 Dieter Baron and Thomas Klausner

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

#include "Progress.h"

//#include <chrono>
#include <csignal>
#include <iostream>

#include "globals.h"
#include "ProgramName.h"

std::vector<std::string> Progress::messages;
volatile bool Progress::siginfo_caught = false;
bool Progress::trace = false;

void Progress::push_message(std::string message) {
    messages.emplace_back(std::move(message));

    if (trace || siginfo_caught) {
        print_message(true);
    }
}

void Progress::pop_message() {
    if (messages.empty()) {
        // TODO: warning?
        return;
    }
    if (trace){
        print_message(false);
    }
    messages.pop_back();
}

void Progress::sig_handler(int signal) {
#ifdef SIGINFO
    if (signal == SIGINFO) {
        siginfo_caught = true;
    }
#endif
}
void Progress::print_message(bool starting) {
    if (!trace && !starting) {
        return;
    }

    if (trace) {
        if (messages.empty()) {
            return;
        }
        // C++ 20:
        // std::cout << std::format("%Y-%m-%d %H:%M:%S ", std::chrono::system_clock::now());
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S ") << (starting ? "start " : "done ");
    }
    else {
        std::cout << ProgramName::get() << ": ";
        if (messages.empty()) {
            std::cout << "no progress available" << std::endl;
            return;
        }
        else {
            std::cout << "currently ";
        }
    }
    std::cout << messages.back();
    if (!configuration.set.empty()) {
        std::cout << " in set " << configuration.set;
    }
    std::cout << std::endl;

    if (!trace) {
        auto first = true;
        for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
            if (first) {
                first = false;
            }
            else {
                std::cout << "\t" << *it << std::endl;
            }
        }
    }

    siginfo_caught = false;
}

void Progress::enable() {
#ifdef SIGINFO
    signal(SIGINFO, sig_handler);
#endif
}
