/*
  romdb_write_dat.c -- write dat struct to db
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "dat.h"
#include "romdb.h"
#include "sq_util.h"
#include "types.h"


int
romdb_write_dat(romdb_t *db, const std::vector<DatEntry> &dat) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_DAT)) == NULL)
	return -1;

    for (size_t i = 0; i < dat.size(); i++) {
        if (sqlite3_bind_int(stmt, 1, i) != SQLITE_OK || sq3_set_string(stmt, 2, dat[i].name) != SQLITE_OK || sq3_set_string(stmt, 3, dat[i].description) != SQLITE_OK || sq3_set_string(stmt, 4, dat[i].version) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK)
	    return -1;
    }

    return 0;
}
