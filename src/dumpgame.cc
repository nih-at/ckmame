/*
  dumpgame.c -- print info about game (from data base)
  Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "dat.h"
#include "error.h"
#include "globals.h"
#include "hashes.h"
#include "romdb.h"
#include "sq_util.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static int dump_game(const char *, int);
static int dump_hashtypes(int);
static int dump_list(int);
static int dump_dat(int);
static int dump_special(const char *);
static int dump_stats(int);
static void print_dat(const DatEntry &de);
static void print_hashtypes(int);
static void print_rs(GamePtr, const char *, const char *, const char *, const char *);

const char *usage = "Usage: %s [-h|-V]\n\
       %s [-b] [-D dbfile] [game ...]\n\
       %s [-b] [-D dbfile] [-c | -d] [checksum ...]\n";

const char help_head[] = "dumpgame (" PACKAGE ") by Dieter Baron and"
		   " Thomas Klausner\n\n";

const char help[] = "\n\
  -b, --brief          brief listing (omit ROM details)\n\
  -c, --checksum       find games containing ROMs with given checksums\n\
  -D, --db dbfile      use database dbfile\n\
  -d, --disk           find games containing disks with given checksums\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

const char version_string[] = "dumpgame (" PACKAGE " " VERSION ")\n\
Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "hbcD:dV"

struct option options[] = {
    {"brief", 0, 0, 'b'}, {"checksum", 0, 0, 'c'}, {"db", 1, 0, 'D'}, {"disk", 0, 0, 'd'}, {"help", 0, 0, 'h'}, {"version", 0, 0, 'V'}, {NULL, 0, 0, 0},
};

static const char *where_name[] = {"game", "cloneof", "grand-cloneof"};

static void
print_checksums(const Hashes *hashes) {
    for (size_t i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
	if (hashes->has_type(i)) {
            printf(" %s %s", Hashes::type_name(i).c_str(), hashes->to_string(i).c_str());
        }
    }
}


static void
print_diskline(Disk *disk) {
    printf("\t\tdisk %-12s", disk->name.c_str());
    print_checksums(&disk->hashes);
    printf(" status %s in %s", status_name(disk->status, true).c_str(), where_name[disk->where]);
    if (!disk->merge.empty() && disk->name != disk->merge) {
        printf(" (%s)", disk->merge.c_str());
    }
    putc('\n', stdout);
}


static void
print_footer(int matches, Hashes *hash) {
    printf("%d matches found for checksum", matches);
    print_checksums(hash);
    putc('\n', stdout);
}


static void
print_romline(File *rom) {
    printf("\t\tfile %-12s  size ", rom->name.c_str());
    if (rom->is_size_known()) {
	printf("%7" PRIu64, rom->size);
    }
    else {
	printf("unknown");
    }
    print_checksums(&rom->hashes);
    printf(" status %s in %s", status_name(rom->status, true).c_str(), where_name[rom->where]);
    if (!rom->merge.empty() && rom->name != rom->merge) {
	printf(" (%s)", rom->merge.c_str());
    }
    putc('\n', stdout);
}


static void
print_match(GamePtr game, filetype_t ft, int i) {
    static std::string name;

    if (name.empty() || game->name != name) {
	name = game->name;
	printf("In game %s:\n", name.c_str());
    }

    if (ft == TYPE_DISK)
	print_diskline(&game->disks[i]);
    else
	print_romline(&game->roms[i]);
}


static void
print_matches(filetype_t ft, Hashes *hash) {
    int matches_count = 0;

    auto matches = romdb_read_file_by_hash(db, ft, hash);
    if (matches.empty()) {
	print_footer(0, hash);
	return;
    }

    for (size_t i = 0; i < matches.size(); i++) {
	auto match = matches[i];
	auto game = romdb_read_game(db, match.name);
	if (!game) {
	    myerror(ERRDEF, "db error: %s not found, though in hash index", match.name.c_str());
	    /* TODO: remember error */
	    continue;
	}

	print_match(game, ft, match.index);
	matches_count++;
    }

    print_footer(matches_count, hash);
}


