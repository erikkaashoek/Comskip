lib_LTLIBRARIES = libargtable2.la

libargtable2_la_SOURCES = argtable2.c arg_end.c arg_rem.c arg_lit.c arg_int.c arg_dbl.c \
			arg_str.c arg_file.c

#libtool library versioning uses current:revision:age where
#   current  = most recent interface number
#   revision = minor version of current interface
#   age      = how many previous interfaces this version supports (first=current-age)
# argtable versions prior to 2.4 did not use libtool versions thus
# argtable-2.4 is 0:0:0
# argtable-2.5 is 1:0:1	(interface 1 is back-compatible with interface 0, ie:argtable-2.4)
# argtable-2.6 is 1:1:1	(this code is identical to argtable2-5)
# argtable-2.7 is 1:2:1 (some minor bug fixes, interface is unchanged)
libargtable2_la_LDFLAGS = -version-info 1:2:1

include_HEADERS = argtable2.h

# If the system provides getopt_long then we use it,
# otherwise we compile in the version supplied with argtable.
if !USE_SYS_GETOPTLONG
libargtable2_la_SOURCES += getopt.c getopt1.c getopt.h
endif

# If the system provides regex then we may compile arg_rex
# otherwise we must omit it.
if USE_ARGREX
libargtable2_la_SOURCES += arg_rex.c
endif

# If the system provides strptime then we may compile arg_date
# otherwise we must omit it.
if USE_ARGDATE
libargtable2_la_SOURCES += arg_date.c
endif


