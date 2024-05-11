dnl config.m4 for extension fast_copy

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary.

dnl If your extension references something external, use 'with':

dnl PHP_ARG_WITH([fast_copy],
dnl   [for fast_copy support],
dnl   [AS_HELP_STRING([--with-fast_copy],
dnl     [Include fast_copy support])])

dnl Otherwise use 'enable':

PHP_ARG_ENABLE([fast_copy],
  [whether to enable fast_copy support],
  [AS_HELP_STRING([--enable-fast_copy],
    [Enable fast_copy support])],
  [no])

if test "$PHP_FAST_COPY" != "no"; then
  dnl Write more examples of tests here...

  dnl Remove this code block if the library does not support pkg-config.
  dnl PKG_CHECK_MODULES([LIBFOO], [foo])
  dnl PHP_EVAL_INCLINE($LIBFOO_CFLAGS)
  dnl PHP_EVAL_LIBLINE($LIBFOO_LIBS, FAST_COPY_SHARED_LIBADD)

  dnl If you need to check for a particular library version using PKG_CHECK_MODULES,
  dnl you can use comparison operators. For example:
  dnl PKG_CHECK_MODULES([LIBFOO], [foo >= 1.2.3])
  dnl PKG_CHECK_MODULES([LIBFOO], [foo < 3.4])
  dnl PKG_CHECK_MODULES([LIBFOO], [foo = 1.2.3])

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-fast_copy -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/fast_copy.h"  # you most likely want to change this
  dnl if test -r $PHP_FAST_COPY/$SEARCH_FOR; then # path given as parameter
  dnl   FAST_COPY_DIR=$PHP_FAST_COPY
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for fast_copy files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FAST_COPY_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FAST_COPY_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the fast_copy distribution])
  dnl fi

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-fast_copy -> add include path
  dnl PHP_ADD_INCLUDE($FAST_COPY_DIR/include)

  dnl Remove this code block if the library supports pkg-config.
  dnl --with-fast_copy -> check for lib and symbol presence
  dnl LIBNAME=FAST_COPY # you may want to change this
  dnl LIBSYMBOL=FAST_COPY # you most likely want to change this

  dnl If you need to check for a particular library function (e.g. a conditional
  dnl or version-dependent feature) and you are using pkg-config:
  dnl PHP_CHECK_LIBRARY($LIBNAME, $LIBSYMBOL,
  dnl [
  dnl   AC_DEFINE(HAVE_FAST_COPY_FEATURE, 1, [ ])
  dnl ],[
  dnl   AC_MSG_ERROR([FEATURE not supported by your fast_copy library.])
  dnl ], [
  dnl   $LIBFOO_LIBS
  dnl ])

  dnl If you need to check for a particular library function (e.g. a conditional
  dnl or version-dependent feature) and you are not using pkg-config:
  dnl PHP_CHECK_LIBRARY($LIBNAME, $LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FAST_COPY_DIR/$PHP_LIBDIR, FAST_COPY_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FAST_COPY_FEATURE, 1, [ ])
  dnl ],[
  dnl   AC_MSG_ERROR([FEATURE not supported by your fast_copy library.])
  dnl ],[
  dnl   -L$FAST_COPY_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FAST_COPY_SHARED_LIBADD)

  dnl In case of no dependencies
  AC_DEFINE(HAVE_FAST_COPY, 1, [ Have fast_copy support ])

  PHP_NEW_EXTENSION(fast_copy, fast_copy.c, $ext_shared)
fi
