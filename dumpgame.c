/*
  $NiH: dumpgame.c,v 1.37 2005/06/12 15:18:45 wiz Exp $

  dumpgame.c -- print info about game (from data base)
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

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
#include <xmalloc.h>

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

static int dump_game(DB *, const char *);
static int dump_hashtypes(DB *);
static int dump_list(DB *, const char *);
static int dump_prog(DB *);
static int dump_special(DB *, const char *);
static void print_hashtypes(int);

char *prg;
char *usage = "Usage: %s [-h|-V]\n\
       %s [-D dbfile] [game ...]\n\
       %s -c [-D dbfile] [-t type] [checksum ...]\n";

char help_head[] = "dumpgame (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -c, --checksum       find games containing roms with given checksums\n\
  -D, --db dbfile      use database dbfile\n\
  -h, --help           display this help message\n\
  -t, --type type      checksum type (crc32 (default), md5, sha1)\n\
  -V, --version        display version number\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "dumpgame (" PACKAGE " " VERSION ")\n\
Copyright (C) 2004 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hcD:t:V"

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "checksum",      0, 0, 'c' },
    { "db",            1, 0, 'D' },
    { "type",          1, 0, 't' },
    { NULL,            0, 0, 0 },
};

static char *where_name[] = {
    "zip", "cloneof", "grand-cloneof"
};

static char *flags_name[] = {
    "ok", "baddump", "nogooddump"
};



static struct hashes *
get_checksum(int type, char *checksumstr)
{
    struct hashes *newhash;

    if ((newhash=xmalloc(sizeof(struct hashes))) == NULL)
	return NULL;

    newhash->types = type;
    switch(type) {
    case GOT_CRC:
	newhash->crc = strtoul(checksumstr, NULL, 16);
	break;
    case GOT_MD5:
	if (hex2bin(newhash->md5, checksumstr,
		    sizeof(newhash->md5)) != 0) {
	    fprintf(stderr, "invalid argument for md5: %s\n",
		    checksumstr);
	    free(newhash);
	    return NULL;
	}
	break;
    case GOT_SHA1:
	if (hex2bin(newhash->sha1, checksumstr,
		    sizeof(newhash->sha1)) != 0) {
	    fprintf(stderr, "invalid argument for sha1: %s\n",
		    checksumstr);
	    free(newhash);
	    return NULL;
	}
	break;
    default:
	return NULL;
    }

    return newhash;
}



static int
parse_type(char *typestr)
{
    if (strcasecmp(typestr, "crc") == 0||
	strcasecmp(typestr, "crc32") == 0)
	return GOT_CRC;
    else if (strcasecmp(typestr, "md5") == 0)
	return GOT_MD5;
    else if (strcasecmp(typestr, "sha1") == 0)
	return GOT_SHA1;
    else
	return -1;
}



static void
print_checksums(struct hashes *hashes)
{
    if (hashes->types & GOT_CRC)
	printf("  crc %.8lx", hashes->crc);
    if (hashes->types & GOT_MD5)
	printf("  md5 %s", bin2hex(hashes->md5,
				   sizeof(hashes->md5)));
    if (hashes->types & GOT_SHA1)
	printf("  sha1 %s", bin2hex(hashes->sha1,
				    sizeof(hashes->sha1)));
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
print_match(struct game *game, int i)
{
    static int first = 0;
    static char *name = NULL;

    if (name == NULL || strcmp(game->name, name) != 0) {
	first = 1;
	free(name);
	name = xstrdup(game->name);
    }

    if (first) {
	printf("In game %s:\n", game->name);
	first = 0;
    }

    print_romline(game->rom+i);
}



static int
match_checksum(DB *db, char *name, struct hashes *hashes)
{
    struct game *game;
    int i, matches;

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "db error: %s not found, though in list of games",
		name);
	exit(-1);
    }

    matches = 0;
    for (i=0; i<game->nrom; i++) {
	if ((hashes->types & game->rom[i].hashes.types) == 0)
	    continue;
	switch(hashes->types) {
	case GOT_CRC:
	    if (game->rom[i].hashes.crc == hashes->crc) {
		matches++;
		print_match(game, i);
	    }
	    break;
	case GOT_MD5:
	    if (memcmp(game->rom[i].hashes.md5, hashes->md5,
			   sizeof(hashes->md5)) == 0) {
		matches++;
		print_match(game, i);
	    }
	    break;
	case GOT_SHA1:
	    if (memcmp(game->rom[i].hashes.sha1, hashes->sha1,
			   sizeof(hashes->sha1)) == 0) {
		matches++;
		print_match(game, i);
	    }
	    break;
	default:
	    break;
	}
    }

    /* XXX: disk matches */

    game_free(game, 1);

    return matches;
}