int
main(int argc, char **argv) {
    int i, j, found, first;
    const char *dbname;
    int c;
    int find_checksum, brief_mode;
    filetype_t filetype = TYPE_ROM;
    parray_t *list;

    setprogname(argv[0]);

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;

    find_checksum = brief_mode = 0;

    opterr = 0;
    while ((c = getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
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
	    fprintf(stderr, usage, getprogname(), getprogname(), getprogname());
	    exit(1);
	}
    }

    if ((db = romdb_open(dbname, DBH_READ)) == NULL) {
	myerror(0, "can't open database '%s': %s", dbname, errno == EFTYPE ? "unsupported database version, please recreate" : strerror(errno) );
	exit(1);
    }
    seterrdb(romdb_dbh(db));

    if ((list = romdb_read_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "list of games not found in database '%s'", dbname);
	exit(1);
    }

    /* find matches for ROMs */
    if (find_checksum != 0) {
	Hashes match;

	for (i = optind; i < argc; i++) {
	    /* checksum */
	    if ((match.set_from_string(argv[i])) == -1) {
		myerror(ERRDEF, "error parsing checksum '%s'", argv[i]);
		exit(2);
	    }

	    print_matches(filetype, &match);
	}
	exit(0);
    }

    first = 1;
    for (i = optind; i < argc; i++) {
	if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
	    if (argv[i][0] == '/') {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_special(argv[i]);
	    }
	    else if (parray_find_sorted(list, argv[i], reinterpret_cast<int (*)(const void *, const void *)>(strcmp)) >= 0) {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_game(argv[i], brief_mode);
	    }
	    else
		myerror(ERRDEF, "game '%s' unknown", argv[i]);
	}
	else {
	    found = 0;
	    for (j = 0; j < parray_length(list); j++) {
		if (fnmatch(argv[i], static_cast<char *>(parray_get(list, j)), 0) == 0) {
		    if (first)
			first = 0;
		    else
			putc('\n', stdout);
		    dump_game(static_cast<char *>(parray_get(list, j)), brief_mode);
		    found = 1;
		}
	    }
	    if (!found)
		myerror(ERRDEF, "no game matching '%s' found", argv[i]);
	}
    }

    parray_free(list, reinterpret_cast<void (*)(void *)>(free));

    return 0;
}


static void
print_rs(GamePtr game, const char *co, const char *gco, const char *cs, const char *fs) {
    int ret;
    sqlite3_stmt *stmt;
    size_t i;

    if (!game->cloneof[0].empty()) {
	printf("%s:\t%s\n", co, game->cloneof[0].c_str());
    }
    if (!game->cloneof[1].empty()) {
	printf("%s:\t%s\n", gco, game->cloneof[1].c_str());
    }

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_CLONES)) == NULL) {
	myerror(ERRDB, "cannot get clones for '%s'", game->name.c_str());
	return;
    }
    if (sq3_set_string(stmt, 1, game->name.c_str()) != SQLITE_OK) {
	myerror(ERRDB, "cannot get clones for '%s'", game->name.c_str());
	return;
    }

    for (i = 0; (ret = sqlite3_step(stmt)) == SQLITE_ROW; i++) {
	if (i == 0)
	    printf("%s", cs);
	if (i % 6 == 0)
	    printf("\t\t");
	printf("%-8s ", sqlite3_column_text(stmt, 0));
	if (i % 6 == 5)
	    printf("\n");
    }
    if (i % 6 != 0)
	printf("\n");

    if (ret != SQLITE_DONE) {
	myerror(ERRDB, "cannot get clones for '%s'", game->name.c_str());
	return;
    }

    if (game->roms.size() > 0) {
	printf("%s:\n", fs);
	for (i = 0; i < game->roms.size(); i++)
	    print_romline(&game->roms[i]);
    }
}


static int
dump_game(const char *name, int brief_mode) {
    GamePtr game;
    std::vector<DatEntry> dat;

    dat = romdb_read_dat(db);

    if (dat.empty()) {
	myerror(ERRDEF, "cannot read dat info");
	return -1;
    }

    if ((game = romdb_read_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or database error): '%s'", name);
	return -1;
    }

    /* TODO: use print_* functions */
    printf("Name:\t\t%s\n", game->name.c_str());
    if (dat.size() > 1) {
	printf("Source:\t\t");
        print_dat(dat[game->dat_no]);
	putc('\n', stdout);
    }
    if (!game->description.empty()) {
	printf("Description:\t%s\n", game->description.c_str());
    }

    if (!brief_mode) {
	print_rs(game, "Cloneof", "Grand-Cloneof", "Clones", "ROMs");

	if (game->disks.size() > 0) {
	    printf("Disks:\n");
	    for (size_t i = 0; i < game->disks.size(); i++) {
		print_diskline(&game->disks[i]);
	    }
	}
    }

    return 0;
}


