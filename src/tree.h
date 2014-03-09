#ifndef HAD_TREE_H
#define HAD_TREE_H

/*
  tree.h -- traverse tree of games to check
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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


#include "types.h"
#include "archive.h"
#include "hashes.h"

struct tree {
    char *name;
    bool check;
    bool checked;
    struct tree *next, *child;
};

typedef struct tree tree_t;


#define tree_check(t)	((t)->check)
#define tree_checked(t)	((t)->checked)
#define tree_name(t)	((t)->name)

int tree_add(tree_t *, const char *);
int tree_add_games_needing(tree_t *, uint64_t, const hashes_t *);
void tree_free(tree_t *);
tree_t *tree_new(void);
void tree_recheck(const tree_t *, const char *);
int tree_recheck_games_needing(tree_t *, uint64_t, const hashes_t *);
void tree_traverse(tree_t *, archive_t *, archive_t *);

#endif /* tree.h */
