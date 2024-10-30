if(NOT HAVE_MD5INIT)
  list(APPEND COMPATIBILITY md5.cc)
endif()
if(NOT HAVE_SHA1INIT)
  list(APPEND COMPATIBILITY sha1.cc)
endif()
if(NOT HAVE_SHA256INIT)
  list(APPEND COMPATIBILITY sha256.cc)
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
  DatRepository.cc
  DB.cc
  DBStatement.cc
  DeleteList.cc
  Detector.cc
  DetectorCollection.cc
  detector_execute.cc
  detector_print.cc
  diagnostics.cc
  Dir.cc
  Exception.cc
  File.cc
  FileData.cc
  FileLocation.cc
  file_util.cc
  find.cc
  fix.cc
  fix_util.cc
  Fixdat.cc
  Game.cc
  Garbage.cc
  globals.cc
  Hashes.cc
  hashes_update.cc
  Match.cc
  OutputContext.cc
  OutputContextCm.cc
  OutputContextDb.cc
  OutputContextHeader.cc
  OutputContextMtree.cc
  ParserCm.cc
  ParserDir.cc
  ParserRc.cc
  Parser.cc
  ParserSource.cc
  ParserSourceFile.cc
  ParserSourceZip.cc
  ProgramName.cc
  Progress.cc
  Result.cc
  Rom.cc
  RomDB.cc
  SharedFile.cc
  Stats.cc
  StatusDB.cc
  StatusDBRun.cc
  superfluous.cc
  TomlSchema.cc
  Tree.cc
  update_romdb.cc
  util.cc
  warn.cc
  zip_util.cc
  ${COMPATIBILITY}
        Command.cc CkmameCache.cc Output.cc check_for_file_in_archive.cc)

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

foreach(PROGRAM ckmame ckstatus dumpgame mkmamedb)
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
