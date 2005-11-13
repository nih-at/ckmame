/*
  $NiH: tree.c,v 1.3 2005/10/05 21:21:33 dillo Exp $

  tree.c -- traverse tree of games to check
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

    if (tree->name) {
	child = archive_new(tree->name, file_type, tree->check);
    
	if (tree->check)
	    tree_process(tree, child, parent, gparent);
    }

    for (t=tree->child; t; t=t->next)
	tree_traverse(t, child, parent);

    if (tree->name)
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

/* XXX: convert rest */



static int
tree_process(const tree_t *tree, archive_t *child,
	     archive_t *parent, archive_t *gparent)
{
    archive_t *all[3];
    game_t *g;
    match_array_t *ma;
    match_disk_array_t *mda;
    file_status_array_t *fsa, *dsa;
    parray_t *dn;

    /* check me */
    if ((g=r_game(db, tree->name)) == NULL) {
	myerror(ERRDEF, "db error: %s not found", tree->name);
	return -1;
    }

    all[0] = child;
    all[1] = parent;
    all[2] = gparent;

    ma = check_files(g, all);
    fsa = check_archive(child, ma, game_name(g));
    if (file_type == TYPE_ROM)
	check_disks(g, &dsa, &mda, &dn);
    else {
	mda = NULL;
	dsa = NULL;
	dn = NULL;
    }

    /* write warnings/errors for me */
    diagnostics(g, child, ma, mda, fsa, dsa, dn);

    if (fix_game(g, child, ma, mda, fsa, dsa, dn) == 1 && tree->child)
	archive_refresh(child);
	
    /* clean up */
    file_status_array_free(fsa);
    file_status_array_free(dsa);
    match_disk_array_free(mda);
    match_array_free(ma);
    parray_free(dn, free);
    game_free(g);

    /* XXX: commit changes to child */

    return 0;
}
