#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <getopt.h>

#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "error.h"
#include "r.h"

char *prg;

int output_options;

#define OPTIONS "hVnsfbcd"

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "nowarnings",    0, 0, 'n' }, /* -SUP, -FIX */
    { "nosuperfluous", 0, 0, 's' }, /* -SUP */
    { "nofixable",     0, 0, 'f' }, /* -FIX */
    { "nobroken",      0, 0, 'b' }, /* -BROKEN */
    { "nonogooddumps", 0, 0, 'd' }, /* -NO_GOOD_DUMPS */
    { "correct",       0, 0, 'c' }, /* +CORRECT */
    { NULL,            0, 0, 0 },
};



int
main(int argc, char **argv)
{
    int i, j;
    DB *db;
    char **list;
    int c, nlist;
    struct tree *tree;
    struct tree tree_root;
    
    prg = argv[0];
    tree = &tree_root;
    tree->child = NULL;
    output_options = WARN_ALL;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'h':
	    /* XXX: help */
	    break;
	case 'V':
	    /* XXX: version */
	    break;
	case 'n':
	    output_options &= WARN_BROKEN;
	    break;
	case 's':
	    output_options &= ~WARN_SUPERFLUOUS;
	    break;
	case 'f':
	    output_options &= ~WARN_FIXABLE;
	    break;
	case 'b':
	    output_options &= ~WARN_BROKEN;
	    break;
	case 'c':
	    output_options |= WARN_CORRECT;
	    break;
	case 'd':
	    output_options &= ~WARN_NO_GOOD_DUMP;
	    break;
	default:
	    myerror(ERRSTR, "unknown option");
	    exit(1);
	}
    }
    
    if ((db=db_open("mame", 1, 0))==NULL) {
	myerror(ERRSTR, "can't open database `mame.db'");
	exit(1);
    }

    nlist = r_list(db, "/list", &list);

    if (optind == argc) {
	for (i=0; i<nlist; i++)
	    tree_add(db, tree, list[i]);
    }
    else {
	for (i=optind; i<argc; i++) {
	    if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
		if (bsearch(argv+i, list, nlist, sizeof(char *),
			    strpcasecmp) != NULL)
		    tree_add(db, tree, argv[i]);
	    }
	    else {
		for (j=0; j<nlist; j++) {
		    if (fnmatch(argv[i], list[j], 0) == 0)
			tree_add(db, tree, list[j]);
		}
	    }
	}
    }

    tree_traverse(db, tree);

    return 0;
}
