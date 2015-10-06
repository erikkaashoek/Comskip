AC_INIT([Comskip], 0.88.89)
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC

AC_ARG_ENABLE([static], [AS_HELP_STRING([--enable-static],
  [build a statically linked binary])], [], [enable_static=no])
AS_IF([test "x$enable_static" != xno], LDFLAGS += "-static")

PKG_CHECK_MODULES(argtable2, [argtable2 >= 2.12])
PKG_CHECK_MODULES(ffmpeg, [libavutil >= 2.1 libavformat >= 2.1 libavcodec >= 2.1])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
