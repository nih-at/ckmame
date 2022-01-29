if(NOT HAVE_GETPROGRAME)
  list(APPEND COMPATIBILITY getprogname.cc)
endif()
if(NOT HAVE_MD5INIT)
  list(APPEND COMPATIBILITY md5.cc)
endif()
if(NOT HAVE_SHA1INIT)
  list(APPEND COMPATIBILITY sha1.cc)
endif()

set(COMMON_SOURCES
  Archive.cc
  ArchiveDir.cc
  ArchiveImages.cc
  ArchiveLocation.cc
  archive_modify.cc
  ArchiveZip.cc
  Chd.cc
  check_archive_files.cc
  check_game_files.cc
  check_old.cc
  check_util.cc
  CkmameDB.cc
  cleanup.cc
  Commandline.cc
  Configuration.cc
  DatDb.cc
  DatEntry.cc
  DB.cc
  DBStatement.cc
  DeleteList.cc
  Detector.cc
  DetectorCollection.cc
  detector_execute.cc
  detector_print.cc
  diagnostics.cc
  Dir.cc
  error.cc
  Exception.cc
  File.cc
  FileData.cc
  FileLocation.cc
  file_util.cc
  find.cc
  fix.cc
  fix_util.cc
  fixdat.cc
  Garbage.cc
  globals.cc
  Hashes.cc
  hashes_update.cc
  Match.cc
  MemDB.cc
  OutputContext.cc
  OutputContextCm.cc
  OutputContextDb.cc
  OutputContextMtree.cc
  ParserCm.cc
  ParserDir.cc
  ParserRc.cc
  Parser.cc
  ParserSource.cc
  ParserSourceFile.cc
  ParserSourceZip.cc
  Result.cc
  Rom.cc
  RomDB.cc
  SharedFile.cc
  sighandle.cc
  Stats.cc
  superfluous.cc
  Tree.cc
  util.cc
  warn.cc
  zip_util.cc
  ${COMPATIBILITY}
  )

set(LIBXML2_SOURCES
  detector_parse.cc
  detector_parse_ps.cc
  OutputContextXml.cc
  ParserXml.cc
  XmlProcessor.cc
  )  

if(LIBXML2_FOUND)
  list(APPEND COMMON_SOURCES ${LIBXML2_SOURCES})
endif()

if (HAVE_LIBARCHIVE)
  list(APPEND COMMON_SOURCES ArchiveLibarchive.cc)
endif()

add_library(libckmame ${COMMON_SOURCES})
target_link_libraries(libckmame PRIVATE ZLIB::ZLIB libzip::zip)
if (HAVE_TOMLPLUSPLUS)
  target_link_libraries(libckmame PRIVATE tomlplusplus::tomlplusplus)
endif()
# for config.h
target_include_directories(libckmame PRIVATE ${PROJECT_BINARY_DIR})

if (HAVE_LIBARCHIVE)
  target_link_libraries(libckmame PRIVATE LibArchive::LibArchive)
endif()

foreach(PROGRAM ckmame dumpgame mkmamedb)
  add_executable(${PROGRAM} ${PROGRAM}.cc)
  target_link_libraries(${PROGRAM} PRIVATE libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
  # for config.h
  target_include_directories(${PROGRAM} PRIVATE ${PROJECT_BINARY_DIR})

  install(TARGETS ${PROGRAM} RUNTIME DESTINATION bin)
endforeach()

if(LibXml2_FOUND)
  foreach(TARGET ckmame dumpgame mkmamedb libckmame)
    target_link_libraries(${TARGET} PRIVATE LibXml2::LibXml2)
  endforeach()
endif()
