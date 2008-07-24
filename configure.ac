AC_PREREQ(2.54)
AC_INIT([ckmame],[0.9a],[ckmame@nih.at])
AC_CONFIG_SRCDIR([src/ckmame.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_RANLIB

AC_CHECK_HEADERS([stdbool.h])

AC_CHECK_TYPES([int8_t])
AC_CHECK_TYPES([int16_t])
AC_CHECK_TYPES([int32_t])
AC_CHECK_TYPES([int64_t])
AC_CHECK_TYPES([uint8_t])
AC_CHECK_TYPES([uint16_t])
AC_CHECK_TYPES([uint32_t])
AC_CHECK_TYPES([uint64_t])

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([off_t])

AC_CHECK_LIB(z, compress2)
# find libzip
PKG_CHECK_MODULES(LIBZIP, libzip >= 0.7.1)

ZIPCMP=`pkg-config --variable=zipcmp libzip`
AC_SUBST(ZIPCMP, $ZIPCMP)

PKG_CHECK_MODULES(SQLITE3, sqlite3 >= 3.0)

AC_HEADER_DIRENT

AC_REPLACE_FUNCS(fnmatch getprogname strlcpy getopt_long)
NIH_REPLACE_BSWAP(bswap16)
NIH_REPLACE_BSWAP(bswap32)
AC_CHECK_FUNCS(MD5Init, , [AC_LIBOBJ([md5])])
AC_CHECK_FUNCS(SHA1Init, , [AC_LIBOBJ([sha1])])
AC_CHECK_FUNCS(fseeko)
AC_SUBST([MANFMT])
AC_SUBST([DBLIBS])
AC_SUBST([DBTYPE])


case $host_os
in
	*bsd*) MANFMT=mdoc;;
	*) MANFMT=man;;
esac

AC_EXEEXT

PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= 2.0, 
			   AC_DEFINE([HAVE_LIBXML2], [1],
			   [Define if you have the xml2 library (-lxml2)]),
			   AC_MSG_WARN(libxml2 not found))

AC_CONFIG_FILES([Makefile
		 docs/Makefile
		 regress/Makefile
		 src/Makefile])
AC_OUTPUT
