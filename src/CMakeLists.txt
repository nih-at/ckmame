if(NOT HAVE_GETPROGRAME)
  list(APPEND COMPATIBILITY getprogname.c)
endif()
if(NOT HAVE_MD5INIT)
  list(APPEND COMPATIBILITY md5.c)
endif()
if(NOT HAVE_SHA1INIT)
  list(APPEND COMPATIBILITY sha1.c)
endif()
if(NOT HAVE_STRLCPY)
  list(APPEND COMPATIBILITY strlcpy.c)
endif()
if(NOT HAVE_VASPRINTF)
  list(APPEND COMPATIBILITY vasprintf.c)
endif()

set(COMMON_SOURCES
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
  stats.c
  superfluous.c
  tree.c
  util.c
  warn.c
  xmalloc.c
  zip_util.c
  ${COMPATIBILITY}
  )

set(LIBXML2_SOURCES
  detector_parse.c
  detector_parse_ps.c
  output_datafile_xml.c
  parse-xml.c
  xmlutil.c
  )  

if(LIBXML2_FOUND)
  list(APPEND COMMON_SOURCES ${LIBXML2_SOURCES})
endif()


add_library(libckmame ${COMMON_SOURCES})
target_link_libraries(libckmame PRIVATE ZLIB::ZLIB libzip::zip)
# for config.h
target_include_directories(libckmame PRIVATE ${PROJECT_BINARY_DIR})
set_property(TARGET libckmame PROPERTY C_STANDARD 99)
set_property(TARGET libckmame PROPERTY C_STANDARD_REQUIRED ON)

foreach(PROGRAM ckmame detective dumpgame mkmamedb)
  add_executable(${PROGRAM} ${PROGRAM}.c)
  target_link_libraries(${PROGRAM} PRIVATE libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
  # for config.h
  target_include_directories(${PROGRAM} PRIVATE ${PROJECT_BINARY_DIR})

  install(TARGETS ${PROGRAM} RUNTIME DESTINATION bin)
  set_property(TARGET ${PROGRAM} PROPERTY C_STANDARD 99)
  set_property(TARGET ${PROGRAM} PROPERTY C_STANDARD_REQUIRED ON)
endforeach()

if(LibXml2_FOUND)
  foreach(TARGET ckmame detective mkmamedb libckmame)
    target_link_libraries(${TARGET} PRIVATE LibXml2::LibXml2)
  endforeach()
endif()

#add_executable(mamedb mamedb.c mamedb_add.c mamedb_cmdtab.c mamedb_help.c mamedb_new.c)
#target_link_libraries(mamedb PRIVATE libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
#install(TARGETS mamedb RUNTIME DESTINATION bin)

add_custom_target(update_dbh_statements
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.c ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl
)
