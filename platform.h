#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef _WIN32
#define _BSD_SOURCE
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>										// needed for play_nice routines
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>        // needed for sleep command
#include <direct.h>         // needed for getcwd
#include <process.h>
#include <io.h>
#include <locale.h>
#include <excpt.h>
#include <winbase.h>
#define inline __inline
#endif

#ifdef _WIN32
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
#define MAX_PATH _MAX_PATH
#elif !defined(MAX_PATH) // MSVC
#define MAX_PATH FILENAME_MAX
#endif // MinGW32,64
#elif  __unix__ // Linux
#define MAX_PATH _POSIX_PATH_MAX
#elif __APPLE__ // MacOSX
#define MAX_PATH PATH_MAX
#else
#error "MAX_PATH is undefined"
#endif

#define bool  int
#define false 0
#define true  1

#ifdef _WIN32
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#ifdef _WIN32
typedef int fileh;
typedef struct _stati64* stath;
#define PATH_SEPARATOR '\\'
#else
typedef FILE* fileh;
typedef struct stat* stath;
#define PATH_SEPARATOR '/'
#endif

int mystat(char * f, stath s);
fileh myfopen(const char * f, char * m);
int myremove(char * f);

#ifndef _WIN32
#define _read read
#define _write write
#define _close close
#define _cprintf printf
#define _flushall() fflush(NULL)
#define _getcwd(x, y) getcwd(x, y)
#define Sleep(x) usleep((x)*1000L)
int min(int i,int j);
int max(int i,int j);
char *_strupr(char *string);
#endif

#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
#include <sys/timeb.h>

struct timeval {
    long tv_sec;
    long tv_usec;
};
void gettimeofday (struct timeval * tp, void * dummy);
#endif

#endif
