/*
 * mpglibdll.h - Interface file for mpglib.dll
 * See "example.c" how to use it.
 *
 * NOTE: The memory where mpstr is pointing to will be
 * handled by the mpglib as a complex struct which
 * needs about 20-30KB. For applications that use
 * the mpglib.dll it should be enough to allocate
 * the memory - mpglib knows what to do with it.
 * For details see mpglib/mpg123.h and mpglib/mpglib.h
 *
 * WARNING: If decodeMP3 returns MP3_ERR you should
 * instantly reinitialize the mpglib. Otherwise it could
 * crash. Originally the mpglib exits if a heavy error occurs.
 * This is disabled in the mpglib.dll (#define BE_QUIET)
 * to force playback of corrupted MP3 files.
 */

#include "../config.h"

struct mpstr { char c[40000]; };

#define ML_ERR       -1
#define ML_OK         0
#define ML_NEED_MORE  1

#define		ML_MAX_HOMEPAGE			256

typedef struct
{
	// Vorbis DLL Version number

	BYTE	byDLLMajorVersion;
	BYTE	byDLLMinorVersion;

	// Vorbis Engine Version Number

	BYTE	byMajorVersion;
	BYTE	byMinorVersion;

	// DLL Release date

	BYTE	byDay;
	BYTE	byMonth;
	WORD	wYear;

	// Vorbis	Homepage URL

	CHAR	zHomepage[ML_MAX_HOMEPAGE + 1];	

} ML_VERSION, *PML_VERSION;			



#ifndef _WIN32
#define MLINIT mlInit
#define MLEXIT mlExit
#define MLDECODE mlDecode
#define MLVERSION mlVersion
#endif


typedef BOOL (*MLINIT)   (struct mpstr *mp, long scale);
typedef void (*MLEXIT)   (struct mpstr *mp);
typedef int  (*MLDECODE) (struct mpstr *mp, char *inmemory, int inmemsize,
                           char *outmemory,  int outmemsize, int *done);

typedef void (*MLVERSION) (PML_VERSION);

#define TEXT_MLVERSION "mlVersion"
#define TEXT_MLINIT   "mlInit"
#define TEXT_MLEXIT   "mlExit"
#define TEXT_MLDECODE "mlDecode"
