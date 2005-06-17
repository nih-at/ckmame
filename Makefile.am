AUTOMAKE_OPTIONS=dist-bzip2

AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@

SUBDIRS=docs regress

bin_PROGRAMS=ckmame mkmamedb dumpgame
dist_bin_SCRIPTS=mkmamedb-xmame.sh

ckmame_SOURCES=ckmame.c error.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_list.c r_util.c dbl.c dbl-int.c \
		treetrav.c fix.c romutil.c chd-supp.c superfluous.c hashes.c \
		chd.c
ckmame_LDADD=@DBLIBS@ @LIBZIP_LIBS@ @LIBOBJS@

mkmamedb_SOURCES=mkmamedb.c dbl.c dbl-int.c parse.c parse-cm.c parse-xml.c \
	w_game.c w_list.c w_prog.c \
	error.c malloc.c w_util.c r_game.c r_util.c util.c romutil.c hashes.c
mkmamedb_LDADD=@DBLIBS@ @LIBXML2_LIBS@ @LIBOBJS@

dumpgame_SOURCES=dumpgame.c error.c malloc.c \
	r_game.c r_util.c r_list.c r_prog.c \
	dbl.c dbl-int.c util.c romutil.c hashes.c
dumpgame_LDADD=@DBLIBS@ @LIBOBJS@

noinst_HEADERS=error.h types.h w.h funcs.h r.h dbl.h \
	dbh.h util.h xmalloc.h romutil.h chd.h parse.h \
	uint32.h md5_own.h sha1_own.h

CLEANFILES=	${HTML} mkmamedb.1

EXTRA_DIST=db-db.c db-db.h db-gdbm.c db-gdbm.h \
	fnmatch.c fnmatch.h \
	getopt1.c getopt.c getopt.h \
	md5.c sha1.c \
	${MANDOC} ${MANDOC2} ${MAN}

dist-hook:
	rm -f $(distdir)/dbl-int.c $(distdir)/dbl-int.h

MANFMT=@MANFMT@
SUFFIXES=.man .mdoc .html
.PHONY: update-man update-html

man1_MANS=${MANDOC:.mdoc=.${MANFMT}} mkmamedb.1

MANDOC=ckmame.mdoc dumpgame.mdoc
MANDOC2=mkmamedb.mdoc
ALLMANDOC=${MANDOC} ${MANDOC2}
MAN=${ALLMANDOC:.mdoc=.man}
HTML=${ALLMANDOC:.mdoc=.html}

mkmamedb.1: mkmamedb.${MANFMT}
	sed 's,PREFIX,${prefix},g' ${srcdir}/mkmamedb.${MANFMT} > $@

.mdoc.man:
	mdoc2man $< > $@.$$$$ && diff -I NiH $@.$$$$ $@ || mv $@.$$$$ $@; \
	rm -f $@.$$$$

.mdoc.html:
	nroff -mdoc2html $< | sed -e "s,../html1/,," > $@.$$$$ && mv $@.$$$$ $@

update-man: ${MAN}
update-html: ${HTML}
