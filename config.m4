dnl config.m4 for extension fast_copy

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary.

dnl If your extension references something external, use 'with':

PHP_ARG_WITH([fast_copy],
  [for fast_copy support],
  [AS_HELP_STRING([--with-fast_copy],
    [Include fast_copy support])])

if test "$PHP_FAST_COPY" != "no"; then
  PKG_CHECK_MODULES([LIBURING], [liburing], [
    PHP_EVAL_LIBLINE($LIBURING_LIBS, LIBURING_SHARED_LIBADD)
    PHP_EVAL_INCLINE($LIBURING_CFLAGS)
  ], [
    dnl If pkg-config is not available, fallback to manual search

    dnl Define the search paths for the header file
    SEARCH_PATH="/usr/include"     
    SEARCH_FOR="liburing.h"

    dnl Check if the header exists in the given path
    if test -r $SEARCH_PATH/$SEARCH_FOR; then
      LIBURING_DIR=$SEARCH_PATH
      AC_MSG_RESULT([liburing.h found in $LIBURING_DIR])
    else
      AC_MSG_ERROR([liburing.h not found, please install liburing.])
    fi

    dnl Add the include path and library for manual linking
    PHP_ADD_INCLUDE([$LIBURING_DIR])
    PHP_ADD_LIBRARY_WITH_PATH([uring], [/usr/lib], [LIBURING_SHARED_LIBADD])
  ])
  
  PHP_ADD_LIBPATH(/usr/include)
  PHP_ADD_LIBRARY([uring])

  dnl In case of no dependencies
  AC_DEFINE(HAVE_LIBURING, 1, [ Have liburing support ])

  PHP_NEW_EXTENSION(fast_copy, fast_copy.c, $ext_shared)
  PHP_INSTALL_HEADERS([ext/fast_copy], [php_fast_copy.h])
  PHP_SUBST(FAST_COPY_SHARED_LIBADD)
fi
