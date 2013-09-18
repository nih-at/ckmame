/*
  romdb_read_dat.c -- read dat struct from db
  Copyright (C) 2006-2013 Dieter Baron and Thomas Klausner

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



#include <string.h>
#include <stdlib.h>

#include "dat.h"
#include "romdb.h"
#include "sq_util.h"
#include "xmalloc.h"

dat_t *
romdb_read_dat(romdb_t *db)
{
    sqlite3_stmt *stmt;
    dat_t *dat;
    dat_entry_t *de;
    int ret;

    if ((stmt=dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_DAT)) == NULL) {
	/* XXX */
	return NULL;
    }

    dat = dat_new();

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	de = (dat_entry_t *)array_grow(dat, dat_entry_init);

	de->name = sq3_get_string(stmt, 0);
	de->description = sq3_get_string(stmt, 1);
	de->version = sq3_get_string(stmt, 2);
    }

    if (ret != SQLITE_DONE) {
	/* XXX */
	dat_free(dat);
	return NULL;
    }
    
    return dat;
}
