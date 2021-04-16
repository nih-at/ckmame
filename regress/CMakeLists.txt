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


add_executable(compare-archives compare-archives.cc)
target_include_directories(compare-archives PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(compare-archives ZLIB::ZLIB LibArchive::LibArchive)


add_library(fwrite MODULE fwrite.c)
target_link_libraries(fwrite ${CMAKE_DL_LIBS})

add_custom_target(cleanup
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cleanup.cmake
  )

set(DBS
  mame.db
  mamedb-1-8-is-4.db
  mamedb-baddump.db
  mamedb-deadbeefish.db
  mamedb-disk.db
  mamedb-disk-many.db
  mamedb-lost-parent-ok.db
  mamedb-merge-parent.db
  mamedb-one-game-two-roms.db
  mamedb-parent-crcdiff.db
  mamedb-parent-no-common.db
  mamedb-small.db
  mamedb-two-games.db
  mamedb-xml-quoting.db
  )

foreach(db ${DBS})
  get_filename_component(main ${db} NAME_WE)
  add_custom_command(OUTPUT "${db}"
    COMMAND mkmamedb -o "${db}" "${CMAKE_CURRENT_SOURCE_DIR}/${main}.dat"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${main}.dat"
    COMMENT "Generating ${db}"
    )
endforeach()

add_custom_target(testinput
  ALL
  VERBATIM
  DEPENDS ${DBS}
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

set(EXTRA_TESTS
  archive-broken.test
  archive-broken2.test
  baddump.test
  ckmame.test
  cleanup-disk.test
  cleanup-implicit.test
  cleanup.test
  delete-used-superfluous.test
  detector-interaction-with-headerless.test
  detector-prefer-file-with-header.test
  disk-fixdat
  disk-from-extra-remove.test
  disk-from-extra-using-copy-remove.test
  disk-from-extra.test
  disk-from-superfluous-all.test
  disk-from-superfluous.test
  disk-invalid.test
  disk-miss.test
  disk-nogood-copy.test
  disk-nogood-miss.test
  disk-nogood.test
  disk-ok.test
  disk-parent-merge.test
  disk-parent-merge-exists.test
  disk-superfluous.test
  disk-swap.test
  disk-swapmerge.test
  disk-to-needed.test
  disk-toplevel.test
  disk-toplevel-cleanup.test
  disk-unrelated.test
  disk-unrelated-fix.test
  disk-wrong.test
  disk-wrong2.test
  diskfull-dir.test
  diskfull-zip.test
  doublezip.test
  dumpgame-stats.test
  dumpgame-unknown.test
  extra-file-dir.test
  extra-file-zip.test
  extra-size-mismatch.test
  fixdat-missing-all.test
  fixdat-missing-child.test
  fixdat-missing-parent.test
  fixdat-missing-part.test
  fixdat-xml-quoting.test
  gamelist-from-file.test
  garbage-archive-exists.test
  garbage.test
  garbage2.test
  hasherr-c.test
  hasherr-extra.test
  hasherr-no-i.test
  hasherr-p.test
  hasherr-pc.test
  hasherr.test
  hasherr2.test
  identical-once.test
  ignore-unknown.test
  inchild.test
  incomplete-basic-b.test
  incomplete-basic.test
  incomplete-existing-b.test
  incomplete-existing.test
  incomplete-from-extra-b.test
  incomplete-from-extra.test
  incomplete-merge-b.test
  incomplete-merge.test
  inconsistent-with-i.test
  inconsistent-without-i.test
  inparent.test
  misnamed-nofix.test
  misnamed.test
  mkmamedb-broken-sha1.test
  mkmamedb-cmpro-bad.test
  mkmamedb-dir-in-name.test
  mkmamedb-duplicate-game.test
  mkmamedb-duplicate-nodump.test
  mkmamedb-duplicate-rom-name.test
  mkmamedb-input-dir-dir.test
  mkmamedb-input-dir-zip.test
  mkmamedb-lost-parent.test
  mkmamedb-mame.test
  mkmamedb-merge-no-parent.test
  mkmamedb-merge-three-generations.test
  mkmamedb-merge-wrong-name.test
  mkmamedb-mess.test
  mkmamedb-multi-dats.test
  mkmamedb-ok.test
  mkmamedb-parent-crcdiff-mame.test
  mkmamedb-parent-crcdiff.test
  mkmamedb-parent-no-common.test
  mkmamedb-rc-ok.test
  mkmamedb-size-hex.test
  mkmamedb-zipped-ok.test
  multi.test
  needed-cleanup.test
  needed-error.test
  needed-exists-different.test
  needed-exists-same.test
  needed-keep.test
  needed.test
  nogood-allmissing-suppress.test
  nogood-allmissing.test
  nogood-diskgood.test
  nogood-miss-d.test
  nogood-miss.test
  nogood.test
  nonexist.test
  norom.test
  ok.test
  old-complete.test
  old-disk.test
  old-keep.test
  old-partial.test
  replace-broken-rom.test
  rom-7z.test
  rom-add-from-crc-different.test
  rom-add-from-crc-only.test
  rom-broken-replace.test
  rom-broken-without-i.test
  rom-broken.test
  rom-dir-in-extra-dirs-subdir-dir.test
  rom-dir-in-extra-dirs-subdir-zip.test
  rom-dir-in-extra-dirs.test
  rom-dir.test
  rom-duplicate-1st.test
  rom-duplicate-2nd.test
  rom-duplicate-ok.test
  rom-duplicate-wrong.test
  rom-from-child-renamed.test
  rom-from-extra-broken-unwanted.test
  rom-from-extra-broken.test
  rom-from-extra-child.test
  rom-from-extra-commit-error.test
  rom-from-extra-dbcache-cleanup.test
  rom-from-extra-detector.test
  rom-from-extra-inconsistent.test
  rom-from-extra-keep-dir-clean.test
  rom-from-extra-keep-dir.test
  rom-from-extra-keep-zip.test
  rom-from-extra-partial.test
  rom-from-extra-partial-7z.test
  rom-from-extra-remove.test
  rom-from-extra-size-changed.test
  rom-from-extra-slash.test
  rom-from-needed.test
  rom-from-needed2.test
  rom-from-romset.test
  rom-from-romset2.test
  rom-from-superfluous-cleanup.test
  rom-from-superfluous-detector.test
  rom-from-superfluous-many-fixdat.test
  rom-from-superfluous-many.test
  rom-from-superfluous-nofix.test
  rom-from-superfluous-some.test
  rom-from-superfluous.test
  rom-from-unknown.test
  rom-long-double.test
  rom-long-end.test
  rom-long-mid.test
  rom-many.test
  rom-need-partial-and-whole.test
  rom-two-long.test
  rom-unused.test
  rom-unused-duplicate.test
  skip-full.test
  skip.test
  skipbad.test
  superfluous-broken.test
  superfluous-cleanup-disk.test
  superfluous-cleanup.test
  superfluous-inconsistent.test
  superfluous.test
  swap-roms-2.test
  swap-roms.test
  swap.test
  tochild.test
  tochildfix.test
  uncompressed-ckmamedb-add-game.test
  uncompressed-ckmamedb-changed-new-mtime.test
  uncompressed-ckmamedb-changed-same-mtime-nofix.test
  uncompressed-ckmamedb-changed-same-mtime-zip.test
  uncompressed-ckmamedb-changed-same-mtime.test
  uncompressed-ckmamedb-extra-dir.test
  uncompressed-ckmamedb-extra-remove.test
  uncompressed-ckmamedb-no-change.test
  uncompressed-ckmamedb-remove-game-2.test
  uncompressed-ckmamedb-remove-game-3.test
  uncompressed-ckmamedb-remove-game.test
  uncompressed-ckmamedb-sort-order-reverse.test
  uncompressed-ckmamedb-sort-order.test
  uncompressed-toplevel-extra.test
  uncompressed-toplevel-file.test
  uncompressed-toplevel-roms.test
  unknown-in-parent.test
  unused-in-parent.test
  wrong.test
  wrong2.test
  zero-good.test
  zero-long-badcrc.test
  zero-long.test
  zero-miss.test
  zero-superfluous.test
  zero.test
)

set(XFAIL_TESTS
  detector-interaction-with-headerless.test
  detector-prefer-file-with-header.test
  rom-7z.test
)
	      
foreach(CASE ${EXTRA_TESTS})
  add_test(NAME ${CASE} COMMAND perl ${CMAKE_CURRENT_BINARY_DIR}/runtest --bin-sub-directory $<CONFIG> ${CMAKE_CURRENT_SOURCE_DIR}/${CASE})
  set_tests_properties(${CASE} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()

foreach(CASE ${XFAIL_TESTS})
  set_tests_properties(${CASE} PROPERTIES WILL_FAIL TRUE)
endforeach()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
