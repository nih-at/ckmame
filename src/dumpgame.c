/*
  $NiH: dumpgame.c,v 1.20 2007/04/10 19:49:49 dillo Exp $

  dumpgame.c -- print info about game (from data base)
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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

#include "dat.h"
#include "dbh.h"
#include "error.h"
#include "file_location.h"
#include "hashes.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static int dump_game(sqlite3 *, const char *, int);
static int dump_hashtypes(sqlite3 *, const char *);
static int dump_list(sqlite3 *, const char *);
static int dump_dat(sqlite3 *, const char *);
static int dump_db_version(sqlite3 *, const char *);
static int dump_special(sqlite3 *, const char *);
static int dump_stats(sqlite3 *, const char *);
static void print_dat(dat_t *, int);
static void print_hashtypes(int);
static void print_rs(game_t *, filetype_t, const char *,
		     const char *, const char *, const char *);

char *usage = "Usage: %s [-h|-V]\n\
       %s [-b] [-D dbfile] [game ...]\n\
       %s [-b] [-D dbfile] [-c | -d] [checksum ...]\n";

char help_head[] = "dumpgame (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -b, --brief          brief listing (omit ROM details)\n\
  -c, --checksum       find games containing ROMs with given checksums\n\
  -D, --db dbfile      use database dbfile\n\
  -d, --disk           find games containing disks with given checksums\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "dumpgame (" PACKAGE " " VERSION ")\n\
Copyright (C) 2007 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hbcD:dV"

struct option options[] = {
    { "brief",         0, 0, 'b' },
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

static char *status_name[] = {
    "ok", "baddump", "nogooddump"
};

parray_t *list;



static void
print_checksums(hashes_t *hashes)
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
print_diskline(disk_t *disk)
{
    printf("\t\tdisk %-12s", disk->name);
    print_checksums(&disk->hashes);
    printf(" status %s", status_name[disk_status(disk)]);
    putc('\n', stdout);
}



static void
print_footer(int matches, hashes_t *hash)
{
    printf("%d matches found for checksum", matches);
    print_checksums(hash);
    putc('\n', stdout);
}



static void
print_romline(rom_t *rom)
{
    printf("\t\tfile %-12s  size %7ld",
	   rom_name(rom), rom_size(rom));
    print_checksums(rom_hashes(rom));
    printf(" status %s in %s",
	   status_name[rom_status(rom)], where_name[rom_where(rom)]);
    if (rom_merge(rom) && strcmp(rom_name(rom), rom_merge(rom)) != 0)
	printf(" (%s)", rom_merge(rom));
    putc('\n', stdout);
}



static void
print_match(game_t *game, filetype_t ft, int i)
{
    static char *name = NULL;

    if (name == NULL || strcmp(game_name(game), name) != 0) {
	free(name);
	name = xstrdup(game_name(game));

	printf("In game %s:\n", game_name(game));
    }

    if (ft == TYPE_DISK)
	print_diskline(game_disk(game, i));
    else
	print_romline(game_file(game, TYPE_ROM, i));
}



static void
print_matches(sqlite3 *db, filetype_t ft, hashes_t *hash)
{
    game_t *game;
    int i, j, matches;
    array_t *fbha;
    file_location_t *fbh;

    matches = 0;

    if (hashes_has_type(hash, file_location_default_hashtype(ft))) {
	if ((fbha=r_file_by_hash(db, ft, hash)) == NULL) {
	    print_footer(0, hash);
	    return;
	}

	for (i=0; i<array_length(fbha); i++) {
	    fbh = array_get(fbha, i);
	    if ((game=r_game(db, file_location_name(fbh))) == NULL) {
		myerror(ERRDEF,
			"db error: %s not found, though in hash index",
			file_location_name(fbh));
		/* XXX: remember error */
		continue;
	    }

	    print_match(game, ft, file_location_index(fbh));
	    matches++;
	    
	    game_free(game);
	}

	array_free(fbha, file_location_finalize);
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
		for (j=0; j<game_num_files(game, ft); j++) {
		    if (hashes_cmp(rom_hashes(game_file(game, ft, j)),
				   hash) == HASHES_CMP_MATCH) {
			print_match(game, ft, j);
			matches++;
		    }
		}
	    }
	    else if (ft == TYPE_DISK) {
		for (j=0; j<game_num_disks(game); j++) {
		    if (hashes_cmp(disk_hashes(game_disk(game, j)),
				   hash) == HASHES_CMP_MATCH) {
			print_match(game, ft, j);
			matches++;
		    }
		}
	    }
	    game_free(game);
	}
    }

    print_footer(matches, hash);
}



