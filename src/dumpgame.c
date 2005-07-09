/*
  $NiH: dumpgame.c,v 1.2 2005/07/04 22:41:35 dillo Exp $

  dumpgame.c -- print info about game (from data base)
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

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
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "types.h"
#include "dbh.h"
#include "error.h"
#include "util.h"
#include "romutil.h"
#include "hashes.h"
#include "xmalloc.h"

static int dump_game(DB *, const char *);
static int dump_hashtypes(DB *, const char *);
static int dump_list(DB *, const char *);
static int dump_prog(DB *, const char *);
static int dump_db_version(DB *, const char *);
static int dump_special(DB *, const char *);
static void print_hashtypes(int);

char *prg;
char *usage = "Usage: %s [-h|-V]\n\
       %s [-D dbfile] [game ...]\n\
       %s [-D dbfile] [-c | -d] [checksum ...]\n";

char help_head[] = "dumpgame (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -c, --checksum       find games containing ROMs with given checksums\n\
  -d, --disk           find games containing disks with given checksums\n\
  -D, --db dbfile      use database dbfile\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "dumpgame (" PACKAGE " " VERSION ")\n\
Copyright (C) 2005 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hcdD:V"

struct option options[] = {
    { "checksum",      0, 0, 'c' },
    { "db",            1, 0, 'D' },
    { "disk",          0, 0, 'd' },
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { NULL,            0, 0, 0 },
};

static char *where_name[] = {
    "zip", "cloneof", "grand-cloneof"
};

static char *flags_name[] = {
    "ok", "baddump", "nogooddump"
};

parray_t *list;



static void
print_checksums(struct hashes *hashes)
{
    int i;
    char h[HASHES_SIZE_MAX*2 + 1];

    for (i=1; i<=HASHES_TYPE_MAX; i<<=1) {
	if (hashes_has_type(hashes, i)) {
	    printf(" %s %s", hash_type_string(i),
		   hash_to_string(h, i, hashes));
	}
    }
}



static void
print_diskline(struct disk *disk)
{
    printf("\t\tdisk %-12s", disk->name);
    print_checksums(&disk->hashes);
    putc('\n', stdout);
}



static void
print_footer(int matches, struct hashes *hash)
{
    printf("%d matches found for checksum", matches);
    print_checksums(hash);
    putc('\n', stdout);
}



static void
print_romline(struct rom *rom)
{
    printf("\t\tfile %-12s  size %7ld",
	   rom->name, rom->size);
    print_checksums(&rom->hashes);
    printf("  flags %s  in %s",
	   flags_name[rom->flags], where_name[rom->where]);
    if (rom->merge && strcmp(rom->name, rom->merge) != 0)
	printf(" (%s)", rom->merge);
    putc('\n', stdout);
}



static void
print_match(struct game *game, enum filetype ft, int i)
{
    static char *name = NULL;

    if (name == NULL || strcmp(game->name, name) != 0) {
	free(name);
	name = xstrdup(game->name);

	printf("In game %s:\n", game->name);
    }

    if (ft == TYPE_DISK)
	print_diskline(game->disk+i);
    else
	print_romline(game->rom+i);
}



static void
print_matches(DB *db, filetype_t ft, hashes_t *hash)
{
    game_t *game;
    int i, j, matches;
    file_by_hash_t *fbh;

    matches = 0;

    if (hash->types == file_by_hash_default_hashtype(ft)) {
	if ((fbh=r_file_by_hash(db, ft, hash)) == NULL) {
	    print_footer(0, hash);
	    return;
	}

	for (i=0; i<file_by_hash_length(fbh); i++) {
	    if ((game=r_game(db, fbh->entry[i].game)) == NULL) {
		myerror(ERRDEF,
			"db error: %s not found, though in hash index",
			fbh->entry[i].game);
		/* XXX: remember error */
		continue;
	    }

	    print_match(game, ft, fbh->entry[i].index);
	    matches++;
	    
	    game_free(game, 1);
	}

	file_by_hash_free(fbh);
    }
    else {
	for (i=0; i<parray_length(list); i++) {
	    if ((game=r_game(db, parray_get(list, i))) == NULL) {
		myerror(ERRDEF,
			"db error: %s not found, though in list",
			parray_get(list, i));
		/* XXX: remember error */
		continue;
	    }

	    if (ft == TYPE_ROM) {
		for (j=0; j<game->nrom; j++) {
		    if (hashes_cmp(&game->rom[j].hashes,
				   hash) == HASHES_CMP_MATCH) {
			print_match(game, ft, j);
			matches++;
		    }
		}
	    }
	    else if (ft == TYPE_DISK) {
		for (j=0; j<game->ndisk; j++) {
		    if (hashes_cmp(&game->disk[j].hashes,
				   hash) == HASHES_CMP_MATCH) {
			print_match(game, ft, j);
			matches++;
		    }
		}
	    }
	    game_free(game, 1);
	}
    }

    print_footer(matches, hash);
}



