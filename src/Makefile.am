AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@ \
	 -DDDB_INCLUDE=\"db-@DBTYPE@.h\"

bin_PROGRAMS=ckmame mkmamedb dumpgame

noinst_LIBRARIES=libckmame.a

ckmame_LDADD=libckmame.a @DBLIBS@ @LIBZIP_LIBS@
mkmamedb_LDADD=libckmame.a @DBLIBS@ @LIBXML2_LIBS@
dumpgame_LDADD=libckmame.a @DBLIBS@

libckmame_a_LIBADD=@LIBOBJS@

libckmame_a_SOURCES= \
	archive.c \
	array.c \
	array_delete.c \
	array_grow.c \
	array_new_length.c \
	array_push.c \
	chd.c \
	check_disks.c \
	check_files.c \
	db-@DBTYPE@.c \
	dbl.c \
	diagnostics.c \
	disk.c \
	disk_get_info.c \
	disk_new.c \
	error.c \
	file_by_hash.c \
	file_by_hash_finalize.c \
	file_by_hash_make_key.c \
	filetype_db_key.c \
	fix.c \
	game.c \
	game_add_clone.c \
	hash_from_string.c \
	hash_to_string.c \
	hashes.c \
	hashes_set.c \
	hashes_update.c \
	hashes_verify.c \
	map.c \
	match.c \
	match_disk.c \
	parray.c \
	parray_delete.c \
	parray_index_sorted.c \
	parray_new_from_data.c \
	parray_push.c \
	parray_sort_real.c \
	parse-cm.c \
	parse-xml.c \
	parse.c \
	psort.c \
	r_file_by_hash.c \
	r_game.c \
	r_list.c \
	r_prog.c \
	r_util.c \
	rom.c \
	rom_add_altname.c \
	romcmp.c \
	superfluous.c \
	tree.c \
	util.c \
	w_file_by_hash_parray.c \
	w_game.c \
	w_list.c \
	w_prog.c \
	w_util.c \
	xmalloc.c

noinst_HEADERS=\
	archive.h \
	array.h \
	chd.h \
	db-@DBTYPE@.h \
	dbh.h \
	dbl.h \
	disk.h \
	error.h \
	file_by_hash.h \
	funcs.h \
	game.h \
	globals.h \
	hashes.h \
	map.h \
	match.h \
	match_disk.h \
	parray.h \
	parse.h \
	r.h \
	rom.h \
	tree.h \
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
