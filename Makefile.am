AUTOMAKE_OPTIONS=dist-bzip2

LIBS=@DBLIBS@ @LIBZIP_LIBS@ @LIBXML2_LIBS@ @LIBS@
AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@

SUBDIRS=regress

MAN1=ckmame.1 dumpgame.1

bin_PROGRAMS=ckmame mkmamedb dumpgame
dist_bin_SCRIPTS=mkmamedb-xmame.sh
man_MANS=${MAN1}
ckmame_SOURCES=ckmame.c error.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_list.c r_util.c dbl.c dbl-int.c \
		treetrav.c fix.c romutil.c chd-supp.c superfluous.c hashes.c \
		chd.c
mkmamedb_SOURCES=mkmamedb.c dbl.c dbl-int.c parse.c parse-cm.c parse-xml.c \
	w_game.c w_list.c w_prog.c \
	error.c malloc.c w_util.c r_game.c r_util.c util.c romutil.c hashes.c
dumpgame_SOURCES=dumpgame.c error.c malloc.c \
	r_game.c r_util.c r_list.c r_prog.c \
	dbl.c dbl-int.c util.c romutil.c hashes.c
noinst_HEADERS=error.h types.h w.h funcs.h r.h dbl.h \
	dbh.h util.h xmalloc.h romutil.h chd.h parse.h

DISTCLEANFILES=	${man_MANS} ${HTML}

EXTRA_DIST=db-db.c db-db.h db-gdbm.c db-gdbm.h getopt.h fnmatch.h README.DOS \
	${MANDOC} ${MAN}

dist-hook:
	rm -f $(distdir)/dbl-int.c $(distdir)/dbl-int.h

SUFFIXES=.man .mdoc .html
.PHONY: update-man update-html

MANDOC=${MAN1:.1=.mdoc}
MAN=${MANDOC:.mdoc=.man}
HTML=${MANDOC:.mdoc=.html}

.mdoc.man:
	mdoc2man $< > $@.$$$$ && diff -I NiH $@.$$$$ $@ || mv $@.$$$$ $@
	rm -f $@.$$$$

.mdoc.html:
	nroff -mdoc2html $< | sed -e "s,../html1/,," > $@.$$$$ && mv $@.$$$$ $@

update-man: ${MAN}
update-html: ${HTML}
