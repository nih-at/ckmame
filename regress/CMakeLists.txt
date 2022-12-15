set(SUPPORT_PROGRAMS
  dbdump
  dbrestore
)

set(ENV{srcdir} ${CMAKE_CURRENT_SOURCE_DIR})

foreach(PROGRAM ${SUPPORT_PROGRAMS})
  add_executable(${PROGRAM} ${PROGRAM}.cc)
  # for config.h
  target_include_directories(${PROGRAM} PRIVATE ${PROJECT_BINARY_DIR})
  # compat.h
  target_include_directories(${PROGRAM} BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src)
  target_link_libraries(${PROGRAM} libckmame ZLIB::ZLIB libzip::zip SQLite::SQLite3)
endforeach()


if (LibArchive_FOUND)
  add_executable(compare-archives compare-archives.cc)
  target_include_directories(compare-archives PRIVATE ${PROJECT_BINARY_DIR})
  target_link_libraries(compare-archives ZLIB::ZLIB LibArchive::LibArchive)
endif()

if (NOT WIN32)
add_library(fwrite MODULE fwrite.c)
target_link_libraries(fwrite ${CMAKE_DL_LIBS})
endif()

add_custom_target(cleanup
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cleanup.cmake
  )

set(DBS
  mame.db
  mame-v2.db
  mamedb-1-8-is-4.db
  mamedb-baddump.db
  mamedb-deadbeefish.db
  mamedb-disk.db
  mamedb-disk-many.db
  mamedb-file-no-crc.db
  mamedb-lost-parent-ok.db
  mamedb-merge-parent.db
  mamedb-one-game-two-roms.db
  mamedb-parent-crcdiff.db
  mamedb-parent-no-common.db
  mamedb-size-empty.db
  mamedb-small.db
  mamedb-two-games.db
  mamedb-xml-quoting.db
  )

set(XML_DBS
  mamedb-detector.db
)

set(CUSTOM_DBS
    mamedb-skipped.db
)

foreach(db ${DBS})
  get_filename_component(main ${db} NAME_WE)
  add_custom_command(OUTPUT "${db}"
    COMMAND mkmamedb -o "${db}" "${CMAKE_CURRENT_SOURCE_DIR}/${main}.dat"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${main}.dat"
    COMMENT "Generating ${db}"
    )
endforeach()

foreach(db ${XML_DBS})
  get_filename_component(main ${db} NAME_WE)
  add_custom_command(OUTPUT "${db}"
    COMMAND mkmamedb -o "${db}" "${CMAKE_CURRENT_SOURCE_DIR}/${main}.xml"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${main}.xml"
    COMMENT "Generating ${db}"
    )
endforeach()

add_custom_command(OUTPUT mamedb-skipped.db
    COMMAND mkmamedb -o mamedb-skipped.db --detector "${CMAKE_CURRENT_SOURCE_DIR}/detector.xml" "${CMAKE_CURRENT_SOURCE_DIR}/mamedb-skipped.dat"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mamedb-skipped.dat" "${CMAKE_CURRENT_SOURCE_DIR}/detector.xml"
    COMMENT "Generating mamedb-skipped.db"
)

add_custom_target(testinput
  ALL
  VERBATIM
  DEPENDS ${DBS} ${XML_DBS} ${CUSTOM_DBS}
  )

add_custom_target(update-mamedb.dump
  COMMAND dbdump ${CMAKE_CURRENT_BINARY_DIR}/mame.db > ${CMAKE_CURRENT_SOURCE_DIR}/mamedb-ok.dump
  COMMAND dbdump ${CMAKE_CURRENT_BINARY_DIR}/mamedb-lost-parent-ok.db > ${CMAKE_CURRENT_SOURCE_DIR}/mamedb-lost-parent.dump
  COMMAND mkmamedb -o mame.db.tmp ${CMAKE_CURRENT_SOURCE_DIR}/mame.dat ${CMAKE_CURRENT_SOURCE_DIR}/mamedb-lost-parent-ok.dat
  COMMAND dbdump mame.db.tmp > mamedb-duplicate-game.dump
  COMMAND cmake -E remove mame.db.tmp
  COMMAND dbdump mamedb-parent-crcdiff.db > mamedb-parent-crcdiff.dump
  COMMAND dbdump mamedb-parent-no-common.db > mamedb-parent-no-common.dump
  COMMAND mkmamedb -o tmptmp.db ${CMAKE_CURRENT_SOURCE_DIR}/mame.dat ${CMAKE_CURRENT_SOURCE_DIR}/mamedb-2.dat
  COMMAND dbdump tmptmp.db > mamedb-multi-dats.dump
  COMMAND cmake -E remove tmptmp.db
  DEPENDS ${DBS}
)


set(XFAIL_TESTS
  extra-file-zip.test
  file-no-crc-wrong-name-other-set.test
  rom-from-extra-loose-directory.test
  rom-from-extra-loose-toplevel.test
)
	      
file(GLOB EXTRA_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/*.test)
foreach(FULL_CASE IN LISTS EXTRA_TESTS)
  get_filename_component(CASE ${FULL_CASE} NAME)
  add_test(NAME ${CASE} COMMAND ${PERL_EXECUTABLE} ${PROJECT_BINARY_DIR}/regress/runtest --bin-sub-directory $<CONFIG> ${PROJECT_SOURCE_DIR}/regress/${CASE})
  set_tests_properties(${CASE} PROPERTIES SKIP_RETURN_CODE 77)
#  set_tests_properties(${CASE} PROPERTIES ENVIRONMENT "PATH=${path}")
endforeach()

foreach(CASE ${XFAIL_TESTS})
  set_tests_properties(${CASE} PROPERTIES WILL_FAIL TRUE)
endforeach()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
