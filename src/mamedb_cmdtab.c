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
#define EXPORT_USAGE	""
#define EXPORT_HELP	""

#define FIND_CLONES_DESC	"find possible clones"
#define FIND_CLONES_USAGE	"[game]"
#define FIND_CLONES_HELP	""

#define HELP_DESC	"print list of commands or help on specific command" 
#define HELP_USAGE	"[cmd]"
#define HELP_HELP	NULL

#define IMPORT_DESC	"import dat file"
#define IMPORT_USAGE	""
#define IMPORT_HELP	""

#define MAKE_PARENT_DESC	"set parent of a clone set"
#define MAKE_PARENT_USAGE	"child"
#define MAKE_PARENT_HELP	""

#define REMOVE_DESC	"remove games"
#define REMOVE_USAGE	"game [...]"
#define REMOVE_HELP	""



cmd_t cmdtab[] = {
    { "add",           ADD_DESC, cmd_add, ADD_USAGE, ADD_HELP },
#if 0
    { "check",         CHECK_DESC, cmd_check, CHECK_USAGE, CHECK_HELP },
    { "clone-of",      CLONE_OF_DESC, cmd_clone_of, CLONE_OF_USAGE, CLONE_OF_HELP },
    { "export",        EXPORT_DESC, cmd_export, EXPORT_USAGE, EXPORT_HELP },
    { "find-clones",   FIND_CLONES_DESC, cmd_find_clones, FIND_CLONES_USAGE, FIND_CLONES_HELP },
#endif
    { "help",          HELP_DESC, cmd_help, HELP_USAGE, HELP_HELP },
#if 0
    { "import",        IMPORT_DESC, cmd_import, IMPORT_USAGE, IMPORT_HELP },
    { "make-parent",   MAKE_PARENT_DESC, cmd_make_parent, MAKE_PARENT_USAGE, MAKE_PARENT_HELP },
    { "remove",        REMOVE_DESC, cmd_remove, REMOVE_USAGE, REMOVE_HELP },
#endif

    /* XXX: add/remove/edit single ROM */
    /* XXX: change dat info */
};
int ncmdtab = sizeof(cmdtab)/sizeof(cmdtab[0]);



const cmd_t *
find_command(const char *name)
{
    size_t len;
    int i;
    bool abbrev;

    len = strlen(name);
    abbrev = false;
    for (i=0; i<ncmdtab; i++) {
	if (strcmp(name, cmdtab[i].name) == 0)
	    return cmdtab+i;

	if (len < strlen(cmdtab[i].name)
	    && strncmp(name, cmdtab[i].name, len) == 0) {
	    if (abbrev) {
		myerror(ERRDEF, "ambigous abbreviation `%s'", name);
		return NULL;
	    }
	    abbrev = true;
	}
	else if (abbrev)
	    return cmdtab+(i-1);
    }

    if (abbrev)
	return cmdtab+(i-1);

    myerror(ERRDEF, "unknown command `%s'", name);
    return NULL;
}



const cmd_t *
resolve_alias(const cmd_t *cmd)
{
    int i;

    if (cmd->usage != NULL)
	return cmd;

    for (i=0; i<ncmdtab; i++)
	if (strcmp(cmd->name, cmdtab[i].name) == 0 && cmdtab[i].usage != NULL)
	    return cmdtab+i;

    myerror(ERRDEF, "unresolved alias `%s'");
    return NULL;
}
