#ifndef COMSKIP_CONFIG_H
#define COMSKIP_CONFIG_H
/* vc++/config.h - manually adapted from include/config.h.in */

/* Suppress warnings relating to mismatched declarations */
#ifdef _WIN32
#pragma warning (disable:4028)
#endif

/* autodetect accelerations */
#define ACCEL_DETECT

/* alpha architecture */
/* #undef ARCH_ALPHA */

#ifdef __POWERPC__
/* ppc architecture */
#define ARCH_PPC
#endif

/* sparc architecture */
/* #undef ARCH_SPARC */

#ifndef __POWERPC__
/* x86 architecture */
#define ARCH_X86
#endif

/* maximum supported data alignment */
/* #undef ATTRIBUTE_ALIGNED_MAX */

/* debug mode configuration */
/* #undef DEBUG */

#ifdef __POWERPC__
/* Define to 1 if you have the <altivec.h> header file. */
#define HAVE_ALTIVEC_H
#endif

/* Define if you have the `__builtin_expect' function. */
/* #undef HAVE_BUILTIN_EXPECT */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `ftime' function. */
#define HAVE_FTIME 1

/* Define to 1 if you have the `gettimeofday' function. */
/* #undef HAVE_GETTIMEOFDAY */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the type `struct timeval'. */
/* #undef HAVE_STRUCT_TIMEVAL */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
/* #undef HAVE_TIME_H */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* libvo DirectX support */
#define LIBVO_DX

/* libvo SDL support */
/* #undef LIBVO_SDL */

/* libvo X11 support */
/* #undef LIBVO_X11 */

/* libvo Xv support */
/* #undef LIBVO_XV */

/* mpeg2dec profiling */
/* #undef MPEG2DEC_GPROF */

/* Name of package */
#define PACKAGE "mpeg2dec"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* The size of a `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of a `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of a `void*', as computed by sizeof. */
#define SIZEOF_VOIDP 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
/* #undef TIME_WITH_SYS_TIME */

/* Version number of package */
#define VERSION "0.4.1-cvs"

#ifdef __POWERPC__
/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#define WORDS_BIGENDIAN
#endif

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#ifndef inline
#define inline __inline
#endif
#endif

/* Define as `__restrict' if that's what the C compiler calls it, or to
   nothing if it is not supported. */
// #define restrict __restrict
#define restrict 

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

#ifndef _WIN32

#include <stdarg.h> /* for va_start, va_end */
#include <unistd.h> /* for usleep */
#include <string.h> /* for memset (ZeroMemory macro) */

/* type/function remappings */
#define LLD_FORMAT "%lld"
#define __int64 long long
#define _write write
#define _close close
#define _cprintf printf
#define _getcwd getcwd
#define _read read
#define _stat stat
#define _lseeki64 fseeko
#define _fseeki64 fseeko
#define _ftelli64 ftello
#define _flushall() fflush(0)
#define Sleep(a) usleep(1000*(a))
#define vo_wait()
#define vo_refresh()

#define MAX_PATH 2048

#define UNALIGNED
#define boolean unsigned char
#define BYTE unsigned char
#define BOOL unsigned char
#define CHAR unsigned char
#define WORD short /* two bytes */
#define DWORD unsigned long  /* four bytes */

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define __forceinline inline
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define ZeroMemory(buf,len) memset(buf,0,len)

#define HAVE_LRINTF
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_STRINGS_H
#define HAVE_GETOPT_H
//#define USE_ASF 0


#else
#define LLD_FORMAT "%I64d"
#define USE_ASF 1
#endif

#endif /* COMSKIP_CONFIG_H */
