if(NOT HAVE_GETPROGRAME)
  list(APPEND COMPATIBILITY getprogname.cc)
endif()
if(NOT HAVE_MD5INIT)
  list(APPEND COMPATIBILITY md5.cc)
endif()
if(NOT HAVE_SHA1INIT)
  list(APPEND COMPATIBILITY sha1.cc)
endif()
if(NOT HAVE_VASPRINTF)
  list(APPEND COMPATIBILITY vasprintf.cc)
endif()

set(COMMON_SOURCES
  archive.cc
  ArchiveDir.cc
  archive_modify.cc
  ArchiveZip.cc
  array.cc
  array_delete.cc
  array_grow.cc
  array_insert.cc
  array_new_length.cc
  array_push.cc
  array_sort.cc
  array_truncate.cc
  chd.cc
  check_archive.cc
  check_disks.cc
  check_files.cc
  check_images.cc
  check_old.cc
  check_util.cc
  cleanup.cc
  dat_entry.cc
  db_init.cc
  dbh.cc
  dbh_cache.cc
  dbh_statements.cc
  delete_list.cc
  detector.cc
  detector_execute.cc
  detector_print.cc
  diagnostics.cc
  Dir.cc
  dir.cc
  disk.cc
  disk_new.cc
  error.cc
  export_db.cc
  file.cc
  file_location.cc
  file_location_ext.cc
  file_location_finalize.cc
  file_location_make_key.cc
  file_status.cc
  file_util.cc
  filetype_db_key.cc
  find.cc
  fix.cc
  fix_util.cc
  fixdat.cc
  garbage.cc
  globals.cc
  hashes.cc
  hashes_update.cc
  images.cc
  intstr.cc
  match.cc
  match_disk.cc
  memdb.cc
  output.cc
  output_cm.cc
  output_db.cc
  output_mtree.cc
  parray.cc
  parray_delete.cc
  parray_find.cc
  parray_find_sorted.cc
  parray_new_from_data.cc
  parray_pop.cc
  parray_push.cc
  parray_set_length.cc
  parray_sort_real.cc
  parse-cm.cc
  parse-dir.cc
  parse-rc.cc
  parse.cc
  parser_source.cc
  ParserSourceFile.cc
  ParserSourceZip.cc
  ptr_sort.cc
  result.cc
  romdb.cc
  romdb_read_dat.cc
  romdb_read_detector.cc
  romdb_read_file_by_hash.cc
  romdb_read_file_by_name.cc
  romdb_read_game.cc
  romdb_read_list.cc
  romdb_write_dat.cc
  romdb_write_detector.cc
  romdb_write_game.cc
  sighandle.cc
  sq_util.cc
  stats.cc
  superfluous.cc
  tree.cc
  util.cc
  warn.cc
  xmalloc.cc
  zip_util.cc
  ${COMPATIBILITY}
  )

set(LIBXML2_SOURCES
  detector_parse.cc
  detector_parse_ps.cc
  output_datafile_xml.cc
  parse-xml.cc
  xmlutil.cc
  )  

if(LIBXML2_FOUND)
  list(APPEND COMMON_SOURCES ${LIBXML2_SOURCES})
endif()


add_library(libckmame ${COMMON_SOURCES})
target_link_libraries(libckmame PRIVATE ZLIB::ZLIB libzip::zip)
# for config.h
target_include_directories(libckmame PRIVATE ${PROJECT_BINARY_DIR})

foreach(PROGRAM ckmame detective dumpgame mkmamedb)
  add_executable(${PROGRAM} ${PROGRAM}.cc)
  target_link_libraries(${PROGRAM} PRIVATE libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
  # for config.h
  target_include_directories(${PROGRAM} PRIVATE ${PROJECT_BINARY_DIR})

  install(TARGETS ${PROGRAM} RUNTIME DESTINATION bin)
endforeach()

if(LibXml2_FOUND)
  foreach(TARGET ckmame detective mkmamedb libckmame)
    target_link_libraries(${TARGET} PRIVATE LibXml2::LibXml2)
  endforeach()
endif()

#add_executable(mamedb mamedb.cc mamedb_add.cc mamedb_cmdtab.cc mamedb_help.cc mamedb_new.cc)
#target_link_libraries(mamedb PRIVATE libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
#install(TARGETS mamedb RUNTIME DESTINATION bin)

add_custom_target(update_dbh_statements
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.cc ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbh_statements.in ${CMAKE_CURRENT_SOURCE_DIR}/mkstatements.pl
)
