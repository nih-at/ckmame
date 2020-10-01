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


#include <stdlib.h>

#include "detector.h"
#include "util.h"
#include "xmalloc.h"

static void pr_rule(FILE *, const detector_rule_t *);
static void pr_string(FILE *, const char *, const char *);
static void pr_test(FILE *, const detector_test_t *);


int
detector_print(const detector_t *d, FILE *fout) {
    int i;

    fprintf(fout, "<?xml version=\"1.0\"?>\n\n<detector>\n\n");
    pr_string(fout, "name", detector_name(d));
    pr_string(fout, "author", detector_author(d));
    pr_string(fout, "version", detector_version(d));
    fprintf(fout, "\n");

    for (i = 0; i < detector_num_rules(d); i++)
	pr_rule(fout, detector_rule(d, i));

    fprintf(fout, "</detector>\n");

    return (ferror(fout));
}


static void
pr_rule(FILE *fout, const detector_rule_t *dr) {
    int i;

    fprintf(fout, "  <rule");
    if (detector_rule_start_offset(dr) != 0)
	fprintf(fout, " start_offset=\"%jx\"", (intmax_t)detector_rule_start_offset(dr));
    if (detector_rule_end_offset(dr) != DETECTOR_OFFSET_EOF)
	fprintf(fout, " end_offset=\"%jx\"", (intmax_t)detector_rule_end_offset(dr));
    if (detector_rule_operation(dr) != DETECTOR_OP_NONE)
	fprintf(fout, " operation=\"%s\"", detector_operation_str(detector_rule_operation(dr)));
    if (detector_rule_num_tests(dr) == 0)
	fprintf(fout, "/>\n\n");
    else {
	fprintf(fout, ">\n");
	for (i = 0; i < detector_rule_num_tests(dr); i++)
	    pr_test(fout, detector_rule_test(dr, i));
	fprintf(fout, "  </rule>\n\n");
    }
}


static void
pr_string(FILE *fout, const char *name, const char *value) {
    if (value == NULL)
	return;

    fprintf(fout, "  <%s>%s</%s>\n", name, value, name);
}


static void
pr_test(FILE *fout, const detector_test_t *dt) {
    char *hex;

    fprintf(fout, "    <%s", detector_test_type_str(detector_test_type(dt)));
    switch (detector_test_type(dt)) {
    case DETECTOR_TEST_DATA:
    case DETECTOR_TEST_OR:
    case DETECTOR_TEST_AND:
    case DETECTOR_TEST_XOR:
	hex = static_cast<char *>(xmalloc(detector_test_length(dt) * 2 + 1));

	if (detector_test_offset(dt) != 0)
	    fprintf(fout, " offset=\"%jx\"", (intmax_t)detector_test_offset(dt));
	if (detector_test_mask(dt))
	    fprintf(fout, " mask=\"%s\"", bin2hex(hex, detector_test_mask(dt), detector_test_length(dt)));
	fprintf(fout, " value=\"%s\"", bin2hex(hex, detector_test_value(dt), detector_test_length(dt)));

	free(hex);
	break;

    case DETECTOR_TEST_FILE_EQ:
    case DETECTOR_TEST_FILE_LE:
    case DETECTOR_TEST_FILE_GR:
	fprintf(fout, " size=\"");
	if (detector_test_size(dt) == DETECTOR_SIZE_PO2)
	    fprintf(fout, "PO2");
	else
	    fprintf(fout, "%jx", (intmax_t)detector_test_size(dt));
	fprintf(fout, "\"");
	if (detector_test_type(dt) != DETECTOR_TEST_FILE_EQ)
	    fprintf(fout, " operator=\"%s\"", detector_file_test_type_str(detector_test_type(dt)));
	break;
    }

    if (detector_test_result(dt) != true)
	fprintf(fout, " result=\"false\"");

    fprintf(fout, "/>\n");
}
