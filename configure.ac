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

# find usable db library

db=none

# db 1.85, included in libc
AC_CHECK_FUNCS(dbopen, [AC_CHECK_HEADERS([db.h], [db=db; DBLIBS=''])])

# db 1.85, separate library
if test "$db" = "none"
then
    AC_CHECK_LIB(db, dbopen,
                 [AC_CHECK_HEADERS([db.h], [db=db; DBLIBS='-ldb'])])
fi

# db 2 compatibility mode
if test "$db" = "none"
then
    AC_CHECK_LIB(db2, dbopen,
                 [AC_CHECK_HEADERS([db2/db_185.h db_185.h],
		                   [db=db; DBLIBS='-ldb2'; break])])
fi

# db 3 compatibility mode
if test "$db" = "none"
then
    AC_CHECK_LIB(db3, __db185_open,
                 [AC_CHECK_HEADERS([db3/db_185.h db_185.h],
		                   [db=db; DBLIBS='-ldb3'; break])])
fi

# db 4 compatibility mode
if test "$db" = "none"
then
    AC_CHECK_LIB(db4, __db185_open,
                 [AC_CHECK_HEADERS([db4/db_185.h db_185.h],
		                   [db=db; DBLIBS='-ldb4'; break])])
fi

if test "$db" = "none"
then
	AC_MSG_ERROR(no supported data base library found)
fi

DBTYPE=$db

AC_HEADER_DIRENT

AC_REPLACE_FUNCS(fnmatch getprogname strlcpy)
NIH_REPLACE_BSWAP(bswap16)
NIH_REPLACE_BSWAP(bswap32)
AC_CHECK_FUNCS(getopt_long, , [AC_LIBOBJ([getopt1]) AC_LIBOBJ([getopt])])
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
