/*
  $NiH$

  detector_str.c -- return string representation of detector enums
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
