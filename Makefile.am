AUTOMAKE_OPTIONS=dist-bzip2

AM_CPPFLAGS=-I$(top_srcdir)/libzip
LDADD=@LIBOBJS@ libzip/libzip.la

LIBS=@DBLIBS@ @LIBS@

SUBDIRS=libzip regress

bin_PROGRAMS=ckmame mkmamedb dumpgame superflouos
man_MANS=ckmame.1
#info_TEXINFOS=  ckmame.texi
ckmame_SOURCES=ckmame.c error.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_list.c r_util.c dbl.c dbl-int.c \
		treetrav.c fix.c romutil.c
mkmamedb_SOURCES=mkmamedb.c dbl.c dbl-int.c dbread.c w_game.c w_list.c \
	w_prog.c \
	error.c malloc.c w_util.c r_game.c r_util.c util.c romutil.c
dumpgame_SOURCES=dumpgame.c error.c malloc.c r_game.c r_util.c r_list.c \
	dbl.c dbl-int.c util.c romutil.c
superflouos_SOURCES=superflouos.c util.c dbl.c dbl-int.c r_list.c r_util.c \
	malloc.c error.c
noinst_HEADERS=error.h types.h w.h funcs.h r.h mkmamedb.h dbl.h \
	dbh.h util.h xmalloc.h romutil.h

DISTCLEANFILES=	ckmame.1 

EXTRA_DIST=db-db.c db-db.h db-gdbm.c db-gdbm.h getopt.h fnmatch.h README.DOS \
	ckmame.mdoc ckmame.man

dist-hook:
	rm -f $(distdir)/dbl-int.c $(distdir)/dbl-int.h
