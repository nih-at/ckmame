AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @CFLAGS@ \
	 -DDBL_INCLUDE=\"db-@DBTYPE@.h\"

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
	array_truncate.c \
	chd.c \
	check_archive.c \
	check_disks.c \
	check_files.c \
	check_images.c \
	check_old.c \
	check_util.c \
	cleanup.c \
	dat_entry.c \
	dat_entry_finalize.c \
	dat_push.c \
	db-@DBTYPE@.c \
	dbl.c \
	dbh.c \
	delete_list.c \
	diagnostics.c \
	dir.c \
	disk.c \
	disk_new.c \
	error.c \
	file_location.c \
	file_location_ext.c \
	file_location_finalize.c \
	file_location_make_key.c \
	file_status.c \
	filetype_db_key.c \
	find.c \
	fix.c \
	fix_util.c \
	game.c \
	game_add_clone.c \
	garbage.c \
	hash_from_string.c \
	hash_to_string.c \
	hashes.c \
	hashes_set.c \
	hashes_update.c \
	hashes_verify.c \
	images.c \
	map.c \
	match.c \
	match_disk.c \
	output.c \
	output_db.c \
	parray.c \
	parray_delete.c \
	parray_index.c \
	parray_index_sorted.c \
	parray_new_from_data.c \
	parray_pop.c \
	parray_push.c \
	parray_set_length.c \
	parray_sort_real.c \
	parse-cm.c \
	parse-xml.c \
	parse.c \
	pmap.c \
	psort.c \
	r_dat.c \
	r_file_by_hash.c \
	r_game.c \
	r_list.c \
	r_util.c \
	result.c \
	rom.c \
	rom_add_altname.c \
	superfluous.c \
	tree.c \
	util.c \
	w_file_by_hash_parray.c \
	w_dat.c \
	w_game.c \
	w_list.c \
	w_util.c \
	warn.c \
	xmalloc.c \
	zip_util.c

noinst_HEADERS=\
	archive.h \
	archive_map.h \
	array.h \
	chd.h \
	dat.h \
	db-@DBTYPE@.h \
	dbh.h \
	dbl.h \
	delete_list.h \
	dir.h \
	disk.h \
	disk_map.h \
	error.h \
	file_location.h \
	file_status.h \
	find.h \
	funcs.h \
	game.h \
	garbage.h \
	globals.h \
	hashes.h \
	images.h \
	map.h \
	match.h \
	match_disk.h \
	parray.h \
	parse.h \
	pmap.h \
	pri.h \
	r.h \
	result.h \
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
