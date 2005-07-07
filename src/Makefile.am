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
	file_by_hash_entry.c \
	file_by_hash_free.c \
	file_by_hash_make_key.c \
	file_by_hash_new.c \
	fix.c \
	hash_from_string.c \
	hash_to_string.c \
	hashes.c \
	hashes_update.c \
	map.c \
	marry.c \
	match.c \
	parray.c \
	parray_delete.c \
	parray_new_from_data.c \
	parray_push.c \
	parray_sort.c \
	parse-cm.c \
	parse-xml.c \
	parse.c \
	r_file_by_hash.c \
	r_game.c \
	r_list.c \
	r_prog.c \
	r_util.c \
	romutil.c \
	superfluous.c \
	treetrav.c \
	util.c \
	w_file_by_hash_parray.c \
	w_game.c \
	w_list.c \
	w_prog.c \
	w_util.c \
	xmalloc.c \
	zip-supp.c

noinst_HEADERS=\
	chd-supp.h \
	chd.h \
	db-@DBTYPE@.h \
	dbh.h \
	dbl.h \
	error.h \
	file_by_hash.h \
	funcs.h \
	hashes.h \
	map.h \
	parray.h \
	parse.h \
	r.h \
	romutil.h \
	types.h \
	util.h \
	w.h \
	warn.h \
	xmalloc.h

EXTRA_DIST=\
	db-db.c \
	db-db.h \
	db-gdbm.c \
	db-gdbm.h \
	fnmatch.c \
	fnmatch.h \
	getopt.c \
	getopt.h \
	getopt1.c \
	md5.c \
	md5_own.h \
	sha1.c \
	sha1_own.h \
	uint32.h
