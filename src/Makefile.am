AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@ \
	 -DDDB_INCLUDE=\"db-@DBTYPE@.h\"

bin_PROGRAMS=ckmame mkmamedb dumpgame

noinst_LIBRARIES=libckmame.a

ckmame_LDADD=libckmame.a @DBLIBS@ @LIBZIP_LIBS@
mkmamedb_LDADD=libckmame.a @DBLIBS@ @LIBXML2_LIBS@
dumpgame_LDADD=libckmame.a @DBLIBS@

libckmame_a_LIBADD=@LIBOBJS@
libckmame_a_SOURCES= \
	chd-supp.c \
	chd.c \
	db-@DBTYPE@.c \
	dbl.c \
	error.c \
	fix.c \
	hash_to_string.c \
	hashes.c \
	hashes_update.c \
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

EXTRA_DIST=db-db.c db-db.h db-gdbm.c db-gdbm.h \
	fnmatch.c fnmatch.h \
	getopt1.c getopt.c getopt.h \
	md5.c sha1.c
