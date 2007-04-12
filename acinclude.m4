dnl Usage: NIH_REPLACE(func)

AC_DEFUN([NIH_REPLACE_BSWAP],
[AC_CHECK_HEADERS([sys/endian.h])
AC_MSG_CHECKING(for $1)
  AC_CACHE_VAL(nih_cv_check_bswap_$1,
   [AC_TRY_LINK([#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif],
[int i; $1(i);],
[nih_cv_check_bswap_$1=yes], [nih_cv_check_bswap_$1=no])])
if test "x$nih_cv_check_bswap_$1" = xyes; then
AC_MSG_RESULT(yes)
AC_DEFINE([HAVE_]translit($1, [a-z], [A-Z]), 1, [Define if you have $1])
else
AC_MSG_RESULT(no)
AC_LIBOBJ($1)
fi])
