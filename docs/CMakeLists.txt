set(MAN_PAGES
  ckmame.1
  detective.1
  dumpgame.1
  mkmamedb.1
)

foreach(MAN_PAGE ${MAN_PAGES})
  string(REGEX REPLACE "[1-9]$" "${DOCUMENTATION_FORMAT}" SOURCE_FILE ${MAN_PAGE})
  if(DOCUMENTATION_FORMAT MATCHES "html")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MAN_PAGE} DESTINATION ${CMAKE_INSTALL_DOCDIR} RENAME ${SOURCE_FILE})
  else()
    string(REGEX REPLACE ".*(.)$" "man\\1" SUBDIR ${MAN_PAGE})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MAN_PAGE} DESTINATION ${CMAKE_INSTALL_MANDIR}/${SUBDIR})
  endif()
  # CONFIGURE_FILE does not find out about updates to the sources, and it does not provide a target
  #configure_file(${SOURCE_FILE} ${MAN_PAGE} COPYONLY)
  add_custom_command(OUTPUT ${MAN_PAGE}
    DEPENDS ${SOURCE_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${MAN_PAGE}
    COMMENT "Preparing ${MAN_PAGE}"
    )

  string(REGEX REPLACE "[1-9]$" "html" HTML_FILE ${MAN_PAGE})
  string(REGEX REPLACE "[1-9]$" "man" MAN_FILE ${MAN_PAGE})
  string(REGEX REPLACE "[1-9]$" "mdoc" MDOC_FILE ${MAN_PAGE})

  # html re-generation
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${HTML_FILE}
    DEPENDS ${MDOC_FILE}
    COMMAND ${CMAKE_COMMAND} -DIN=${MDOC_FILE} -DOUT=${HTML_FILE} -DDIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/update-html.cmake
    )
  list(APPEND UPDATEHTML ${CMAKE_CURRENT_SOURCE_DIR}/${HTML_FILE})

  # man re-generation
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${MAN_FILE}
    DEPENDS ${MDOC_FILE}
    COMMAND ${CMAKE_COMMAND} -DIN=${MDOC_FILE} -DOUT=${MAN_FILE} -DDIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/update-man.cmake
    )
  list(APPEND UPDATEMAN ${CMAKE_CURRENT_SOURCE_DIR}/${MAN_FILE})
endforeach()
add_custom_target(man ALL DEPENDS ${MAN_PAGES})
add_custom_target(update-man DEPENDS ${UPDATEMAN})
add_custom_target(update-html DEPENDS ${UPDATEHTML})
