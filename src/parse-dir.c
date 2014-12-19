/*
  parse-dir.c -- read info from zip archives
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


#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "dir.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"


static int parse_archive(parser_context_t *, archive_t *, int hashtypes);


int
parse_dir(const char *dname, parser_context_t *ctx, int hashtypes)
{
    dir_t *dir;
    archive_t *a;
    char b[8192];
    dir_status_t ds;
    struct stat st;

    ctx->lineno = 0;
    
    if ((dir=dir_open(dname, roms_unzipped ? 0 : DIR_RECURSE)) == NULL)
	return -1;

    if (roms_unzipped) {
        while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
            if (ds == DIR_ERROR) {
		myerror(ERRSTR, "error reading directory entry '%s', skipped", b);
                continue;
            }

            if (stat(b, &st) < 0) {
		myerror(ERRSTR, "can't stat '%s', skipped", b);
                /* TODO: handle error */
                continue;
            }
            
            if (S_ISDIR(st.st_mode)) {
                /* TODO: handle errors */
                if ((a = archive_new(b, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE)) != NULL) {
                    parse_archive(ctx, a, hashtypes);
                    archive_close(a);
                }
            }
            else {
                /* TOOD: if chd, include in dat */
		if (S_ISREG(st.st_mode)) {
		    myerror(ERRDEF, "skipping unknown file '%s'", b);
		}
            }
        }
    }
    else {
        while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
            if (ds == DIR_ERROR) {
		myerror(ERRSTR, "error reading directory entry '%s', skipped", b);
                continue;
            }
            switch (name_type(b)) {
                case NAME_ZIP:
                    /* TODO: handle errors */
                    if ((a=archive_new(b, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE)) != NULL) {
                        parse_archive(ctx, a, hashtypes);
                        archive_free(a);
                    }
                    break;
                    
                case NAME_CHD:
                    /* TODO: include disks in dat */
                case NAME_NOEXT:
                case NAME_UNKNOWN:
		    if (stat(b, &st) < 0) {
			myerror(ERRSTR, "can't stat '%s', skipped", b);
			break;
		    }
		    if (S_ISREG(st.st_mode)) {
			myerror(ERRDEF, "skipping unknown file '%s'", b);
		    }
                    break;
            }
        }
    }
    
    dir_close(dir);
    return 0;
}


static int
parse_archive(parser_context_t *ctx, archive_t *a, int hashtypes)
{
    char *name;
    int i, ht;
    file_t *r;
    char hstr[HASHES_SIZE_MAX*2+1];

    parse_game_start(ctx, 0);

    name = xstrdup(mybasename(archive_name(a)));
    if (strlen(name) > 4 && strcmp(name+strlen(name)-4, ".zip") == 0)
	name[strlen(name)-4] = '\0';
    parse_game_name(ctx, 0, 0, name);
    free(name);

    for (i=0; i<archive_num_files(a); i++) {
	if (archive_file_compute_hashes(a, i, hashtypes) < 0)
	    continue;
	r = archive_file(a, i);

	parse_file_start(ctx, TYPE_ROM);
	parse_file_name(ctx, TYPE_ROM, 0, file_name(r));
	sprintf(hstr, "%" PRIu64, file_size(r));
	parse_file_size(ctx, TYPE_ROM, 0, hstr);
        parse_file_mtime(ctx, TYPE_ROM, 0, file_mtime(r));
	for (ht=1; ht<=HASHES_TYPE_MAX; ht<<=1) {
	    if (hashtypes & ht)
		parse_file_hash(ctx, TYPE_ROM, ht, hash_to_string(hstr, ht, file_hashes(r)));
	}
	parse_file_end(ctx,TYPE_ROM);
    }

    parse_game_end(ctx, 0);

    return 0;
}
