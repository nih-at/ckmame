/*
  $NiH: dbdump.c,v 1.1.2.2 2005/08/06 22:11:42 wiz Exp $

  dbdump.c -- print contents of db
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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



#include <stdio.h>
#include <stdlib.h>

#include "dbl.h"
#include "error.h"
#include "util.h"
#include "xmalloc.h"

const char *prg;
const char *usage = "usage: %s db-file\n";
char *buf;
int bufsize;

int
dump(const DBT *key, const DBT *value, void *ud)
{
    if (value->size*2+1 > bufsize) {
	bufsize = value->size*2+1;
	buf = xrealloc(buf, bufsize);
    }
    printf("%.*s: %s\n", key->size, (char *)key->data,
	   bin2hex(buf, value->data, value->size));

    return 0;
}

int
main(int argc, char *argv[])
{
    int verbose;
    DB *db;

    verbose = 0;
    prg = argv[0];
    bufsize = 0;
    buf = NULL;

    /* in: two databases, verbose flag */
    if (strcmp(argv[1], "-v") == 0) {
	verbose = 1;
	argv++, argc--;
    }

    if (argc != 2) {
	fprintf(stderr, usage, prg);
	exit(1);
    }

    seterrinfo(argv[1], NULL);
    if ((db=ddb_open(argv[1], DDB_READ)) == NULL) {
	myerror(ERRDB, "can't open database");
	exit(1);
    }

    if (ddb_foreach(db, dump, NULL) < 0) {
	myerror(ERRDB, "can't read all keys and values from database");
	exit(1);
    }

    /* read-only */
    ddb_close(db);

    exit(0);
}