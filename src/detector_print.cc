/*
  detector_print.cc -- print clrmamepro header skip detector in XML format
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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

#include "Detector.h"

#include <cinttypes>
#include <iostream>

#include "util.h"


static void pr_string(std::ostream& out, const char* name, const std::string& value) {
    if (value.empty()) {
        return;
    }

    out << "  <" << name << ">" << value << "</" << name << ">" << std::endl;
}

bool Detector::print(std::ostream& out) const {
    out << "<?xml version=\"1.0\"?>" << std::endl << "<detector>" << std::endl << std::endl;
    pr_string(out, "name", name);
    pr_string(out, "author", author);
    pr_string(out, "version", version);
    out << std::endl;

    for (auto& rule : rules) {
        rule.print(out);
    }

    out << "</detector>" << std::endl;

    return (out.good());
}


void Detector::Rule::print(std::ostream& out) const {
    out << "  <rule";
    if (start_offset != 0) {
        out << " start_offset=\"" << start_offset << "\"";
        if (end_offset != DETECTOR_OFFSET_EOF) {
            out << " end_offset=\"" << end_offset << "\"";
        }
    }

    if (operation != OP_NONE) {
        out << " operation=\"" << operation_name(operation) << "\"";
    }

    if (tests.empty()) {
        out << "/>" << std::endl << std::endl;
    }
    else {
        out << ">" << std::endl;
        for (auto& test : tests) {
            test.print(out);
        }
        out << "  </rule>" << std::endl << std::endl;
    }
}


void Detector::Test::print(std::ostream& out) const {
    out << "    <" << test_type_name(type) << "\"";

    switch (type) {
    case TEST_DATA:
    case TEST_OR:
    case TEST_AND:
    case TEST_XOR:
        if (offset != 0) {
            out << " offset=\"" << offset << "\"";
        }
        if (!mask.empty()) {
            out << " mask=\"" << bin2hex(mask) << "\"";
        }
        out << " value=\"" << bin2hex(value) << "\"";
        break;

    case TEST_FILE_EQ:
    case TEST_FILE_LE:
    case TEST_FILE_GR:
        out << " size=\"";
        if (offset == DETECTOR_SIZE_POWER_OF_2) {
            out << "PO2";
        }
        else {
            out << offset;
        }
        out << "\"";
        if (type != TEST_FILE_EQ) {
            out << " operator=\"" << file_test_type_name(type) << "\"";
        }
        break;
    }

    if (!result) {
        out << " result=\"false\"";
    }

    out << "/>" << std::endl;
}
