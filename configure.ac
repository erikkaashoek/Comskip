AC_INIT([Comskip], m4_esyscmd_s([ sed -nEe 's/.*PACKAGE_STRING[^0-9.]*([0-9.]*).*/\1/p' comskip.h ]), [https://github.com/erikkaashoek/Comskip/issues])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

: ${CFLAGS=""}
AC_PROG_CC

AC_CHECK_LIB([m],[fmax])
AC_CHECK_LIB([pthread],[sem_post])

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

AC_ARG_ENABLE([donator], [AS_HELP_STRING([--disable-donator], [build without donator features])])
AC_MSG_CHECKING([whether to enable donator features])
AS_IF([test "x${enable_donator}" = "xno"], AC_MSG_RESULT([no]), AC_MSG_RESULT([yes]))
AM_CONDITIONAL([ENABLE_DONATOR], [test "x${enable_donator}" != "xno"])

AC_ARG_ENABLE([gui], AS_HELP_STRING([--disable-gui], [build without the graphical user interface]))
AC_MSG_CHECKING([whether to enable the graphical user interface])
AS_CASE([x${enable_gui}], [xno], [AC_MSG_RESULT([no])], [x?*], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([if possible])])

AC_ARG_ENABLE([static], [AS_HELP_STRING([--enable-static], [build a statically linked binary])])
AC_MSG_CHECKING([whether to build a statically linked binary])
AS_IF([test "x${enable_static}" = "xyes"], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))
AS_IF([test "x${platform_darwin}" = "xyes"], static_c_lib=no,
    [test "x${enable_static}" = "xyes"], static_c_lib=yes, static_c_lib=no)
AM_CONDITIONAL([ENABLE_STATIC], [test "x${static_c_lib}" = "xyes"])

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [build with debugging symbols and verbose output])])
AC_MSG_CHECKING([whether to build with debugging options enabled])
AS_IF([test "x${enable_debug}" = "xyes"], AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]))
AM_CONDITIONAL([ENABLE_DEBUG], [test "x${enable_debug}" = "xyes"])

PKG_PROG_PKG_CONFIG
AS_IF([test "x${enable_static}" = "xyes"], PKG_CONFIG="$PKG_CONFIG --static")
PKG_CHECK_MODULES(argtable2, [argtable2 >= 2.12])
PKG_CHECK_MODULES(ffmpeg, [libavutil >= 54.7 libavformat >= 57.33.100 libavcodec >= 57.37.100 libswscale >= 5.0])

AS_IF([test "x${enable_gui}" = "xno"], [
    has_sdl=no
], [
    AS_IF([test "x${enable_gui}" != "x"], [
        PKG_CHECK_MODULES(sdl, [sdl2 >= 2.0.0], has_sdl=yes)
    ], [
        PKG_CHECK_MODULES(sdl, [sdl2 >= 2.0.0], has_sdl=yes, has_sdl=no)
    ])

    AC_MSG_CHECKING([whether we can build the graphical user interface])
    AC_MSG_RESULT([$has_sdl])
])

AM_CONDITIONAL([BUILD_GUI], [test "x${has_sdl}" = "xyes"])
AM_PROG_CC_C_O

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
