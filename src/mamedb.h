#ifndef HAD_MAMEDB_H
#define HAD_MAMEDB_H

/*
  mamedb.h -- tool to edit mamedb, common definitions
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


#include <stdio.h>

struct cmd {
    char *name;
    int (*fn)(int, char **);
    int flags;
    char *description;
    char *usage;
    char *help;
};

typedef struct cmd cmd_t;

#define CMD_FL_ALIAS	0x01
#define CMD_FL_NODB	0x02

extern cmd_t cmdtab[];
extern int ncmdtab;

extern char *dbname;

void command_usage(FILE *, const char *);
const cmd_t *find_command(const char *);
const cmd_t *resolve_alias(const cmd_t *cmd);


extern int cmd_add(int, char **);
extern int cmd_check(int, char **);
extern int cmd_clone_of(int, char **);
extern int cmd_export(int, char **);
extern int cmd_find_clones(int, char **);
extern int cmd_help(int, char **);
extern int cmd_import(int, char **);
extern int cmd_make_parent(int, char **);
extern int cmd_new(int, char **);
extern int cmd_remove(int, char **);

#endif /* mamedb.h */
