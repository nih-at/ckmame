#include <stdio.h>
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbl.h"
#include "error.h"
#include "funcs.h"
#include "r.h"

int dump_game(DB *db, char *name);

char *prg;

static char *where_name[] = {
    "zip", "cloneof", "grand-cloneof"
};



int
main(int argc, char **argv)
{
    int i, j, optind, nlist, found, first;
    DB *db;
    char **list;
    
    prg = argv[0];

    if ((db=db_open("mame", 1, 0))==NULL) {
	myerror(ERRSTR, "can't open database `mame.db'");
	exit (1);
    }

    if ((nlist=r_list(db, "/list", &list)) < 0) {
	myerror(ERRDEF, "list of games not found in database");
	exit(1);
    }

    optind = 1;

    first = 1;
    for (i=optind; i<argc; i++) {
	if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
	    if (bsearch(argv+i, list, nlist, sizeof(char *),
			strpcasecmp) != NULL) {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_game(db, argv[i]);
	    }
	    else
		myerror(ERRDEF, "game `%s' unknown", argv[i]);
	}
	else {
	    found = 0;
	    for (j=0; j<nlist; j++) {
		if (fnmatch(argv[i], list[j], 0) == 0) {
		    if (first)
			first = 0;
		    else
			putc('\n', stdout);
		    dump_game(db, list[j]);
		    found = 1;
		}
	    }
	    if (!found)
		myerror(ERRDEF, "no game matching `%s' found", argv[i]);
	}
    }


    return 0;
}



int
dump_game(DB *db, char *name)
{
    int i;
    struct game *game;

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or db error): %s", name);
	return -1;
    }
    
    printf("Name:\t\t%s\n", game->name);
    if (game->cloneof[0])
	printf("Cloneof:\t%s\n", game->cloneof[0]);
    if (game->cloneof[1])
	printf("Grand-Cloneof:\t%s\n", game->cloneof[1]);
    if (game->nclone) {
	printf("Clones:");
	for (i=0; i<game->nclone; i++) {
	    if (i%6 == 0)
		fputs("\t\t", stdout);
	    printf("%-8s ", game->clone[i]);
	    if (i%6 == 5)
		putc('\n', stdout);
	}
	if (game->nclone % 6 != 5)
	    putc('\n', stdout);
    }
    printf("Roms:");
    for (i=0; i<game->nrom; i++)
	printf("\t\tfile %-12s  size %6ld  crc %.8lx  in %s\n",
	       game->rom[i].name, game->rom[i].size, game->rom[i].crc,
	       where_name[game->rom[i].where]);
    if (game->sampleof)
	printf("Sampleof:\t%s\n", game->sampleof);
    if (game->nsample) {
	printf("Samples:");
	for (i=0; i<game->nsample; i++)
	    printf("\t%s%s\n", (i==0 ? "" : "\t"), game->sample[i].name);
    }

    game_free(game, 1);

    return 0;
}
