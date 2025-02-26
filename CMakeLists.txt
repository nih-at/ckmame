cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

project(ckmame
  VERSION 2.1
  LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

option(RUN_REGRESS "Run regression tests" ON)

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)
include(FindLibXml2)
include(FindSQLite3)
include(FindZLIB)

find_program(MDOCTOOL NAMES mandoc groff)
if(MDOCTOOL)
  set(DOCUMENTATION_FORMAT "mdoc" CACHE STRING "Documentation format")
else()
  find_program(MANTOOL NAMES nroff)
  if(MANTOOL)
    set(DOCUMENTATION_FORMAT "man" CACHE STRING "Documentation format")
  else()
    set(DOCUMENTATION_FORMAT "html" CACHE STRING "Documentation format")
  endif()
endif()

set(PACKAGE_BUGREPORT "ckmame@nih.at")

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION})
add_custom_target(dist
  COMMAND git config tar.tar.xz.command "xz -c"
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ -o ${ARCHIVE_NAME}.tar.gz HEAD
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ -o ${ARCHIVE_NAME}.tar.xz HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
add_custom_target(distcheck
  COMMAND chmod -R u+w ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest 2>/dev/null || true
  COMMAND rm -rf ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND cmake -E tar xf ${ARCHIVE_NAME}.tar.gz
  COMMAND chmod -R u-w ${ARCHIVE_NAME}
  COMMAND mkdir ${ARCHIVE_NAME}-build
  COMMAND mkdir ${ARCHIVE_NAME}-dest
  COMMAND cd ${ARCHIVE_NAME}-build && cmake -DCMAKE_INSTALL_PREFIX=../${ARCHIVE_NAME}-dest ../${ARCHIVE_NAME}
  COMMAND cd ${ARCHIVE_NAME}-build && make -j4
  COMMAND cd ${ARCHIVE_NAME}-build && make test
  COMMAND cd ${ARCHIVE_NAME}-build && make install
#  COMMAND cd ${ARCHIVE_NAME}-build && make uninstall
#  COMMAND if [ `find ${ARCHIVE_NAME}-dest ! -type d | wc -l` -ne 0 ]; then echo leftover files in ${ARCHIVE_NAME}-dest; false; fi
  COMMAND cd ${ARCHIVE_NAME}-build && make clean
  COMMAND chmod -R u+w ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND rm -rf ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND echo "${ARCHIVE_NAME}.tar.gz is ready for distribution."
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
add_dependencies(distcheck dist)

check_function_exists(MD5Init HAVE_MD5INIT)
check_function_exists(SHA1Init HAVE_SHA1INIT)
check_function_exists(SHA256Init HAVE_SHA256INIT)
check_function_exists(fnmatch HAVE_FNMATCH)

if(NOT ZLIB_FOUND)
  message(ERROR "-- zlib library not found (required)")
endif()

find_package(libzip 1.10 REQUIRED)

if(NOT SQLite3_FOUND)
  message(ERROR "-- sqlite3 library not found (required)")
endif()

if(LibXml2_FOUND)
  set(HAVE_LIBXML2 1)
else()
  message(WARNING "-- libxml2 library not found; XML support disabled")
endif()

find_package(LibArchive)

if (LibArchive_FOUND)
  set(HAVE_LIBARCHIVE 1)
else()
  message(WARNING "-- libarchive not found; 7z read support disabled")
endif()

# install with rpath
if(NOT CMAKE_SYSTEM_NAME MATCHES Linux)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# disable warnings about strerror and other 'unsafe' functions
if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
endif(MSVC)

if (NOT PYTHONBIN)
  find_package (Python COMPONENTS Interpreter)
  set (PYTHONBIN ${Python_EXECUTABLE})
else()
  message("Python set to ${PYTHONBIN}")
endif()

find_program(NIHTEST nihtest)
set(NIHTEST_REQUIRED_VERSION "1.9.1")

if(RUN_REGRESS)
  if (NOT NIHTEST OR NOT PYTHONBIN)
    message(WARNING "-- nihtest or Python interpreter not found, regression testing disabled")
    set(RUN_REGRESS OFF)
  else()
    execute_process(COMMAND ${NIHTEST} --version OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NIHTEST_VERSION_RAW)
    string(REGEX REPLACE "[^ ]* " "" NIHTEST_VERSION ${NIHTEST_VERSION_RAW})
    if (${NIHTEST_VERSION} VERSION_LESS ${NIHTEST_REQUIRED_VERSION})
      message(WARNING "-- nihtest ${NIHTEST_VERSION} too old, at least ${NIHTEST_REQUIRED_VERSION} required, regression testing disabled")
      set(RUN_REGRESS OFF)
    endif()
  endif()
endif()

# Testing
enable_testing()

# Targets
add_subdirectory(docs)
add_subdirectory(regress)
add_subdirectory(src)

# write out config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# for code completion frameworks
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# for tests

set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/regress)
set(abs_srcdir ${CMAKE_CURRENT_SOURCE_DIR}/regress)
set(top_builddir ${CMAKE_BINARY_DIR})
