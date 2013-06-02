#ifndef HAD_GLOBALS_H
#define HAD_GLOBALS_H

/*
  globals.h -- declaration of global variables
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



#include "dbh.h"
#include "delete_list.h"
#include "detector.h"
#include "output.h"
#include "parray.h"
#include "tree.h"

extern dbh_t *db;
extern dbh_t *old_db;

extern detector_t *detector;

extern char *needed_dir;
extern char *unknown_dir;
extern const char *rom_dir;

extern parray_t *search_dirs;

extern delete_list_t *extra_delete_list;
extern parray_t *extra_list;
extern delete_list_t *needed_delete_list;
extern delete_list_t *superfluous_delete_list;

extern parray_t *superfluous;

extern int diskhashtypes;	/* hash type recorded for disks in db */
extern int romhashtypes;	/* hash type recorded for ROMs in db */
extern int check_integrity;	/* full integrity check of ROM set */

extern filetype_t file_type;	/* type of files to check (ROMs or samples) */

extern int output_options;
extern int fix_options;

extern tree_t *check_tree;
extern output_context_t *fixdat;

#endif /* globals.h */
