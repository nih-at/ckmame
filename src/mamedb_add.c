/*
  mamedb_add.c -- add subcommand
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



#include <stdlib.h>

#include "compat.h"
#include "error.h"
#include "hashes.h"
#include "mamedb.h"

#define OPTIONS "C:d:"

static struct option options[] = {
    { "dat-no",           1, 0, 'd' },
    { "hash-types",       1, 0, 'C' },
    { NULL,               0, 0, 0 },
};




int
cmd_add(int argc, char **argv)
{
    int datno, hashtypes;
    int c;

    datno = 0;
    hashtypes = HASHES_TYPE_ALL;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'C':
	    hashtypes = hash_types_from_str(optarg);
            if (hashtypes == 0) {
		myerror(ERRDEF, "illegal hash types `%s'", optarg);
                return -1;
            }
	    break;
	case 'd':
	    datno = atoi(optarg); /* TODO: check valid input */
	    break;

    	default:
	    command_usage(stderr, argv[0]);
	    return -1;
	}
    }

    if (optind == argc) {
	command_usage(stderr, argv[0]);
	return -1;
    }

    /* TODO: add games */

    return 0;
}
