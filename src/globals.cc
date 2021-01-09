/*
  globals.c -- definition of global variables
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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

#include "globals.h"

#include <sys/param.h>
/* option settings */

const char *needed_dir = "needed";   /* TODO: proper value */
const char *unknown_dir = "unknown"; /* TODO: proper value */
const char *rom_dir = NULL;

parray_t *search_dirs = NULL;

int check_integrity = 0; /* full integrity check of ROM set */
int roms_unzipped = 0;   /* ROMs are files on disk, not contained in zip archives */

int output_options = 0;
int fix_options = 0;


/* check input (read only) */

romdb_t *db = NULL;
romdb_t *old_db = NULL;

DetectorPtr detector;

std::vector<std::string> superfluous;


/* check state */

DeleteListPtr extra_delete_list = NULL;
std::vector<std::string> extra_list;
DeleteListPtr needed_delete_list = NULL;
DeleteListPtr superfluous_delete_list = NULL;

tree_t *check_tree = NULL;
OutputContextPtr fixdat;

/* roms dir */
char rom_dir_normalized[MAXPATHLEN];

stats_t *stats = NULL;
