AM_CFLAGS=@LIBZIP_CFLAGS@ @LIBXML2_CFLAGS@ @SQLITE3_CFLAGS@ @CFLAGS@ 

bin_PROGRAMS=ckmame detective dumpgame mkmamedb

noinst_LIBRARIES=libckmame.a

ckmame_LDADD=libckmame.a @LIBZIP_LIBS@ @SQLITE3_LIBS@
detective_LDADD=libckmame.a @LIBXML2_LIBS@ @LIBZIP_LIBS@ @SQLITE3_LIBS@
dumpgame_LDADD=libckmame.a @SQLITE3_LIBS@
#mamedb_LDADD=libckmame.a @LIBXML2_LIBS@ @LIBZIP_LIBS@ @SQLITE3_LIBS@
mkmamedb_LDADD=libckmame.a @LIBXML2_LIBS@ @LIBZIP_LIBS@ @SQLITE3_LIBS@

#mamedb_SOURCES= \
#	mamedb.c \
#	mamedb_add.c \
#	mamedb_cmdtab.c \
#	mamedb_help.c \
#	mamedb_new.c

libckmame_a_LIBADD=@LIBOBJS@

libckmame_a_SOURCES= \
	archive.c \
	archive_file_compare_hashes.c \
	archive_modify.c \
	array.c \
	array_delete.c \
	array_grow.c \
	array_insert.c \
	array_new_length.c \
	array_push.c \
	array_sort.c \
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
	db_init.c \
	dbh.c \
	delete_list.c \
	detector.c \
	detector_execute.c \
	detector_init.c \
	detector_parse.c \
	detector_parse_ps.c \
	detector_print.c \
	detector_str.c \
	diagnostics.c \
	dir.c \
	disk.c \
	disk_new.c \
	error.c \
	export_db.c \
	file.c \
	file_compare.c \
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
	garbage.c \
	hash_from_string.c \
	hash_to_string.c \
	hashes.c \
	hashes_set.c \
	hashes_update.c \
	hashes_verify.c \
	images.c \
	intstr.c \
	match.c \
	match_disk.c \
	memdb.c \
	output.c \
	output_cm.c \
	output_db.c \
	output_datafile_xml.c \
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
	parse-dir.c \
	parse-rc.c \
	parse-xml.c \
	parse.c \
	parser_source.c \
	ptr_sort.c \
	r_dat.c \
	r_detector.c \
	r_file_by_hash.c \
	r_game.c \
	r_list.c \
	result.c \
	sighandle.c \
	sq_util.c \
	superfluous.c \
	tree.c \
	util.c \
	w_dat.c \
	w_detector.c \
	w_game.c \
	w_list.c \
	warn.c \
	xmalloc.c \
	xmlutil.c \
	zip_util.c

noinst_HEADERS=\
	archive.h \
	array.h \
	bswap.h \
	chd.h \
	compat.h \
	dat.h \
	dbh.h \
	delete_list.h \
	detector.h \
	dir.h \
	disk.h \
	error.h \
	file.h \
	file_location.h \
	file_status.h \
	find.h \
	funcs.h \
	game.h \
	garbage.h \
	globals.h \
	hashes.h \
	images.h \
	intstr.h \
	mamedb.h \
	match.h \
	match_disk.h \
	memdb.h \
	myinttypes.h \
	output.h \
	parray.h \
	parse.h \
	parser_source.h \
	pri.h \
	result.h \
	sighandle.h \
	sq_util.h \
	tree.h \
	types.h \
	util.h \
	warn.h \
	xmalloc.h \
	xmlutil.h

EXTRA_DIST=\
	bswap16.c \
	bswap32.c \
	compat_fnmatch.h \
	getopt_long.h \
	getprogname.c \
	fnmatch.c \
	getopt_long.c \
	md5.c \
	md5_own.h \
	sha1.c \
	sha1_own.h
