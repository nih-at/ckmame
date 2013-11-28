/*
  superfluous.c -- check for unknown file in rom directories
  Copyright (C) 1999-2012 Dieter Baron and Thomas Klausner

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
#include <string.h>
#include <stdlib.h>


#include "dir.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "romdb.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"



parray_t *
list_directory(const char *dirname, const char *dbname)
{
    dir_t *dir;
    char b[8192], *p;
    parray_t *listf, *listd, *lst, *found;
    dir_status_t err;
    int len_dir, len_name;

    listf = listd = NULL;

    if (dbname) {
	if (file_type == TYPE_ROM) {
	    if ((listf=romdb_read_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	        myerror(ERRDEF, "list of games not found in database `%s'",
		        dbname);
	        exit(1);
	    }
	    if ((listd=romdb_read_list(db, DBH_KEY_LIST_DISK)) == NULL) {
	        myerror(ERRDEF, "list of disks not found in database `%s'",
		        dbname);
	        exit(1);
	    }
        }
        else {
	    if ((listf=romdb_read_list(db, DBH_KEY_LIST_SAMPLE)) == NULL) {
	        myerror(ERRDEF, "list of samples not found in database `%s'",
		        dbname);
	        exit(1);
	    }
        }
    }

    found = parray_new();

    if ((dir=dir_open(dirname, 0)) == NULL)
	return found;

    len_dir = strlen(dirname)+1;

    while ((err=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	if (err == DIR_ERROR) {
	    /* TODO: handle error */
	    continue;
	}

	len_name = strlen(b+len_dir);

	if (len_name > 4) {
	    p = b+len_dir+len_name-4;
	    if (strcmp(p, ".zip") == 0) {
		*p = '\0';
		lst = listf;
	    }
	    else if (strcmp(p, ".chd") == 0) {
		*p = '\0';
		lst = listd;
	    }
	    else {
		p = NULL;
		lst = listd;
	    }
	}
	else {
	    p = NULL;
	    lst = listd;
	}

	if (lst == NULL || parray_index_sorted(lst, b+len_dir, strcmp) == -1) {
	    if (p)
		*p = '.';
		
	    parray_push(found, xstrdup(b));
	}
    }
    dir_close(dir);

    if (parray_length(found) > 0)
	parray_sort_unique(found, strcmp);

    return found;
}



void
print_superfluous(const parray_t *files)
{
    int i;

    if (parray_length(files) == 0)
	return;

    printf("Extra files found:\n");
    
    for (i=0; i<parray_length(files); i++)
	printf("%s\n", (char *)parray_get(files, i));
}
