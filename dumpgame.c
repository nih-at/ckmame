#include <stdio.h>
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbh.h"
#include "error.h"
#include "util.h"
#include "romutil.h"

int dump_game(DB *db, char *name);

char *prg;

static char *where_name[] = {
    "zip", "cloneof", "grand-cloneof"
};



int
main(int argc, char **argv)
{
    int i, j, optind, nlist, found, first;
    char *dbname;
    DB *db;
    char **list;
    
    prg = argv[0];

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = "mame";

    if ((db=db_open(dbname, 1, 0))==NULL) {
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
			(cmpfunc)strpcasecmp) != NULL) {
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
    printf("Description:\t%s\n", game->description);
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
	if (game->nclone % 6 != 0)
	    putc('\n', stdout);
    }
    printf("Roms:");
    for (i=0; i<game->nrom; i++) {
	printf("\t\tfile %-12s  size %6ld  crc %.8lx  in %s",
	       game->rom[i].name, game->rom[i].size, game->rom[i].crc,
	       where_name[game->rom[i].where]);
	if (game->rom[i].merge
	    && strcmp(game->rom[i].name, game->rom[i].merge) != 0)
	    printf(" (%s)", game->rom[i].merge);
	putc('\n', stdout);
    }
    if (game->sampleof[0])
	printf("Sampleof:\t%s\n", game->sampleof[0]);
    if (game->sampleof[1])
	printf("Grand-Sampleof:\t%s\n", game->sampleof[1]);
    if (game->nsclone) {
	printf("Sample Clones:");
	for (i=0; i<game->nsclone; i++) {
	    if (i%6 == 0)
		fputs("\t\t", stdout);
	    printf("%-8s ", game->sclone[i]);
	    if (i%6 == 5)
		putc('\n', stdout);
	}
	if (game->nsclone % 6 != 5)
	    putc('\n', stdout);
    }
    if (game->nsample) {
	printf("Samples:");
	for (i=0; i<game->nsample; i++)
	    printf("\t%s%s\n", (i==0 ? "" : "\t"), game->sample[i].name);
    }

    game_free(game, 1);

    return 0;
}
