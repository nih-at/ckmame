/*
  $NiH: treetrav.c,v 1.19 2003/03/16 10:21:35 wiz Exp $

  treetrav.c -- traverse tree of games to check
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

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
#include "types.h"
#include "dbh.h"
#include "error.h"
#include "funcs.h"
#include "util.h"
#include "romutil.h"
#include "xmalloc.h"

static int tree_child_traverse(DB *db, struct tree *tree, int sample,
			       int parentcheck,
			       struct zfile *parent_z, struct zfile *gparent_z,
			       int parent_no, int gparent_no);
static struct tree *tree_add_node(struct tree *tree, char *name, int check);



void
tree_traverse(DB *db, struct tree *tree, int sample)
{
    struct tree *t;
    
    for (t=tree->child; t; t=t->next)
	tree_child_traverse(db, t, sample, 0, NULL, NULL, 0, 0);

    return;
}



static int
tree_child_traverse(DB *db, struct tree *tree, int sample, int parentcheck,
		    struct zfile *parent_z, struct zfile *gparent_z,
		    int parent_no, int gparent_no)
{
    int i;
    char **unchecked;
    struct tree *t;
    struct zfile *child_z, *me_z, *all_z[3];
    struct game *child_g, *me_g;
    struct match *child_m, *me_m;
    struct disk_match *me_d;

    me_z = zfile_new(tree->name, sample, NULL);

    me_m = NULL;
    me_g = NULL;

    /* check me */
    if (parentcheck || tree->check) {
	if ((me_g=r_game(db, tree->name)) == NULL) {
	    myerror(ERRDEF, "db error: %s not found", tree->name);
	    return -1;
	}
	if (sample)
	    game_swap_rs(me_g);
	all_z[0] = me_z;
	all_z[1] = parent_z;
	all_z[2] = gparent_z;
	me_m = check_game(me_g, all_z);
    }
    /* run through children & update zipstruct */
    for (t=tree->child; t; t=t->next) {
	if (tree->check) {
	    /* find out, which clone it is (by number) */
	    i =(int)((char **)bsearch(&(t->name), me_g->clone, me_g->nclone,
				      sizeof(char *),
				      (cmpfunc)strpcasecmp) - me_g->clone);
	}
	tree_child_traverse(db, t, sample, tree->check, me_z, parent_z, i+1,
			    parent_no);
    }

    if (parentcheck || tree->check) {
	/* set names of checked children and grandchildren in my list
	   of clones to NULL */
	unchecked = delchecked(tree, me_g->nclone, me_g->clone);

	/* while files are left, check children until all files are needed
	   somewhere, or children run out; also check children with check==1 */
	for (i=0; countunused(me_z) && (i<me_g->nclone); i++) {
	    if (unchecked[i]) {
		if ((child_g=r_game(db, me_g->clone[i])) == NULL) {
		    myerror(ERRDEF, "db error: %s not found", me_g->clone[i]);
		    return -1;
		}
		if (sample)
		    game_swap_rs(child_g);
		child_z = zfile_new(me_g->clone[i], sample, NULL);
		
		all_z[0] = child_z;
		all_z[1] = me_z;
		all_z[2] = parent_z;
		child_m = check_game(child_g, all_z);
		merge_match(child_m, child_g->nrom, all_z, i+1, parent_no);
		/* XXX: fix if clone-fix forced */
		match_free(child_m, child_g->nrom);
		game_free(child_g, 1);
		zfile_free(child_z);
	    }
	}
	free(unchecked);
    
	all_z[0] = me_z;
	all_z[1] = parent_z;
	all_z[2] = gparent_z;

	merge_match(me_m, me_g->nrom, all_z, parent_no, gparent_no);

	/* check disks */
	if (!sample) {
	    me_d = check_disks(me_g);
	}

	if (fix_do || fix_print) {
	    fix_game(me_g, all_z, me_m);
	    me_z = all_z[0]; /* fix_do opens file if need be */
	}

	/* write warnings/errors for me */
	diagnostics(me_g, me_m, me_d, all_z);
	
	/* clean up */
	match_free(me_m, me_g->nrom);
	disk_match_free(me_d, me_g->ndisk);
	game_free(me_g, 1);
    }

    zfile_free(me_z);
    
    return 0;
}



int
countunused(struct zfile *z)
{
    int i;

    if (z == NULL)
	return 0;

    for (i=0; i<z->nrom; i++)
	if (z->rom[i].state < ROM_TAKEN)
	    return 1;

    return 0;
}



int
tree_add(DB *db, struct tree *tree, char *name, int sample)
{
    struct game *g;

    if ((g=r_game(db, name)) == NULL)
	return -1;
    if (sample)
	game_swap_rs(g);

    if (g->cloneof[1])
	tree = tree_add_node(tree, g->cloneof[1], 0);
    if (g->cloneof[0])
	tree = tree_add_node(tree, g->cloneof[0], 0);

    tree_add_node(tree, name, 1);

    game_free(g, 1);
    
    return 0;
}



static struct tree *
tree_add_node(struct tree *tree, char *name, int check)
{
    struct tree *t;
    int cmp;

    if (tree->child == NULL) {
	t = tree_new(name, check);
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
	    t = tree_new(name, check);
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
		    t = tree_new(name, check);
		    t->next = tree->next;
		    tree->next = t;
		    return t;
		}
	    }

	    t = tree_new(name, check);
	    tree->next = t;
	    return t;
	}
    }
}



struct tree *
tree_new(char *name, int check)
{
    struct tree *t;

    t = (struct tree *)xmalloc(sizeof(struct tree));

    t->name = strdup(name);
    t->check = check;
    t->child = t->next = NULL;

    return t;
}



void
tree_free(struct tree *tree)
{
    struct tree *t;
    
    while (tree) {
	if (tree->child)
	    tree_free(tree->child);
	t = tree;
	tree = tree->next;
	free(t->name);
	free(t);
    }
}



struct zfile *
zfile_new(char *name, int sample, char *parent)
{
    struct zfile *z;
    char b[8192], *p;
    int i;
    
    z = (struct zfile *)xmalloc(sizeof(struct zfile));

    if (parent) {
	strcpy(b, parent);
	p = strrchr(b, '/');
	if (p == NULL)
	    p = b;
	else
	    p++;
	strcpy(p, name);
	strcat(p, ".zip");
	z->name = xstrdup(b);
    }
    else {
	z->name = findfile(name, sample ? TYPE_SAMPLE : TYPE_ROM);
	if (z->name == NULL) {
	    free(z);
	    return NULL;
	}
    }
    
    i = readinfosfromzip(z);
    if (i < 0) {
	/* XXX: error? */
    }

    for (i=0; i<z->nrom; i++) {
	z->rom[i].state = ROM_UNKNOWN;
	z->rom[i].where = -1;
    }

    return z;
}
