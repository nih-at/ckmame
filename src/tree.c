/*
  $NiH: treetrav.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

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
#include "tree.h"
#include "xmalloc.h"

static array_t *get_checked(const tree_t *, const parray_t *);
static void get_checked_r(const tree_t *, array_t *, const parray_t *);
static int has_unused(archive_t *);
static void set_zero(int *);
static tree_t *tree_add_node(tree_t *, const char *, filetype_t, int);
static int tree_child_traverse(DB *, const tree_t *, int,
			       archive_t *, archive_t *, int, int);
static tree_t *tree_new_full(const char *, filetype_t, int);



int
tree_add(DB *db, tree_t *tree, const char *name, filetype_t ft)
{
    game_t *g;

    if (tree->ft != ft)
	return -1;

    if ((g=r_game(db, name)) == NULL)
	return -1;

    if (game_cloneof(g, ft, 1))
	tree = tree_add_node(tree, game_cloneof(g, ft, 1), ft, 0);
    if (game_cloneof(g, ft, 0))
	tree = tree_add_node(tree, game_cloneof(g, ft, 0), ft, 0);

    tree_add_node(tree, name, ft, 1);

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
tree_new(filetype_t ft)
{
    tree_t *t;

    t = xmalloc(sizeof(*t));

    t->name = NULL;
    t->check = 0;
    t->ft = ft;
    t->child = t->next = NULL;

    return t;
}



void
tree_traverse(DB *db, const tree_t *tree)
{
    tree_t *t;
    
    for (t=tree->child; t; t=t->next)
	tree_child_traverse(db, t, 0, NULL, NULL, 0, 0);

    return;
}



static array_t *
get_checked(const tree_t *t, const parray_t *pa)
{
    array_t *a;

    a = array_new_length(sizeof(int), parray_length(pa), set_zero);
    get_checked_r(t->child, a, pa);
    
    return a;
}



static void
get_checked_r(const tree_t *t, array_t *a, const parray_t *pa)
{
    int i;
    
    for (; t; t=t->next) {
	if ((i=parray_index_sorted(pa, tree_name(t), strcmp)) >= 0) 
	    *(int *)array_get(a, i) = 1;

	if (t->child)
	    get_checked_r(t->child, a, pa);
    }
}



static int
has_unused(archive_t *a)
{
    int i;

    if (a == NULL)
	return 0;

    for (i=0; i<archive_num_files(a); i++)
	if (rom_state(archive_file(a, i)) < ROM_TAKEN)
	    return 1;

    return 0;
}



static void
set_zero(int *ip)
{
    *ip = 0;
}



static tree_t *
tree_add_node(tree_t *tree, const char *name, filetype_t ft, int check)
{
    tree_t *t;
    int cmp;

    if (tree->child == NULL) {
	t = tree_new_full(name, ft, check);
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
	    t = tree_new_full(name, ft, check);
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
		    t = tree_new_full(name, ft, check);
		    t->next = tree->next;
		    tree->next = t;
		    return t;
		}
	    }

	    t = tree_new_full(name, ft, check);
	    tree->next = t;
	    return t;
	}
    }
}



static tree_t *
tree_new_full(const char *name, filetype_t ft, int check)
{
    tree_t *t;

    t = tree_new(ft);
    t->name = xstrdup(name);
    t->check = check;

    return t;
}

/* XXX: convert rest */



static int
tree_child_traverse(DB *db, const tree_t *tree, int parentcheck,
		    archive_t *parent_z, archive_t *gparent_z,
		    int parent_no, int gparent_no)
{
    int i;
    array_t *checked;
    tree_t *t;
    filetype_t ft;
    archive_t *child_z, *me_z, *all_z[3];
    game_t *child_g, *me_g;
    match_array_t *child_m, *me_m;
    match_disk_array_t *me_d;

    ft = tree->ft;

    me_z = archive_new(tree->name, tree->ft, NULL);

    me_m = NULL;
    me_g = NULL;

    /* check me */
    if (parentcheck || tree->check) {
	if ((me_g=r_game(db, tree->name)) == NULL) {
	    myerror(ERRDEF, "db error: %s not found", tree->name);
	    return -1;
	}
	all_z[0] = me_z;
	all_z[1] = parent_z;
	all_z[2] = gparent_z;
	me_m = check_roms(me_g, all_z, ft);
    }

    /* run through children & update zipstruct */
    for (t=tree->child; t; t=t->next) {
	/* XXX: init i */
	if (tree->check)
	    i = game_clone_index(me_g, ft, t->name);
	tree_child_traverse(db, t, tree->check, me_z, parent_z, i+1,
			    parent_no);
    }

    if ((parentcheck || tree->check)) {

	/* while files are left, check children until all files are needed
	   somewhere, or children run out */
	/* XXX: this comment seems wrong: also check children with check==1 */
	/* XXX: only compute checked if has_unuesd() */
	checked = get_checked(tree, game_clones(me_g, ft));
	for (i=0; has_unused(me_z) && (i<game_num_clones(me_g,ft)); i++) {
	    if (*(int *)array_get(checked, i) == 0) {
		if ((child_g=r_game(db, game_clone(me_g, ft, i))) == NULL) {
		    myerror(ERRDEF, "db error: %s not found",
			    game_clone(me_g, ft, i));
		    return -1;
		}
		child_z = archive_new(game_clone(me_g, ft, i), ft, NULL);
		
		all_z[0] = child_z;
		all_z[1] = me_z;
		all_z[2] = parent_z;
		child_m = check_roms(child_g, all_z, ft);
		match_merge(child_m, all_z, i+1, parent_no);
		/* XXX: fix if clone-fix forced */
		match_array_free(child_m);
		game_free(child_g);
		archive_free(child_z);
	    }
	}
	array_free(checked, NULL);
    
	all_z[0] = me_z;
	all_z[1] = parent_z;
	all_z[2] = gparent_z;

	match_merge(me_m, all_z, parent_no, gparent_no);

	/* check disks */
	if (ft != TYPE_SAMPLE)
	    me_d = check_disks(me_g);
	else
	    me_d = NULL;

	if (fix_do || fix_print) {
	    fix_game(me_g, ft, all_z, me_m);
	    me_z = all_z[0]; /* fix_do opens file if need be */
	}

	/* write warnings/errors for me */
	diagnostics(me_g, ft, me_m, me_d, (const archive_t **)all_z);
	
	/* clean up */
	match_array_free(me_m);
	match_disk_array_free(me_d);
	game_free(me_g);
    }

    archive_free(me_z);
    
    return 0;
}
