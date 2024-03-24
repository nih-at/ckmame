/*
Progress.cc --

Copyright (C) Dieter Baron

The authors can be contacted at <accelerate@tpau.group>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. The names of the authors may not be used to endorse or promote
  products derived from this software without specific prior
  written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS
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

std::string Progress::current_message;
volatile bool Progress::siginfo_caught = false;
bool Progress::trace = false;

void Progress::set_message(std::string message) {
    current_message = std::move(message);

    if (trace || siginfo_caught) {
        print_message();
    }
}

void Progress::sig_handler(int signal) {
#ifdef SIGINFO
    if (signal == SIGINFO) {
        siginfo_caught = true;
    }
#endif
}
void Progress::print_message() {
    if (current_message.empty()) {
        return;
    }

    if (trace) {
        // C++ 20:
        // std::cout << std::format("%Y-%m-%d %H:%M:%S ", std::chrono::system_clock::now());
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S ");
    }
    else {
        std::cout << ProgramName::get() << ": ";
    }
    std::cout << current_message;
    if (!configuration.set.empty()) {
        std::cout << " in set " << configuration.set;
    }
    std::cout << std::endl;

    siginfo_caught = false;
}

void Progress::enable() {
#ifdef SIGINFO
    signal(SIGINFO, sig_handler);
#endif
}
