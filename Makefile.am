AUTOMAKE_OPTIONS=dist-bzip2

AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@

SUBDIRS=docs regress

bin_PROGRAMS=ckmame mkmamedb dumpgame
dist_bin_SCRIPTS=mkmamedb-xmame.sh

noinst_LIBRARIES=libckmame.a

# ckmame_SOURCES=ckmame.c
ckmame_LDADD=libckmame.a @DBLIBS@ @LIBZIP_LIBS@

# mkmamedb_SOURCES=mkmamedb.c
mkmamedb_LDADD=libckmame.a @DBLIBS@ @LIBXML2_LIBS@

# dumpgame_SOURCES=dumpgame.c
dumpgame_LDADD=libckmame.a @DBLIBS@

libckmame_a_LIBADD=@LIBOBJS@
libckmame_a_SOURCES= \
	chd-supp.c \
	chd.c \
	dbl-int.c \
	dbl.c \
	error.c \
	fix.c \
	hash_to_string.c \
	hashes.c \
	malloc.c \
	marry.c \
	match.c \
	parse-cm.c \
	parse-xml.c \
	parse.c \
	r_game.c \
	r_list.c \
	r_prog.c \
	r_file_by_hash.c \
	r_util.c \
	fbh_make_key.c \
	file_by_hash.c \
	romutil.c \
	superfluous.c \
	treetrav.c \
	util.c \
	w_file_by_hash.c \
	w_game.c \
	w_list.c \
	w_prog.c \
	w_util.c \
	zip-supp.c

noinst_HEADERS=\
	chd.h \
	dbh.h \
	dbl.h \
	error.h \
	funcs.h \
	hashes.h \
	md5_own.h \
	parse.h \
	r.h \
	romutil.h \
	sha1_own.h \
	types.h \
	uint32.h \
	util.h \
	w.h \
	xmalloc.h

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
