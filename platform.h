#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef _WIN32
#define _BSD_SOURCE
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>
#else
#undef WINVER
#define WINVER _WIN32_WINNT_WIN7
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
#include <sys/time.h>
#include <inttypes.h>

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

#ifdef _POSIX_ARG_MAX
#define MAX_ARG _POSIX_ARG_MAX
#elif defined(ARG_MAX)
#define MAX_ARG ARG_MAX
#else
#define MAX_ARG MAX_PATH
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
#ifndef HARDWARE_DECODE
#include <compat/w32pthreads.h>  // Is already defined in ffmpeg
#endif

#include <time.h>
#else
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#endif

#ifdef _WIN32
typedef HANDLE sema_t;
#define sema_init(s,v) (s = CreateSemaphore(NULL, v, LONG_MAX, NULL))
#define sema_wait(s) WaitForSingleObject(s, INFINITE)
#define sema_post(s) ReleaseSemaphore(s, 1, NULL)
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t sema_t;
#define sema_init(s,v) (s = dispatch_semaphore_create(v))
#define sema_wait(s) dispatch_semaphore_wait(s, DISPATCH_TIME_FOREVER)
#define sema_post(s) dispatch_semaphore_signal(s)
#else
#include <semaphore.h>
typedef sem_t sema_t;
#define sema_init(s,v) sem_init(&s,0,v)
#define sema_wait(s) sem_wait(&s)
#define sema_post(s) sem_post(&s)
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
typedef FILE* fileh;
typedef struct _stati64* stath;
#elif defined(_WIN32)
typedef int fileh;
typedef struct _stati64* stath;
#else
typedef FILE* fileh;
typedef struct stat* stath;
#endif

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
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