int
main(int argc, char **argv)
{
    int i, j, found, first;
    char *dbname;
    sqlite3 *db;
    int c;
    int find_checksum, brief_mode;
    filetype_t filetype;

    setprogname(argv[0]);

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;

    find_checksum = brief_mode = 0;

    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'b':
	    brief_mode = 1;
	    break;
	case 'c':
	    find_checksum = 1;
	    filetype = TYPE_ROM;
	    break;
	case 'D':
	    dbname = optarg;
	    break;
	case 'd':
	    find_checksum = 1;
	    filetype = TYPE_DISK;
	    break;
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, getprogname(), getprogname(), getprogname());
	    fputs(help, stdout);
	    exit(0);
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);
    	default:
	    fprintf(stderr, usage,
		    getprogname(), getprogname(), getprogname());
	    exit(1);
	}
    }

    if ((db=dbh_open(dbname, DBL_READ))==NULL) {
	myerror(ERRSTR, "can't open database `%s'", dbname);
	exit (1);
    }

    if ((list=r_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "list of games not found in database '%s'", dbname);
	exit(1);
    }

    /* find matches for ROMs */
    if (find_checksum != 0) {
	hashes_t match;

	for (i=optind; i<argc; i++) {
	    /* checksum */
	    hashes_init(&match);
	    if ((hash_from_string(&match, argv[i])) == -1) {
		myerror(ERRDEF, "error parsing checksum `%s'", argv[i]);
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
	    else if (parray_index_sorted(list, argv[i], strcmp) >= 0) {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_game(db, argv[i], brief_mode);
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
		    dump_game(db, parray_get(list, j), brief_mode);
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



static void
print_rs(game_t *game, filetype_t ft,
	const char *co, const char *gco, const char *cs, const char *fs) 
{
    int i, j;
    rom_t *r;

    if (game_cloneof(game, ft, 0))
	printf("%s:\t%s\n", co, game_cloneof(game, ft, 0));
    if (game_cloneof(game, ft, 1))
	printf("%s:\t%s\n", gco, game_cloneof(game, ft, 1));
    
    if (game_num_clones(game, ft) > 0) {
	printf("%s", cs);
	for (i=0; i<game_num_clones(game, ft); i++) {
	    if (i%6 == 0)
		fputs("\t\t", stdout);
	    printf("%-8s ", game_clone(game, ft, i));
	    if (i%6 == 5)
		putc('\n', stdout);
	}
	if (game_num_clones(game, ft) % 6 != 0)
	    putc('\n', stdout);
    }
    if (game_num_files(game, ft) > 0) {
	printf("%s:\n", fs);
	for (i=0; i<game_num_files(game, ft); i++) {
	    r = game_file(game, ft, i);
	    print_romline(r);
	    for (j=0; j<rom_num_altnames(r); j++) {
		/* XXX: check hashes.types */
		printf("\t\tfile %-12s  size %7ld  crc %.8lx  status %s in %s",
		       rom_altname(r, j), rom_size(r), rom_hashes(r)->crc,
		       status_name[rom_status(r)], where_name[rom_where(r)]);
		if (rom_merge(r)) {
		    if (strcmp(rom_altname(r, j), rom_merge(r)) != 0)
			printf(" (%s)", rom_merge(r));
		} else
		    printf(" (%s)", rom_name(r));
		putc('\n', stdout);
	    }
	}
    }
}


static int
dump_game(sqlite3 *db, const char *name, int brief_mode)
{
    int i;
    game_t *game;
    dat_t *dat;

    if ((dat=r_dat(db)) == NULL) {
	myerror(ERRDEF, "cannot read dat info");
	return -1;
    }

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or database error): `%s'", name);
	return -1;
    }

    /* XXX: use print_* functions */
    printf("Name:\t\t%s\n", game->name);
    if (dat_length(dat) > 1) {
	printf("Source:\t\t");
	print_dat(dat, game_dat_no(game));
	putc('\n', stdout);
    }
    if (game_description(game))
	printf("Description:\t%s\n", game_description(game));

    if (!brief_mode) {
	print_rs(game, TYPE_ROM, "Cloneof", "Grand-Cloneof", "Clones", "ROMs");
	print_rs(game, TYPE_SAMPLE, "Sampleof", "Grand-Sampleof",
		 "Sample Clones", "Samples");
    
	if (game_num_disks(game) > 0) {
	    printf("Disks:\n");
	    for (i=0; i<game_num_disks(game); i++)
		print_diskline(game_disk(game, i));
	}
    }
    
    game_free(game);

    return 0;
}



/*ARGSUSED2*/
static int
dump_hashtypes(sqlite3 *db, const char *dummy)
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
dump_list(sqlite3 *db, const char *key)
{
    int i;
    parray_t *list;

    if ((list=r_list(db, key)) == NULL) {
	myerror(ERRDEF, "db error reading list `%s'", key);
	return -1;
    }

    for (i=0; i<parray_length(list); i++)
	printf("%s\n", (char *)parray_get(list, i));

    parray_free(list, free);

    return 0;
}



/*ARGSUSED2*/
static int
dump_dat(sqlite3 *db, const char *dummy)
{
    dat_t *d;
    int i;

    if ((d=r_dat(db)) == NULL) {
	myerror(ERRDEF, "db error reading /dat");
	return -1;
    }

    for (i=0; i<dat_length(d); i++) {
	if (dat_length(d) > 1)
	    printf("%2d: ", i);
	print_dat(d, i);
	putc('\n', stdout);
    }
    
    return 0;
}



/*ARGSUSED2*/
static int
dump_db_version(sqlite3 *db, const char *dummy)
{
    /* dbh_open won't let us open a db with a different version */
    printf("%d\n", DBH_FORMAT_VERSION);

    return 0;
}



/*ARGSUSED2*/
static int
dump_detector(sqlite3 *db, const char *dummy)
{
    detector_t *d;
    
    if ((d=r_detector(db)) != NULL) {
	printf("%s", detector_name(d));
	if (detector_version(d))
	    printf(" (%s)", detector_version(d));
	printf("\n");
	detector_free(d);
    }
    
    return 0;
}



static int
dump_special(sqlite3 *db, const char *name)
{
    static const struct {
	const char *key;
	int (*f)(sqlite3 *, const char *);
	const char *arg_override;
    } keys[] = {
	{ "/list",             dump_list,       DBH_KEY_LIST_GAME },
	{ "/dat",              dump_dat,        NULL },
	{ "/ckmame",           dump_db_version, NULL },
	{ "/detector",         dump_detector,   NULL },
	{ "/hashtypes",        dump_hashtypes,  NULL },
	{ "/list/disk",        dump_list,       NULL },
	{ "/list/game",        dump_list,       NULL },
	{ "/list/sample",      dump_list,       NULL },
	{ "/stats",            dump_stats,      NULL }
    };
    static const int nkeys = sizeof(keys)/sizeof(keys[0]);

    int i;

    for (i=0; i<nkeys; i++) {
	if (strcasecmp(name, keys[i].key) == 0)
	    return keys[i].f(db, (keys[i].arg_override ? keys[i].arg_override
				  : name));
    }
    
    myerror(ERRDEF, "unknown special: `%s'", name);
    return -1;
}



/*ARGSUSED2*/
static int
dump_stats(sqlite3 *db, const char *dummy)
{
    int ngames, nroms, ndisks;
    unsigned long long sroms;
    parray_t *list;
    game_t *game;
    int i, j;

    if ((list=r_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "db error reading game list");
	return -1;
    }

    ngames = parray_length(list);
    nroms = ndisks = 0;
    sroms = 0;

    for (i=0; i<parray_length(list); i++) {
	if ((game=r_game(db, (char *)parray_get(list, i))) == NULL) {
	    /* XXX: internal error */
	    continue;
	}
	nroms += game_num_files(game, TYPE_ROM);
	for (j=0; j<game_num_files(game, TYPE_ROM); j++)
	    sroms += rom_size(game_file(game, TYPE_ROM, j));

	ndisks += game_num_disks(game);

	game_free(game);
    }

    parray_free(list, free);

    printf("Games:\t%d\n", ngames);
    printf("ROMs:\t%d ", nroms);
    if (sroms > 1024*1024*1024)
	printf("(%llu.%02llugb)\n",
	       sroms/(1024*1024*1024),
	       (((sroms/(1024*1024))*10+512)/1024) % 100);
    else if (sroms > 1024*1024)
	printf("(%llu.%02llumb)\n",
	       sroms/(1024*1024),
	       (((sroms/1024)*10+512)/1024) % 100);
    else
	printf("(%llu bytes)\n", sroms);
    if (ndisks)
	printf("Disks:\t%d\n", ndisks);

    return 0;
}




static void
print_dat(dat_t *d, int i)
{
    printf("%s (%s)",
	   dat_name(d, i) ? dat_name(d, i) : "unknown",
	   dat_version(d, i) ? dat_version(d, i) : "unknown");
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
