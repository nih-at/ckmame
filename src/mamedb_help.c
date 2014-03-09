/*
  mamedb_help.c -- mamedb help subcommand
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


#include <stdlib.h>

#include "compat.h"
#include "mamedb.h"

static void help_all(void);
static void help_one(const char *);


int
cmd_help(int argc, char **argv)
{
    switch (argc) {
    case 1:
	help_all();
	break;

    case 2:
	help_one(argv[1]);
	break;

    default:
	command_usage(stderr, argv[0]);
	return -1;
    }

    return 0;
}


void
command_usage(FILE *fout, const char *name)
{
    const cmd_t *cmd;
    
    if ((cmd=find_command(name)) == NULL)
	return;

    fprintf(fout, "Usage: %s %s %s\n",
	    getprogname(), cmd->name, cmd->usage);
}


static void
help_all(void)
{
    int i;

    printf("list of commands:\n\n");

    for (i=0; i<ncmdtab; i++) {
	/* skip aliases */
	if (cmdtab[i].usage == NULL)
	    continue;

	printf("  %-16s %s\n", cmdtab[i].name, cmdtab[i].description);
    }

    printf("\nUse %s help CMD for help on a specific command.\n",
	   getprogname());
}


static void
help_one(const char *name)
{
    const cmd_t *cmd;

    if ((cmd=find_command(name)) == NULL)
	return;

    command_usage(stdout, name);
    printf("  %s\n", cmd->description);
    if (cmd->help)
	printf("\n%s", cmd->help);
}
