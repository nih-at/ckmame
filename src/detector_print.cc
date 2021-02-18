/*
  detector_print.c -- print clrmamepro header skip detector in XML format
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

#include "detector.h"

#include <cinttypes>

#include "util.h"


static void pr_string(FILE *fout, const char *name, const std::string &value) {
    if (value.empty()) {
        return;
    }

    fprintf(fout, "  <%s>%s</%s>\n", name, value.c_str(), name);
}

bool Detector::print(FILE *fout) const {
    fprintf(fout, "<?xml version=\"1.0\"?>\n\n<detector>\n\n");
    pr_string(fout, "name", name);
    pr_string(fout, "author", author);
    pr_string(fout, "version", version);
    fprintf(fout, "\n");

    for (auto &rule : rules) {
        rule.print(fout);
    }

    fprintf(fout, "</detector>\n");

    return (ferror(fout) == 0);
}


void Detector::Rule::print(FILE *fout) const {
    fprintf(fout, "  <rule");
    if (start_offset != 0) {
        fprintf(fout, " start_offset=\"%" PRId64 "\"", start_offset);
        if (end_offset != DETECTOR_OFFSET_EOF) {
            fprintf(fout, " end_offset=\"%" PRId64 "\"", end_offset);
        }
    }
    
    if (operation != OP_NONE) {
        fprintf(fout, " operation=\"%s\"", operation_name(operation).c_str());
    }
    
    if (tests.empty()) {
        fprintf(fout, "/>\n\n");
    }
    else {
        fprintf(fout, ">\n");
        for (auto &test: tests) {
            test.print(fout);
        }
        fprintf(fout, "  </rule>\n\n");
    }
}


void Detector::Test::print(FILE *fout) const {
    fprintf(fout, "    <%s", test_type_name(type).c_str());

    switch (type) {
        case TEST_DATA:
        case TEST_OR:
        case TEST_AND:
        case TEST_XOR:
            if (offset != 0) {
                fprintf(fout, " offset=\"%" PRId64 "\"", offset);
            }
            if (!mask.empty()) {
                fprintf(fout, " mask=\"%s\"", bin2hex(mask.data(), mask.size()).c_str());
            }
            fprintf(fout, " value=\"%s\"", bin2hex(value.data(), value.size()).c_str());
            break;

        case TEST_FILE_EQ:
        case TEST_FILE_LE:
        case TEST_FILE_GR:
            fprintf(fout, " size=\"");
            if (offset == DETECTOR_SIZE_POWER_OF_2) {
                fprintf(fout, "PO2");
            }
            else {
                fprintf(fout, "%" PRId64, offset);
            }
            fprintf(fout, "\"");
            if (type != TEST_FILE_EQ) {
                fprintf(fout, " operator=\"%s\"", file_test_type_name(type).c_str());
            }
            break;
    }
    
    if (result != true) {
        fprintf(fout, " result=\"false\"");
    }

    fprintf(fout, "/>\n");
}
