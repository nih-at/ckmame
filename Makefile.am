AUTOMAKE_OPTIONS=dist-zip 

LDADD=@LIBOBJS@

bin_PROGRAMS=ckmame mkmamedb dumpgame ziptest
ckmame_SOURCES=ckmame.c error.c unzip.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_list.c r_util.c dbl.c dbl-int.c \
		treetrav.c fix.c ziplow.c romutil.c
mkmamedb_SOURCES=mkmamedb.c dbl.c dbl-int.c dbread.c w_game.c w_list.c \
	error.c malloc.c w_util.c r_game.c r_util.c util.c romutil.c
dumpgame_SOURCES=dumpgame.c error.c malloc.c r_game.c r_util.c r_list.c \
	dbl.c dbl-int.c util.c romutil.c
ziptest_SOURCES=ziplow.c ziptest.c error.c malloc.c util.c
noinst_HEADERS=unzip.h error.h types.h w.h funcs.h r.h mkmamedb.h dbl.h \
	dbh.h util.h xmalloc.h romutil.h

EXTRA_DIST=db-db.c db-db.h db-gdbm.c db-gdbm.h getopt.h fnmatch.h zip.h \
	ziplow.h

dist-hook:
	rm -f $(distdir)/dbl-int.c
