AC_PREREQ(2.54)
AC_INIT([ckmame],[0.5],[nih@giga.or.at])
AC_CONFIG_SRCDIR([ckmame.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

# We want these before the checks, so the checks can modify their values.
test -z "$CFLAGS" && CFLAGS='-Wall -g' auto_cflags=1


AC_PROG_CC
AC_PROG_RANLIB


AC_ARG_WITH(zlib,
    [  --with-zlib=PREFIX  specify prefix for ZLIB library],,
    with_zlib=yes)

if test "$with_zlib" != "yes"
then
    if test -f "$with_zlib"/zlib.h
    then
	# PREFIX is to uninstalled version in distribution directory
	CFLAGS="$CFLAGS -I$with_zlib"
	LDFLAGS="$LDFLAGS -L$with_zlib"
    else if test -f "$with_zlib"/include/zlib.h
    then
	# PREFIX is installation prefix
	CFLAGS="$CFLAGS -I$with_zlib/include"
	LDFLAGS="$LDFLAGS -L$with_zlib/lib"
    fi
    fi
fi

AC_CHECK_LIB(z, main)
AC_CACHE_CHECK(new ZLIB version, id_cv_lib_zlib_ok, 
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <zlib.h>]], [[extern ZEXPORT unzOpen (const char *path);]])],[id_cv_lib_zlib_ok=yes],[id_cv_lib_zlib_ok=no]))
if test "$id_cv_lib_zlib_ok" = "no"
then
    AC_MSG_ERROR([ZLIB version too old, please install at least v1.1.2])
fi


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
AC_SUBST(DBLIBS)

case $host_os
in
	*bsd*) MANFMT=mdoc;;
	*) MANFMT=man;;
esac

AC_CONFIG_LINKS([dbl-int.h:db-${db}.h dbl-int.c:db-${db}.c ckmame.1:ckmame.${MANFMT}])

AC_EXEEXT

AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile libzip/Makefile libzip/tests/Makefile
		 regress/Makefile])
AC_OUTPUT