int
main(int argc, char **argv)
{
    int i, j, nlist, found, first;
    char *dbname;
    DB *db;
    char **list;
    int dbext;
    int c;
    int type;
    int find_checksum;
    
    prg = argv[0];

    dbext = 0;
    dbname = getenv("MAMEDB");
    if (dbname == NULL) {
	dbname = "mame";
	dbext = DDB_EXT;
    }

    find_checksum = 0;
    type = GOT_CRC;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'c':
	    find_checksum = 1;
	    break;
	case 'D':
	    dbname = optarg;
	    dbext = 0;
	    break;
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, prg, prg, prg);
	    fputs(help, stdout);
	    exit(0);
	case 't':
	    if ((type=parse_type(optarg)) == -1) {
		fprintf(stderr, "unknown checksum type `%s'\n",
			optarg);
		exit(1);
	    }
	    break;
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);
    	default:
	    fprintf(stderr, usage, prg, prg, prg);
	    exit(1);
	}
    }

    if ((db=ddb_open(dbname, DDB_READ|dbext))==NULL) {
	myerror(ERRSTR, "can't open database `%s'", dbname);
	exit (1);
    }

    if ((nlist=r_list(db, "/list", &list)) < 0) {
	myerror(ERRDEF, "list of games not found in database '%s'", dbname);
	exit(1);
    }

    /* find matches for roms */
    if (find_checksum != 0) {
	int matches;
	struct hashes *match;

	for (i=optind; i<argc; i++) {
	    /* checksum */
	    if ((match=get_checksum(type, argv[i])) == NULL) {
		fprintf(stderr, "error parsing checksum `%s'\n", argv[i]);
		exit(2);
	    }

	    matches = 0;
	    for (j=0; j<nlist; j++)
		matches += match_checksum(db, list[j], match);

	    printf("%d matches found for checksum", matches);
	    print_checksums(match);
	    putc('\n', stdout);
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
	    else if (bsearch(argv+i, list, nlist, sizeof(char *),
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
dump_game(DB *db, const char *name)
{
    int i, j;
    struct game *game;

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or database error): %s", name);
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
dump_hashtypes(DB *db)
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
    int i, n;
    char **l;

    if ((n=r_list(db, key, &l)) < 0) {
	myerror(ERRDEF, "db error reading list %s", key);
	return -1;
    }

    for (i=0; i<n; i++) {
	printf("%s\n", l[i]);
	free(l[i]);
    }
    free(l);

    return 0;
}



static int
dump_prog(DB *db)
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
dump_special(DB *db, const char *name)
{
    if (strcmp(name, "/prog") == 0)
	return dump_prog(db);
    else if (strcmp(name, "/list") == 0
	     || strcmp(name, "/sample_list") == 0
	     || strcmp(name, "/extra_list") == 0)
	return dump_list(db, name);
    else if (strcmp(name, "/hashtypes") == 0)
	return dump_hashtypes(db);
    else {
	myerror(ERRDEF, "unknown special: %s", name);
	return -1;
    }
}



#define DO(ht, x, s)	(((ht) & (x)) ?					   \
			 printf("%s%s", (first ? first=0, "" : ", "), (s)) \
			 : 0)
				

static void
print_hashtypes(int ht)
{
    int first;

    first = 1;

    DO(ht, GOT_CRC, "crc");
    DO(ht, GOT_MD5, "md5");
    DO(ht, GOT_SHA1, "sha1");
}
