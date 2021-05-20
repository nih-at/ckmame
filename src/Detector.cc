/*
  detector.c -- alloc/free detector
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


DetectorPtr detector;

DetectorDescriptor::DetectorDescriptor(const Detector *detector) : DetectorDescriptor(detector->name, detector->version) { }

DetectorCollection Detector::detector_ids;

std::string Detector::file_test_type_name(TestType type) {
    switch (type) {
        case TEST_FILE_EQ:
            return "equal";
        case TEST_FILE_LE:
            return "less";
        case TEST_FILE_GR:
            return "greater";
            
        default:
            return "unknown";
    }
}


std::string Detector::operation_name(Operation operation) {
    switch (operation) {
        case OP_NONE:
            return "none";
        case OP_BITSWAP:
            return "bitswap";
        case OP_BYTESWAP:
            return "byteswap";
        case OP_WORDSWAP:
            return "wordswap";
            
        default:
            return "unknown";
    }
}


std::string Detector::test_type_name(TestType type) {
    switch (type) {
        case TEST_DATA:
            return "data";
        case TEST_OR:
            return "or";
        case TEST_AND:
            return "and";
        case TEST_XOR:
            return "xor";
            
        case TEST_FILE_EQ:
        case TEST_FILE_LE:
        case TEST_FILE_GR:
            return "file";
            
        default:
            return "unknown";
    }
}