/*ARGSUSED1*/
static int
dump_hashtypes(int dummy) {
    printf("roms: ");
    print_hashtypes(romdb_hashtypes(db, TYPE_ROM));
    printf("\ndisks: ");
    print_hashtypes(romdb_hashtypes(db, TYPE_DISK));
    putc('\n', stdout);

    return 0;
}


static int
dump_list(int type) {
    parray_t *list;

    if ((list = romdb_read_list(db, static_cast<enum dbh_list>(type))) == NULL) {
	myerror(ERRDB, "db error reading list");
	return -1;
    }

    for (auto i = 0; i < parray_length(list); i++)
	printf("%s\n", (char *)parray_get(list, i));

    parray_free(list, reinterpret_cast<void (*)(void *)>(free));

    return 0;
}


/*ARGSUSED1*/
static int
dump_dat(int dummy) {
    auto dat = romdb_read_dat(db);
    
    if (dat.empty()) {
	myerror(ERRDEF, "db error reading /dat");
	return -1;
    }

    for (size_t i = 0; i < dat.size(); i++) {
        if (dat.size() > 1) {
            printf("%2zu: ", i);
        }
        print_dat(dat[i]);
	putc('\n', stdout);
    }

    return 0;
}


/*ARGSUSED1*/
static int
dump_detector(int dummy) {
    DetectorPtr detector;

    if ((detector = romdb_read_detector(db))) {
        printf("%s", detector->name.c_str());
        if (!detector->version.empty()) {
            printf(" (%s)", detector->version.c_str());
        }
	printf("\n");
    }

    return 0;
}


static int
dump_special(const char *name) {
    static const struct {
	const char *key;
	int (*f)(int);
	int arg;
    } keys[] = {{"/dat", dump_dat, 0}, {"/detector", dump_detector, 0}, {"/hashtypes", dump_hashtypes, 0}, {"/list", dump_list, DBH_KEY_LIST_GAME}, {"/list/disk", dump_list, DBH_KEY_LIST_DISK}, {"/list/game", dump_list, DBH_KEY_LIST_GAME}, {"/stats", dump_stats, 0}};
    static const size_t nkeys = sizeof(keys) / sizeof(keys[0]);

    for (size_t i = 0; i < nkeys; i++) {
	if (strcasecmp(name, keys[i].key) == 0)
	    return keys[i].f(keys[i].arg);
    }

    myerror(ERRDEF, "unknown special: '%s'", name);
    return -1;
}


/*ARGSUSED1*/
static int
dump_stats(int dummy) {
    sqlite3_stmt *stmt;
    int i, ft;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_STATS_GAMES)) == NULL) {
	myerror(ERRDB, "can't get number of games");
	return -1;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
	myerror(ERRDB, "can't get number of games");
	return -1;
    }

    stats = stats_new();
    stats->games_total = (uint64_t)sqlite3_column_int(stmt, 0);

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_STATS_FILES)) == NULL) {
	myerror(ERRDB, "can't get file stats");
	return -1;
    }

    ft = -1;
    for (i = 0; i < TYPE_MAX; i++) {
	if (ft < i) {
	    switch (sqlite3_step(stmt)) {
	    case SQLITE_ROW:
		ft = sqlite3_column_int(stmt, 0);
		break;
	    case SQLITE_DONE:
		ft = TYPE_MAX;
		break;
	    default:
		myerror(ERRDB, "can't get file stats");
		return -1;
	    }
	}

        if (ft != i) {
	    continue;
        }
        
        stats->files[i].files_total = (uint64_t)sqlite3_column_int(stmt, 1);
        stats->files[i].bytes_total = (uint64_t)sqlite3_column_int64(stmt, 2);
    }
    
    stats_print(stats, stdout, true);

    return 0;
}


static void print_dat(const DatEntry &de) {
    printf("%s (%s)", de.name.empty() ? "unknown" : de.name.c_str(), de.version.empty() ? "unknown" : de.version.c_str());
}


#define DO(ht, x, s) (((ht) & (x)) ? printf("%s%s", (first ? first = 0, "" : ", "), (s)) : 0)


static void
print_hashtypes(int ht) {
    int first;

    first = 1;

    DO(ht, Hashes::TYPE_CRC, "crc");
    DO(ht, Hashes::TYPE_MD5, "md5");
    DO(ht, Hashes::TYPE_SHA1, "sha1");
}
