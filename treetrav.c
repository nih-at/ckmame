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

int tree_child_traverse(DB *db, struct tree *tree, int parentcheck,
			struct zip *parent_z, struct zip *gparent_z,
			int parent_no, int gparent_no);
void tree_traverse(DB *db, struct tree *tree);
int countunused(struct zip *z);
int tree_add(DB *db, struct tree *tree, char *name);
struct tree *tree_add_node(struct tree *tree, char *name, int check);
struct tree *tree_new(char *name, int check);
void tree_free(struct tree *tree);



void
tree_traverse(DB *db, struct tree *tree)
{
    struct tree *t;
    
    for (t=tree->child; t; t=t->next)
	tree_child_traverse(db, t, 0, NULL, NULL, 0, 0);

    return;
}



int
tree_child_traverse(DB *db, struct tree *tree, int parentcheck,
		    struct zip *parent_z, struct zip *gparent_z,
		    int parent_no, int gparent_no)
{
    int i;
    char **unchecked;
    struct tree *t;
    struct zip *child_z, *me_z, *all_z[3];
    struct game *child_g, *me_g;
    struct match *child_m, *me_m;

    me_z = zip_new(tree->name);

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
	me_m = check_game(me_g, all_z, parent_no, gparent_no);
    }
    /* run through children & update zipstruct */
    for (t=tree->child; t; t=t->next) {
	if (tree->check) {
	    /* find out, which clone it is (by number) */
	    i =(int)((char **)bsearch(&(t->name), me_g->clone, me_g->nclone,
				      sizeof(char *),
				      (cmpfunc)strpcasecmp) - me_g->clone);
	}
	tree_child_traverse(db, t, tree->check, me_z, parent_z, i+1,
			    parent_no);
    }

    if (parentcheck || tree->check) {
	/* set names of checked childs and grandchildren in my list
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
		child_z = zip_new(me_g->clone[i]);
		
		all_z[0] = child_z;
		all_z[1] = me_z;
		all_z[2] = parent_z;
		child_m = check_game(child_g, all_z, i+1, parent_no);
		/* XXX: fix if clone-fix forced */
		merge_match(child_m, child_g->nrom, all_z, i+1, parent_no);
		match_free(child_m, child_g->nrom);
		game_free(child_g, 1);
		zip_free(child_z);
	    }
	}
	free(unchecked);
    
	all_z[0] = me_z;
	all_z[1] = parent_z;
	all_z[2] = gparent_z;

	/* XXX: fix */
	merge_match(me_m, me_g->nrom, all_z, parent_no, gparent_no);

	/* write warnings/errors for me */
	diagnostics(me_g, me_m, all_z);
	/* clean up */
	match_free(me_m, me_g->nrom);
	game_free(me_g, 1);
    }

    zip_free(me_z);
    
    return 0;
}



int
countunused(struct zip *z)
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
tree_add(DB *db, struct tree *tree, char *name)
{
    struct game *g;

    if ((g=r_game(db, name)) == NULL)
	return -1;

    if (g->cloneof[1])
	tree = tree_add_node(tree, g->cloneof[1], 0);
    if (g->cloneof[0])
	tree = tree_add_node(tree, g->cloneof[0], 0);

    tree_add_node(tree, name, 1);

    game_free(g, 1);
    
    return 0;
}



struct tree *
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



struct zip *
zip_new(char *name)
{
    struct zip *z;
    int i;
    
    z = (struct zip *)xmalloc(sizeof(struct zip));
    
    z->name = findzip(name);
    if (z->name == NULL) {
	free(z);
	return NULL;
    }
    
    z->nrom = readinfosfromzip(&(z->rom), z->name);
    if (z->nrom < 0)
	z->nrom = 0;

    for (i=0; i<z->nrom; i++)
	z->rom[i].state = ROM_UNKNOWN;

    return z;
}
