#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

int tree_add(DB *db, struct tree *tree, char *name);
struct tree *tree_add_node(struct tree *tree, char *name, int check);
struct tree *tree_new(char *name, int check);
void tree_free(struct tree *tree);
void tree_traverse(DB *db, struct tree *tree);
int tree_child_traverse(DB *db, struct tree *tree, int parentcheck,
			struct zip *parent_z, struct zip *gparent_z,
			int parent_no, int gparent_no);


#endif /* funcs.h */
