#ifndef _HAD_DBH_DIR_H
#define _HAD_DBH_DIR_H

/*
 dbh_dir.h -- files in dirs sqlite3 data base
 Copyright (C) 2014 Dieter Baron and Thomas Klausner
 
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

#include "array.h"
#include "dbh.h"
#include "file.h"

int dbh_dir_close_all(void);
int dbh_dir_delete(dbh_t *, int);
int dbh_dir_delete_files(dbh_t *, int);
int dbh_dir_get_archive_id(dbh_t *, const char *);
bool dbh_dir_get_archive_last_change(dbh_t *, int, time_t *, off_t *);
dbh_t *dbh_dir_get_db_for_archive(const char *);
bool dbh_dir_is_empty(dbh_t *);
parray_t *dbh_dir_list_archives(dbh_t *);
int dbh_dir_read(dbh_t *, const char *, array_t *);
int dbh_dir_register_cache_directory(const char *);
int dbh_dir_write(dbh_t *, int, const char *, time_t, off_t, array_t *);
int dbh_dir_write_archive(dbh_t *, int, const char *, time_t, off_t);
int dbh_dir_write_file(dbh_t *, int, const file_t *);
int ensure_romset_dir_db(void);

#endif /* dbh_dir.h */
