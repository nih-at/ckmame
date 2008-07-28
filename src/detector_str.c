/*
  detector_str.c -- return string representation of detector enums
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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



const char *
detector_file_test_type_str(detector_test_type_t t)
{
    switch (t) {
    case DETECTOR_TEST_FILE_EQ:
	return "equal";
    case DETECTOR_TEST_FILE_LE:
	return "less";
    case DETECTOR_TEST_FILE_GR:
	return "greater";

    default:
	return "unknown";
    }
    
}



const char *detector_operation_str(detector_operation_t op)
{
    switch (op) {
    case DETECTOR_OP_NONE:
	return "none";
    case DETECTOR_OP_BITSWAP:
	return "bitswap";
    case DETECTOR_OP_BYTESWAP:
	return "byteswap";
    case DETECTOR_OP_WORDSWAP:
	return "wordswap";

    default:
	return "unknown";
    }
}



const char *detector_test_type_str(detector_test_type_t t)
{
    switch (t) {
    case DETECTOR_TEST_DATA:
	return "data";
    case DETECTOR_TEST_OR:
	return "or";
    case DETECTOR_TEST_AND:
	return "and";
    case DETECTOR_TEST_XOR:
	return "xor";

    case DETECTOR_TEST_FILE_EQ:
    case DETECTOR_TEST_FILE_LE:
    case DETECTOR_TEST_FILE_GR:
	return "file";

    default:
	return "unknown";
    }
}