int
main(int argc, char **argv)
{
    int i, j, found, first;
    char *dbname;
    DB *db;
    int c;
    int type;
    int find_checksum;
    filetype_t filetype;

    prg = argv[0];

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DDB_DEFAULT_DB_NAME;

    find_checksum = 0;
    type = HASHES_TYPE_CRC;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'c':
	    find_checksum = 1;
	    filetype = TYPE_ROM;
	    break;
	case 'd':
	    find_checksum = 1;
	    filetype = TYPE_DISK;
	    break;
	case 'D':
	    dbname = optarg;
	    break;
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, prg, prg, prg);
	    fputs(help, stdout);
	    exit(0);
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);
    	default:
	    fprintf(stderr, usage, prg, prg, prg);
	    exit(1);
	}
    }

    if ((db=ddb_open(dbname, DDB_READ))==NULL) {
	myerror(ERRSTR, "can't open database `%s'", dbname);
	exit (1);
    }

    if ((list=r_list(db, DDB_KEY_LIST_GAME)) < 0) {
	myerror(ERRDEF, "list of games not found in database '%s'", dbname);
	exit(1);
    }

    /* find matches for roms */
    if (find_checksum != 0) {
	struct hashes match;

	for (i=optind; i<argc; i++) {
	    /* checksum */
	    if ((hash_from_string(&match, argv[i])) == -1) {
		fprintf(stderr, "error parsing checksum `%s'\n", argv[i]);
		exit(2);
	    }

	    print_matches(db, filetype, &match);
	}
	exit(0);
    }

    first = 1;
    for (i=optind; i<argc; i++) {
	if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
	    if (argv[i][0] == '/') {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_special(db, argv[i]);
	    }
	    else if (parray_bsearch(list, argv+i,
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
	    for (j=0; j<parray_length(list); j++) {
		if (fnmatch(argv[i], parray_get(list, j), 0) == 0) {
		    if (first)
			first = 0;
		    else
			putc('\n', stdout);
		    dump_game(db, parray_get(list, j));
		    found = 1;
		}
	    }
	    if (!found)
		myerror(ERRDEF, "no game matching `%s' found", argv[i]);
	}
    }

    parray_free(list, free);
    
    return 0;
}



