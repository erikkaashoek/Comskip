AC_INIT([Comskip], 0.88.89)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC

AC_CANONICAL_HOST

platform_win32=no
platform_darwin=no
case $host_os in
  *mingw* )
    platform_win32=yes
    ;;
  *darwin* )
    platform_darwin=yes
    ;;
esac
AC_MSG_CHECKING([whether host is win32])
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" = "xyes"])
AC_MSG_CHECKING([whether host is darwin])
AC_MSG_RESULT([$platform_darwin])

PKG_CHECK_MODULES(argtable2, [argtable2 >= 2.12])
PKG_CHECK_MODULES(ffmpeg, [libavutil >= 2.1 libavformat >= 2.1 libavcodec >= 2.1])

AC_ARG_ENABLE([donator], [AS_HELP_STRING([--enable-donator], [build with donator features])])
AC_MSG_CHECKING([whether to enable donator features])
AS_IF([test "x${enable_donator}" = "xyes"], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))
AM_CONDITIONAL([ENABLE_DONATOR], [test "x${enable_donator}" = "xyes"])

AC_ARG_ENABLE([static], [AS_HELP_STRING([--enable-static], [build a statically linked binary])])
AC_MSG_CHECKING([whether to build a statically linked binary])
AS_IF([test "x$platform_darwin" = "xyes"], AC_MSG_RESULT([static linking not supported on darwin], AS_IF([test "x$enable_static" = "xyes"], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))))
AS_IF([test "x$platform_darwin" = "xyes"], enable_static=no)
AM_CONDITIONAL([ENABLE_STATIC], [test "x${enable_static}" = "xyes"])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
