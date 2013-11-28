/*
  mamedb_cmdtab.c -- table of commands
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



#include <string.h>

#include "error.h"
#include "mamedb.h"
#include "myinttypes.h"

#define ADD_DESC	"add games from archives"
#define ADD_USAGE	"[-C hashtypes] [-d dat-no] archive [...]"
#define ADD_HELP	"\
  -C, --hash-types types    specify hash types to compute (default: all)\n\
  -d, --dat-no no           specify which dat game belongs to (default: 0)\n\
"

#define CHECK_DESC	"check consistency"
#define CHECK_USAGE	""
#define CHECK_HELP	""

#define CLONE_OF_DESC	"set clone-of"
#define CLONE_OF_USAGE	"child parent"
#define CLONE_OF_HELP	""

#define EXPORT_DESC	"export to dat files"
#define EXPORT_USAGE	"[-d dat-no]"
#define EXPORT_HELP	"\
  -d, --dat-no no           specify which dat to export (default: all)\n\
"

#define FIND_CLONES_DESC	"find possible clones"
#define FIND_CLONES_USAGE	"[game]"
#define FIND_CLONES_HELP	""

#define HELP_DESC	"print list of commands or help on specific command" 
#define HELP_USAGE	"[cmd]"
#define HELP_HELP	NULL

#define IMPORT_DESC	"import dat file"
#define IMPORT_USAGE	"[-x pat] [file ...]"
#define IMPORT_HELP	"\
  -x, --exclude pat         exclude games matching shell glob PAT\n\
"

#define MAKE_PARENT_DESC	"set parent of a clone set"
#define MAKE_PARENT_USAGE	"child"
#define MAKE_PARENT_HELP	""

#define NEW_DESC	"create new database"
#define NEW_USAGE	""
#define NEW_HELP	NULL

#define REMOVE_DESC	"remove games"
#define REMOVE_USAGE	"game [...]"
#define REMOVE_HELP	""



#define XX(N)		N##_DESC, N##_USAGE, N##_HELP
#define ALIAS(N)	cmd_##N, CMD_FL_ALIAS, NULL, NULL, NULL

cmd_t cmdtab[] = {
    { "add",           cmd_add,         0,           XX(ADD) },
    { "help",          cmd_help,        CMD_FL_NODB, XX(HELP) },
    { "new",           cmd_new,         CMD_FL_NODB, XX(NEW) },

#if 0 /* not yet */
    { "check",         cmd_check,       0,           XX(CHECK) },
    { "clone-of",      cmd_clone_of,    0,           XX(CLONE_OF) },
    { "export",        cmd_export,      0,           XX(EXPORT) },
    { "find-clones",   cmd_find_clones, 0,           XX(FIND_CLONES) },
    { "import",        cmd_import,      CMD_FL_NODB, XX(IMPORT) },
    { "make-parent",   cmd_make_parent, 0,           XX(MAKE_PARENT) },
    { "remove",        cmd_remove,      0,           XX(REMOVE) },
#endif

    /* TODO: add/remove/edit single ROM */
    /* TODO: change dat info */
};
int ncmdtab = sizeof(cmdtab)/sizeof(cmdtab[0]);



const cmd_t *
find_command(const char *name)
{
    cmd_t *cmd;
    size_t len;
    int i;
    bool abbrev;

    len = strlen(name);
    abbrev = false;
    for (i=0; i<ncmdtab; i++) {
	if (strcmp(name, cmdtab[i].name) == 0)
	    cmd = cmdtab+i;

	if (len < strlen(cmdtab[i].name)
	    && strncmp(name, cmdtab[i].name, len) == 0) {
	    if (abbrev) {
		myerror(ERRDEF, "ambigous abbreviation '%s'", name);
		return NULL;
	    }
	    abbrev = true;
	}
	else if (abbrev)
	    break;
    }

    if (abbrev)
	cmd = cmdtab+(i-1);

    if (cmd == NULL) {
	myerror(ERRDEF, "unknown command '%s'", name);
	return NULL;
    }

    if (cmd->flags & CMD_FL_ALIAS) {
	for (i=0; i<ncmdtab; i++)
	    if (cmd->fn == cmdtab[i].fn
		&& (cmdtab[i].flags & CMD_FL_ALIAS) == 0)
		return cmdtab+i;
	
	myerror(ERRDEF, "unresolved alias '%s'");
    }

    return cmd;
}
