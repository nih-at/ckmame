#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

int tree_add(DB *db, struct tree *tree, char *name, int sample);
struct tree *tree_new(char *name, int check);
void tree_free(struct tree *tree);
void tree_traverse(DB *db, struct tree *tree, int sample);

#endif /* funcs.h */
