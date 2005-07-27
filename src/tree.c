/*
  $NiH: tree.c,v 1.1 2005/07/13 17:42:20 dillo Exp $

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
static int tree_child_traverse(DB *, const tree_t *, int,
			       archive_t *, archive_t *, int, int);
static tree_t *tree_new_full(const char *, int);



int
tree_add(DB *db, tree_t *tree, const char *name)
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
tree_traverse(DB *db, const tree_t *tree)
{
    tree_t *t;
    
    for (t=tree->child; t; t=t->next)
	tree_child_traverse(db, t, 0, NULL, NULL, 0, 0);
#if 0
    /* XXX */
    
    /* run through children & update zipstruct */
    for (t=tree->child; t; t=t->next) {
	/* XXX: init i */
	if (tree->check)
	    i = game_clone_index(me_g, file_type, t->name);
	tree_child_traverse(db, t, tree->check, me_a, parent_a, i+1,
			    parent_no);
    }
#endif
    
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
tree_child_traverse(DB *db, const tree_t *tree, int parentcheck,
		    archive_t *parent_a, archive_t *gparent_a,
		    int parent_no, int gparent_no)
{
    archive_t *me_a, *all_a[3];
    game_t *me_g;
    match_array_t *me_m;
    match_disk_array_t *me_d;
    file_status_array_t *me_fs;

    /* check me */
    if ((me_g=r_game(db, tree->name)) == NULL) {
	myerror(ERRDEF, "db error: %s not found", tree->name);
	return -1;
    }

    me_a = archive_new(tree->name, file_type, NULL);
    all_a[0] = me_a;
    all_a[1] = parent_a;
    all_a[2] = gparent_a;

    me_m = check_files(me_g, all_a);
    me_fs = check_archive(me_a, me_m);
    if (file_type == TYPE_ROM)
	me_d = check_disks(me_g);
    else
	me_d = NULL;

    /* write warnings/errors for me */
    diagnostics(me_g, me_a, me_m, me_d, me_fs);

    fix_game(me_g, me_a, me_m, me_d, me_fs);
	
    /* clean up */
    file_status_array_free(me_fs);
    match_disk_array_free(me_d);
    match_array_free(me_m);
    game_free(me_g);
    archive_free(me_a);
    
    return 0;
}
