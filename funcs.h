#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

enum state romcmp(struct rom *r1, struct rom *r2);
int r_list(DB *db, char *key, char ***listp);
int w_list(DB *db, char *key, char **list, int n);
struct zip *zip_new(char *name);
void *xrealloc(void *p, size_t size);
void *xmalloc(size_t size);
void marry (struct match *rm, int count, int *noz);
struct match *check_game(struct game *game, struct zip **zip,
			 int pno, int gpno);
int matchcmp(struct match *m1, struct match *m2);
void diagnostics(struct game *game, struct match *m, struct zip **zip);
void match_free(struct match *m, int n);
struct game *r_game(DB *db, char *name);
int tree_add(DB *db, struct tree *tree, char *name);
struct tree *tree_add_node(struct tree *tree, char *name, int check);
struct tree *tree_new(char *name, int check);
void tree_free(struct tree *tree);
void tree_traverse(DB *db, struct tree *tree);
int tree_child_traverse(DB *db, struct tree *tree, int parentcheck,
			struct zip *parent_z, struct zip *gparent_z,
			int parent_no, int gparent_no);
int countunused(struct zip *z);
char *findzip(char *name);
void game_free(struct game *g, int fullp);
int strpcasecmp(char **sp1, char **sp2);
char **delchecked(struct tree *t, int nclone, char **clone);
void zip_free(struct zip *zip);
int w_game(DB *db, struct game *game);
unsigned long makencrc (char *zn, char *fn, int n);
int readinfosfromzip (struct rom **rompp, char *zipfile);

#endif /* funcs.h */
