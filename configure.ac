AC_PREREQ(2.54)
AC_INIT([ckmame],[0.7],[nih@giga.or.at])
AC_CONFIG_SRCDIR([src/ckmame.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

# We want these before the checks, so the checks can modify their values.
test -z "$CFLAGS" && CFLAGS='-Wall -g' auto_cflags=1

AC_PROG_CC
AC_PROG_RANLIB

dnl they are already checked
dnl AC_CHECK_HEADERS([stdint.h], [], [AC_CHECK_HEADERS([inttypes.h])])

AC_CHECK_TYPE([uint32_t], [],
	      [AC_CHECK_SIZEOF([unsigned int])
	       AC_CHECK_SIZEOF([unsigned long])])

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([off_t])

AC_CHECK_LIB(z, compress2)
# find libzip
PKG_CHECK_MODULES(LIBZIP, libzip >= 0.5)

ZIPCMP=`pkg-config --variable=zipcmp libzip`
AC_SUBST(ZIPCMP, $ZIPCMP)

# find usable db library
db=none
AC_CHECK_HEADERS([db.h db_185.h db2/db_185.h db3/db_185.h db4/db_185.h],
		 [break])
AC_CHECK_FUNCS(dbopen, [db=db; DBLIBS=''])
if test "$db" = "none"
then
    AC_CHECK_LIB(db, dbopen, [db=db; DBLIBS='-ldb'],
	[AC_CHECK_LIB(db2, dbopen, [db=db; DBLIBS='-ldb2'],
	    [AC_CHECK_LIB(__db185_open, db3, [db=db; DBLIBS='-ldb3'],
		[AC_CHECK_LIB(__db185_open, db3, [db=db; DBLIBS='-ldb4'])])])])
fi
if test "$db" = "none"
then
	AC_MSG_ERROR(no supported data base library found)
fi
DBTYPE=$db

AC_HEADER_DIRENT

AC_REPLACE_FUNCS(fnmatch)
AC_CHECK_FUNCS(getopt_long, , [AC_LIBOBJ([getopt1]) AC_LIBOBJ([getopt])])
AC_CHECK_FUNCS(MD5Init, , [AC_LIBOBJ([md5])])
AC_CHECK_FUNCS(SHA1Init, , [AC_LIBOBJ([sha1])])
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
