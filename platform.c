#include "platform.h"

#if defined(_WIN32)
BOOL AnsiToUnicode16(const char *in_Src, WCHAR *out_Dst, INT in_MaxLen)
{
    /* locals */
    INT lv_Len;
    int i;
    // do NOT decrease maxlen for the eos
    if (in_MaxLen <= 0)
        return FALSE;
    // let windows find out the meaning of ansi
    // - the SrcLen=-1 triggers MBTWC to add a eos to Dst and fails if MaxLen is too small.
    // - if SrcLen is specified then no eos is added
    // - if (SrcLen+1) is specified then the eos IS added
    lv_Len = MultiByteToWideChar(CP_UTF8, 0, in_Src, -1, out_Dst, in_MaxLen);
/*
    for (i = 0; i < strlen(in_Src); i++)
    {
    fprintf(stderr, "[%i]=%i\n", i, in_Src[i]);

    }
*/
    // validate
    if (lv_Len < 0)
        lv_Len = 0;
    // ensure eos, watch out for a full buffersize
    // - if the buffer is full without an eos then clear the output like MBTWC does
    //   in case of too small outputbuffer
    // - unfortunately there is no way to let MBTWC return shortened strings,
    //   if the outputbuffer is too small then it fails completely
    if (lv_Len < in_MaxLen)
        out_Dst[lv_Len] = 0;
    else if (out_Dst[in_MaxLen-1])
        out_Dst[0] = 0;
    // done
    return TRUE;
}

int mystat(char * f, stath s)
{
    wchar_t wf[2000];
    int n;
    n= AnsiToUnicode16(f, wf, 2000);
    return(_wstati64(wf,s));
}

fileh myfopen(const char * f, char * m)
{
    wchar_t wf[2000], wm[2000];
    int n;

    n= AnsiToUnicode16(f, wf, 2000);
    n= AnsiToUnicode16(m, wm, 2000);
    return(_wfopen(wf,wm));
}

int myremove(char * f)
{
    wchar_t wf[2000];
    int n;
    n= AnsiToUnicode16(f, wf, 2000);
    return(_wremove(wf));
}
#endif

#if !defined(_WIN32)
int mystat(char * f, stath s)
{
  return stat(f, s);
}

fileh myfopen(const char * f, char * m)
{
  return fopen(f, m);
}

int myremove(char * f)
{
  return unlink(f);
}
#endif

#if !defined(_WIN32)
int min(int i, int j)
{
  return(i<j?i:j);
}

int max(int i, int j)
{
  return(i>j?i:j);
}

char *_strupr(char *string)
{
    char *s;

    if (string)
    {
        for (s = string; *s; ++s)
            *s = toupper(*s);
    }
    return string;
}
#endif

#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
void gettimeofday (struct timeval * tp, void * dummy)
{
    struct _timeb tm;
    _ftime (&tm);
    tp->tv_sec = tm.time;
    tp->tv_usec = tm.millitm * 1000;
}
#endif
