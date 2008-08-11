/*
  tree.c -- traverse tree of games to check
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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

#include "archive.h"
#include "dbh.h"
#include "error.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "sighandle.h"
#include "tree.h"
#include "xmalloc.h"

static tree_t *tree_add_node(tree_t *, const char *, int);
static tree_t *tree_new_full(const char *, int);
static int tree_process(const tree_t *, archive_t *, archive_t *, archive_t *);



int
tree_add(tree_t *tree, const char *name)
{
    game_t *g;

    if ((g=r_game(db, name)) == NULL)
	return -1;

    if (game_cloneof(g, file_type, 1))
	tree = tree_add_node(tree, game_cloneof(g, file_type, 1), 0);
    if (game_cloneof(g, file_type, 0))
	tree = tree_add_node(tree, game_cloneof(g, file_type, 0), 0);

    tree_add_node(tree, name, 1);

    game_free(g);
    
    return 0;
}



void
tree_free(tree_t *tree)
{
    tree_t *t;
    
    while (tree) {
	if (tree->child)
	    tree_free(tree->child);
	t = tree;
	tree = tree->next;
	free(t->name);
	free(t);
    }
}



tree_t *
tree_new(void)
{
    tree_t *t;

    t = xmalloc(sizeof(*t));

    t->name = NULL;
    t->check = 0;
    t->child = t->next = NULL;

    return t;
}



void
tree_traverse(const tree_t *tree, archive_t *parent, archive_t *gparent)
{
    tree_t *t;
    archive_t *child;
    char *full_name;
    int flags;

    child = NULL;

    if (tree->name) {
	if (siginfo_caught)
	    print_info(tree->name);

	flags = ((tree->check ? ARCHIVE_FL_CREATE : 0)
		 | (check_integrity ? ARCHIVE_FL_CHECK_INTEGRITY: 0)
		 | ((fix_options & FIX_TORRENTZIP) ? ARCHIVE_FL_TORRENTZIP :0));

	full_name = findfile(tree->name, file_type);
	if (full_name == NULL && tree->check) {
	    full_name = make_file_name(file_type, 0, tree->name);
	}
	if (full_name)
	    child = archive_new(full_name, TYPE_ROM, FILE_ROMSET, flags);
	free(full_name);

	if (tree->check)
	    tree_process(tree, child, parent, gparent);
    }

    for (t=tree->child; t; t=t->next)
	tree_traverse(t, child, parent);

    if (child)
	archive_free(child);

    return;
}



static tree_t *
tree_add_node(tree_t *tree, const char *name, int check)
{
    tree_t *t;
    int cmp;

    if (tree->child == NULL) {
	t = tree_new_full(name, check);
	tree->child = t;
	return t;
    }
    else {
	cmp = strcmp(tree->child->name, name);
	if (cmp == 0) {
	    if (check)
		tree->child->check = 1;
	    return tree->child;
	}
	else if (cmp > 0) {
	    t = tree_new_full(name, check);
	    t->next = tree->child;
	    tree->child = t;
	    return t;
	}
	else {
	    for (tree=tree->child; tree->next; tree=tree->next) {
		cmp = strcmp(tree->next->name, name);
		if (cmp == 0) {
		    if (check)
			tree->next->check = 1;
		    return tree->next;
		}
		else if (cmp > 0) {
		    t = tree_new_full(name, check);
		    t->next = tree->next;
		    tree->next = t;
		    return t;
		}
	    }

	    t = tree_new_full(name, check);
	    tree->next = t;
	    return t;
	}
    }
}



static tree_t *
tree_new_full(const char *name, int check)
{
    tree_t *t;

    t = tree_new();
    t->name = xstrdup(name);
    t->check = check;

    return t;
}



static int
tree_process(const tree_t *tree, archive_t *child,
	     archive_t *parent, archive_t *gparent)
{
    archive_t *all[3];
    game_t *g;
    result_t *res;
    images_t *images;

    /* check me */
    if ((g=r_game(db, tree->name)) == NULL) {
	myerror(ERRDEF, "db error: %s not found", tree->name);
	return -1;
    }

    all[0] = child;
    all[1] = parent;
    all[2] = gparent;

    images = images_new(g, check_integrity ? DISK_FL_CHECK_INTEGRITY : 0);

    res = result_new(g, child, images);

    check_old(g, res);
    check_files(g, all, res);
    check_archive(child, game_name(g), res);
    if (file_type == TYPE_ROM) {
	check_disks(g, images, res);
	check_images(images, game_name(g), res);
    }

    /* write warnings/errors for me */
    diagnostics(g, child, images, res);

    if (fix_options & (FIX_DO|FIX_PRINT))
	fix_game(g, child, images, res);
	
    /* clean up */
    result_free(res);
    game_free(g);
    images_free(images);

    return 0;
}
