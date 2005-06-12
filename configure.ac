AC_PREREQ(2.54)
AC_INIT([ckmame],[0.5],[nih@giga.or.at])
AC_CONFIG_SRCDIR([ckmame.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

# We want these before the checks, so the checks can modify their values.
test -z "$CFLAGS" && CFLAGS='-Wall -g' auto_cflags=1

AC_PROG_CC

dnl they are already checked
dnl AC_CHECK_HEADERS([stdint.h], [], [AC_CHECK_HEADERS([inttypes.h])])

AC_CHECK_TYPE([uint32_t], [],
	      [AC_CHECK_SIZEOF([unsigned int])
	       AC_CHECK_SIZEOF([unsigned long])])

AC_CHECK_LIB(z, compress2)
# find libzip
PKG_CHECK_MODULES(LIBZIP, libzip >= 0.5)

ZIPCMP=`pkg-config --variable=zipcmp libzip`
AC_SUBST(ZIPCMP, $ZIPCMP)

# find usable db library
db=none
check_db=yes
check_gdbm=yes

AC_ARG_WITH(gdbm,
    [  --with-gdbm=PREFIX  specify prefix for GDBM library],,
	 with_gdbm=unspec)

if test "$with_gdbm" = "no"
then
    check_gdbm=no
else if test "$with_gdbm" != "unspec"
then
    if test -f "$with_gdbm"/gdbm.h
    then
	# PREFIX is to uninstalled version in distribution directory
	CFLAGS="$CFLAGS -I$with_gdbm"
	LDFLAGS="$LDFLAGS -L$with_gdbm"
    else if test -f "$with_gdbm"/include/gdbm.h
    then
	# PREFIX is installation prefix
	CFLAGS="$CFLAGS -I$with_gdbm/include"
	LDFLAGS="$LDFLAGS -L$with_gdbm/lib"
    fi
    fi
    check_db=no
fi
fi


if test "$check_db" = "yes"
then
    AC_CHECK_HEADERS([db_185.h])
    AC_CHECK_FUNCS(dbopen, [db=db])
    if test "$db" = "none"
    then
	AC_CHECK_LIB(db, dbopen, [db=db; DBLIBS="-ldb"])
    fi
fi
if test "$db" = "none" -a "$check_gdbm" = "yes"
then
	AC_CHECK_LIB(gdbm, gdbm_open, [db=gdbm; DBLIBS="-lgdbm"])
fi
if test "$db" = "none"
then
	AC_MSG_ERROR(no supported data base library found)
fi

AC_REPLACE_FUNCS(fnmatch)
AC_CHECK_FUNCS(getopt_long, , [AC_LIBOBJ([getopt1]) AC_LIBOBJ([getopt])])
AC_CHECK_FUNCS(MD5Init, , [AC_LIBOBJ([md5])])
AC_CHECK_FUNCS(SHA1Init, , [AC_LIBOBJ([sha1])])
AC_SUBST([DBLIBS])
AC_SUBST([MANFMT])

case $host_os
in
	*bsd*) MANFMT=mdoc;;
	*) MANFMT=man;;
esac

AC_CONFIG_LINKS([dbl-int.h:db-${db}.h dbl-int.c:db-${db}.c])

AC_EXEEXT

PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= 2.0, 
			   AC_DEFINE([HAVE_LIBXML2], [1],
			   [Define if you have the xml2 library (-lxml2)]),
			   AC_MSG_WARN(libxml2 not found))

AC_CONFIG_FILES([Makefile regress/Makefile])
AC_OUTPUT
