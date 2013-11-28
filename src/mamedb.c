/*
  mamedb.c -- tool to edit mamedb
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



#include <stdio.h>
#include <stdlib.h>

#include "compat.h"
#include "dbh.h"
#include "funcs.h"
#include "error.h"
#include "mamedb.h"
#include "output.h"
#include "parse.h"
#include "types.h"
#include "xmalloc.h"

char *usage = "Usage: %s [-hV] [-D dbfile] cmd [args ...]\n";

char help_head[] = "mamedb (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help                display this help message\n\
  -V, --version             display version number\n\
\n\
Use the help command for a list of available commands and help on\n\
individual commands.\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "mamedb (" PACKAGE " " VERSION ")\n\
Copyright (C) 2007 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "+hD:"

static struct option options[] = {
    { "help",             0, 0, 'h' },
    { "version",          0, 0, 'V' },
    { "db",               1, 0, 'o' },
    { NULL,               0, 0, 0 },
};

int romdb_hashtypes(db, TYPE_ROM);
detector_t *detector;
sqlite3 *db;
char *dbname;



int
main(int argc, char **argv)
{
    const cmd_t *cmd;
    int c, ret;

    setprogname(argv[0]);

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    romdb_hashtypes(db, TYPE_ROM) = HASHES_TYPE_CRC|HASHES_TYPE_MD5|HASHES_TYPE_SHA1;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, getprogname());
	    fputs(help, stdout);
	    exit(0);
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);
	case 'D':
	    dbname = optarg;
	    break;
    	default:
	    fprintf(stderr, usage, getprogname());
	    exit(1);
	}
    }

    if (optind == argc) {
	fprintf(stderr, usage, getprogname());
	exit(1);
    }

    if ((cmd=find_command(argv[optind])) == NULL) {
	exit(1);
    }

    db = NULL;
    ret = 0;

    if ((cmd->flags & CMD_FL_NODB) == 0) {
	/* TODO: use proper mode */
	if ((db=dbh_open(dbname, DBL_WRITE)) == NULL) {
	    myerror(ERRDB, "can't open database '%s'", dbname);
	    exit(1);
	}
	seterrdb(db);
    }

    if (cmd->fn(argc-optind, argv+optind) < 0)
	ret = 1;

    if (db)
	dbh_close(db);

    return ret;
}
