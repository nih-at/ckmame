# for config.h
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/..)

SET(COMPATIBILITY)
IF(NOT HAVE_GETPROGRAME)
  LIST(APPEND COMPATIBILITY getprogname.c)
ENDIF()
IF(NOT HAVE_MD5INIT)
  LIST(APPEND COMPATIBILITY md5.c)
ENDIF()
IF(NOT HAVE_SHA1INIT)
  LIST(APPEND COMPATIBILITY sha1.c)
ENDIF()
IF(NOT HAVE_STRLCPY)
  LIST(APPEND COMPATIBILITY strlcpy.c)
ENDIF()
IF(NOT HAVE_VASPRINTF)
  LIST(APPEND COMPATIBILITY vasprintf.c)
ENDIF()

SET(COMMON_SOURCES
  archive.c
  archive_dir.c
  archive_file_compare_hashes.c
  archive_modify.c
  archive_zip.c
  array.c
  array_delete.c
  array_grow.c
  array_insert.c
  array_new_length.c
  array_push.c
  array_sort.c
  array_truncate.c
  chd.c
  check_archive.c
  check_disks.c
  check_files.c
  check_images.c
  check_old.c
  check_util.c
  cleanup.c
  dat_entry.c
  dat_entry_finalize.c
  dat_push.c
  db_init.c
  dbh.c
  dbh_cache.c
  dbh_statements.c
  delete_list.c
  detector.c
  detector_execute.c
  detector_init.c
  detector_parse.c
  detector_parse_ps.c
  detector_print.c
  detector_str.c
  diagnostics.c
  dir.c
  disk.c
  disk_new.c
  error.c
  export_db.c
  file.c
  file_compare.c
  file_location.c
  file_location_ext.c
  file_location_finalize.c
  file_location_make_key.c
  file_status.c
  file_util.c
  filetype_db_key.c
  find.c
  fix.c
  fix_util.c
  fixdat.c
  game.c
  garbage.c
  globals.c
  hash_from_string.c
  hash_to_string.c
  hashes.c
  hashes_set.c
  hashes_update.c
  hashes_verify.c
  images.c
  intstr.c
  match.c
  match_disk.c
  memdb.c
  output.c
  output_cm.c
  output_datafile_xml.c
  output_db.c
  output_mtree.c
  parray.c
  parray_delete.c
  parray_find.c
  parray_find_sorted.c
  parray_new_from_data.c
  parray_pop.c
  parray_push.c
  parray_set_length.c
  parray_sort_real.c
  parse-cm.c
  parse-dir.c
  parse-rc.c
  parse-xml.c
  parse.c
  parser_source.c
  ptr_sort.c
  result.c
  romdb.c
  romdb_read_dat.c
  romdb_read_detector.c
  romdb_read_file_by_hash.c
  romdb_read_file_by_name.c
  romdb_read_game.c
  romdb_read_list.c
  romdb_write_dat.c
  romdb_write_detector.c
  romdb_write_game.c
  sighandle.c
  sq_util.c
  superfluous.c
  tree.c
  util.c
  warn.c
  xmalloc.c
  xmlutil.c
  zip_util.c
  ${COMPATIBILITY}
)

ADD_LIBRARY(libckmame ${COMMON_SOURCES})
TARGET_LINK_LIBRARIES(libckmame ${ZLIB_LIBRARIES})

FOREACH(PROGRAM ckmame detective dumpgame mkmamedb)
  ADD_EXECUTABLE(${PROGRAM} ${PROGRAM}.c)
  TARGET_LINK_LIBRARIES(${PROGRAM} libckmame ${MAIN_LIBS_ABSOLUTE})
  INSTALL(TARGETS ${PROGRAM} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDFOREACH()

FOREACH(PROGRAM detective mkmamedb)
  TARGET_LINK_LIBRARIES(${PROGRAM} ${SUPPORT_LIBS_ABSOLUTE})
ENDFOREACH()

#ADD_EXECUTABLE(mamedb mamedb.c mamedb_add.c mamedb_cmdtab.c mamedb_help.c mamedb_new.c)
#TARGET_LINK_LIBRARIES(mamedb libckmame ${MAIN_LIBS_ABSOLUTE})
#INSTALL(TARGETS mamedb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

ADD_CUSTOM_TARGET(update_dbh_statements
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.c ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl
)
