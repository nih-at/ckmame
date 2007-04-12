#ifndef HAD_DETECTOR_H
#define HAD_DETECTOR_H

/*
  $NiH: detector.h,v 1.3 2007/04/12 12:36:07 dillo Exp $

  detector.h -- clrmamepro XML header skip detector
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



#include <stdbool.h>
#include <stdio.h>

#include "array.h"
#include "myinttypes.h"
#include "rom.h"

struct detector {
    char *name;
    char *author;
    char *version;

    array_t *rules;

    unsigned char *buf;
    size_t buf_size;
};

typedef struct detector detector_t ;

/* keep in sync with ops in detector_execute.c */

enum detector_operation {
    DETECTOR_OP_NONE,
    DETECTOR_OP_BITSWAP,
    DETECTOR_OP_BYTESWAP,
    DETECTOR_OP_WORDSWAP
};

typedef enum detector_operation detector_operation_t;

struct detector_rule {
    int64_t start_offset;
    int64_t end_offset;
    detector_operation_t operation;

    array_t *tests;
};

typedef struct detector_rule detector_rule_t;

enum detector_test_type {
    DETECTOR_TEST_DATA,
    DETECTOR_TEST_OR,
    DETECTOR_TEST_AND,
    DETECTOR_TEST_XOR,
    DETECTOR_TEST_FILE_EQ,
    DETECTOR_TEST_FILE_LE,
    DETECTOR_TEST_FILE_GR,
};

#define DETECTOR_OFFSET_EOF	0
#define DETECTOR_SIZE_PO2	(-1)

typedef enum detector_test_type detector_test_type_t;

struct detector_test {
    detector_test_type_t type;
    int64_t offset;
    uint64_t length;
    uint8_t *mask;
    uint8_t *value;
    bool result;
};

typedef struct detector_test detector_test_t;

typedef int (*detector_read_cb)(void *, void *, int);



#define detector_author(d)	((d)->author)
#define detector_name(d)	((d)->name)
#define detector_num_rules(d)	(array_length(detector_rules(d)))
#define detector_rule(d, i)	\
	((detector_rule_t *)array_get(detector_rules(d), (i)))
#define detector_rules(d)	((d)->rules)
#define detector_version(d)	((d)->version)

#define detector_rule_end_offset(r)	((r)->end_offset)
#define detector_rule_operation(r)	((r)->operation)
#define detector_rule_num_tests(d)	(array_length(detector_rule_tests(d)))
#define detector_rule_start_offset(r)	((r)->start_offset)
#define detector_rule_test(d, i)	\
	((detector_test_t *)array_get(detector_rule_tests(d), (i)))
#define detector_rule_tests(r)		((r)->tests)

#define detector_test_length(t)	((t)->length)
#define detector_test_mask(t)	((t)->mask)
#define detector_test_offset(t)	((t)->offset)
#define detector_test_result(t)	((t)->result)
#define detector_test_size	detector_test_offset
#define detector_test_type(t)	((t)->type)
#define detector_test_value(t)	((t)->value)

int detector_execute(detector_t *, rom_t *, detector_read_cb, void *);
void detector_free(detector_t *);
detector_t *detector_parse(const char *);
int detector_print(const detector_t *, FILE *);
detector_t *detector_new(void);

const char *detector_file_test_type_str(detector_test_type_t);
const char *detector_operation_str(detector_operation_t);
const char *detector_test_type_str(detector_test_type_t);
void detector_rule_init(detector_rule_t *);
void detector_rule_finalize(detector_rule_t *);
void detector_test_init(detector_test_t *);
void detector_test_finalize(detector_test_t *);

#endif /* detector.h */