int
dump_game(DB *db, const char *name)
{
    int i, j;
    struct game *game;

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or database error): %s", name);
	return -1;
    }

    /* XXX: use print_* functions */
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
	print_romline(game->rom+i);
	for (j=0; j < game->rom[i].naltname; j++) {
	    /* XXX: check hashes.types */
	    printf("\t\tfile %-12s  size %7ld  crc %.8lx  flags %s  in %s",
		   game->rom[i].altname[j], game->rom[i].size,
		   game->rom[i].hashes.crc, flags_name[game->rom[i].flags],
		   where_name[game->rom[i].where]);
	    if (game->rom[i].merge) {
		if (strcmp(game->rom[i].altname[j], game->rom[i].merge) != 0)
		    printf(" (%s)", game->rom[i].merge);
	    } else
		printf(" (%s)", game->rom[i].name);
	    putc('\n', stdout);
	}
    }
    if (game->sampleof[0])
	printf("Sampleof:\t%s\n", game->sampleof[0]);
    if (game->sampleof[1])
	printf("Grand-Sampleof:\t%s\n", game->sampleof[1]);
    if (game->nsclone) {
	printf("Sample Clones:");
	for (i=0; i<game->nsclone; i++) {
	    if (i%6 == 0)
		printf("%s\t", (i==0 ? "" : "\t"));
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
	    printf("\t%sfile %-12s  in %s\n",
		   (i==0 ? "" : "\t"),
		   game->sample[i].name,
		   where_name[game->sample[i].where]);
    }
    if (game->ndisk) {
	printf("Disks:");
	for (i=0; i<game->ndisk; i++) {
	    printf("\t\tdisk %-12s", game->disk[i].name);
	    print_checksums(&game->disk[i].hashes);
	    putc('\n', stdout);
	}
    }
    
    game_free(game, 1);

    return 0;
}



static int
dump_hashtypes(DB *db, const char *dummy)
{
    int romhashtypes, diskhashtypes;

    if (r_hashtypes(db, &romhashtypes, &diskhashtypes) < 0) {
	myerror(ERRDEF, "db error reading hashtypes");
	return -1;
    }
    printf("roms: ");
    print_hashtypes(romhashtypes);
    printf("\ndisks: ");
    print_hashtypes(diskhashtypes);
    putc('\n', stdout);

    return 0;
}



static int
dump_list(DB *db, const char *key)
{
    int i;
    parray_t *list;

    if ((list=r_list(db, key)) == NULL) {
	myerror(ERRDEF, "db error reading list %s", key);
	return -1;
    }

    for (i=0; i<parray_length(list); i++)
	printf("%s\n", (char *)parray_get(list, i));

    parray_free(list, free);

    return 0;
}



static int
dump_prog(DB *db, const char *dummy)
{
    char *name, *version;

    if (r_prog(db, &name, &version) < 0) {
	myerror(ERRDEF, "db error reading /prog");
	return -1;
    }

    printf("%s (%s)\n",
	   name ? name : "unknown",
	   version ? version: "unknown");
    free(name);
    free(version);

    return 0;
}



static int
dump_db_version(DB *db, const char *dummy)
{
    /* ddb_open won't let us open a db with a different version */
    printf("%d\n", DDB_FORMAT_VERSION);

    return 0;
}



static int
dump_special(DB *db, const char *name)
{
    static const struct {
	const char *key;
	int (*f)(DB *, const char *);
	const char *arg_override;
    } keys[] = {
	{ "/list",             dump_list,       DDB_KEY_LIST_GAME },
	{ DDB_KEY_DB_VERSION,  dump_db_version, NULL },
	{ DDB_KEY_HASH_TYPES,  dump_hashtypes,  NULL },
	{ DDB_KEY_LIST_DISK,   dump_list,       NULL },
	{ DDB_KEY_LIST_GAME,   dump_list,       NULL },
	{ DDB_KEY_LIST_SAMPLE, dump_list,       NULL },
	{ DDB_KEY_PROG,        dump_prog,       NULL }
    };

    int i;

    for (i=0; i<sizeof(keys)/sizeof(keys[0]); i++) {
	if (strcasecmp(name, keys[i].key) == 0)
	    return keys[i].f(db, (keys[i].arg_override ? keys[i].arg_override
				  : name));
    }
    
    myerror(ERRDEF, "unknown special: %s", name);
    return -1;
}



#define DO(ht, x, s)	(((ht) & (x)) ?					   \
			 printf("%s%s", (first ? first=0, "" : ", "), (s)) \
			 : 0)
				

static void
print_hashtypes(int ht)
{
    int first;

    first = 1;

    DO(ht, HASHES_TYPE_CRC, "crc");
    DO(ht, HASHES_TYPE_MD5, "md5");
    DO(ht, HASHES_TYPE_SHA1, "sha1");
}