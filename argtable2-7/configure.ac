#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# AC_INIT(PACKAGE,VERSION,maintainer, TARNAME)
AC_INIT(argtable2, 7, sheitmann@users.sourceforge.net)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR(src/argtable2.c)
AM_CONFIG_HEADER(src/config.h)
    
# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h stdlib.h string.h strings.h unistd.h getopt.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero strchr strcspn strrchr strtol])
AC_CHECK_FUNC(getopt_long, SYS_GETOPTLONG=1, SYS_GETOPTLONG=0)
AC_CHECK_FUNC(regcomp,     SYS_REGEX=1,      SYS_REGEX=0)
AC_CHECK_FUNC(strptime,    SYS_STRPTIME=1,   SYS_STRPTIME=0)

# Define automake conditionals
AM_CONDITIONAL(USE_SYS_GETOPTLONG, test "$SYS_GETOPTLONG" = "1")
AM_CONDITIONAL(USE_ARGREX,         test "$SYS_REGEX" = "1")
AM_CONDITIONAL(USE_ARGDATE,        test "$SYS_STRPTIME" = "1")

# check for --enable-debug argument
AC_ARG_ENABLE(
    [debug],
    AC_HELP_STRING([--enable-debug],[enable library debugging symbols]),
    [
       case $enableval in
          yes) echo "enabling debugging symbols" & DEBUGFLAGS="-g -UNDEBUG";;               
          no)  echo "disabling debugging symbols" & DEBUGFLAGS="-DNDEBUG";;
          *)   echo "illegal argument to --enable-debug" & exit 1;;
       esac
    ],
    [DEBUGFLAGS="-DNDEBUG"]
    )
AC_SUBST(DEBUGFLAGS)
# Additional paths
AC_ARG_VAR([docdir], [Directory where argtable docs are stored])
[if test "x$docdir" = "x"
then
    docdir='${prefix}/share/doc/argtable2'
fi]
AC_SUBST(exampledir, ['${docdir}/example'])

AC_CONFIG_FILES([Makefile
                 argtable.spec
                 example/Makefile
                 src/Makefile
                 doc/Makefile
                 doc/argtable2.3
                 doc/argtable2.html
                 tests/Makefile])
AC_OUTPUT
