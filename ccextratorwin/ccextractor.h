

#include <stdio.h>

#include "608.h"

#ifndef int64_t_C
#define int64_t_C(c)     ((__int64) c )
#define uint64_t_C(c)    ((__int64) c )
#endif

#ifdef _WIN32
#define FOPEN fopen
#define OPEN _open
// 64 bit file functions
//extern int  _fseeki64(FILE *, __int64, int);
//extern __int64 _ftelli64(FILE *);
// Don't bug me with strcpy() deprecation warnings
#pragma warning(disable : 4996)
#define FSEEK _fseeki64
#define FTELL _ftelli64
#define TELL _telli64
#define LSEEK _lseeki64
typedef struct _stati64 FSTATSTRUCT;
// #define LONG long long
// typedef unsigned long long int64_t;
#else
#define FOPEN fopen
#define OPEN open
#define FSEEK fseek
#define FTELL ftell
#define FSTAT fstat
#define LSEEK lseek
#define TELL tell
#endif

typedef signed long LONG;

#define ONEPASS 120 /* Bytes we can always look ahead without going out of limits */
#define BUFSIZE (256*1024+ONEPASS) /* 1 Mb plus the safety pass */
#define MAX_CLOSED_CAPTION_DATA_PER_PICTURE 32
#define TS_PACKET_PAYLOAD_LENGTH     184     // From specs
#define SUBLINESIZE 2048 // Max. length of a .srt line - TODO: Get rid of this
#define STARTBYTESLENGTH	(1024*1024)

struct boundary_time
{
	int hh,mm,ss;
	LONG time_in_ms;
	LONG time_in_ccblocks;
	int set;
};

struct s_write
{
    FILE *fh;
    char *filename;
    unsigned char *buffer;
    int used;
    struct eia608 *data608;
};

enum output_format
{
    OF_RAW	= 0,
    OF_SRT	= 1,
    OF_SAMI = 2
};

enum encoding_type
{
    ENC_UNICODE = 0,
    ENC_LATIN_1 = 1,
    ENC_UTF_8 = 2
} ;

enum frame_type
{
	RESET_OR_UNKNOWN = 0,
    I_FRAME = 1,
    P_FRAME = 2,
    B_FRAME = 3
};

struct gop_time_code
{
  int drop_frame_flag;
  int time_code_hours;
  int time_code_minutes;
  int marker_bit;
  int time_code_seconds;
  int time_code_pictures;
  int inited;
  LONG ccblocks;
};

extern struct gop_time_code gop_time, first_gop_time, printed_gop;
extern int gop_rollover;
extern LONG min_pts, max_pts, last_pts,current_pts;
extern const char *framerates_types[16];
extern int ts_mode;
extern unsigned char *fbuffer;
extern LONG past;

extern unsigned char startbytes[STARTBYTESLENGTH];
extern unsigned int startbytes_pos;
extern unsigned int startbytes_avail;

extern unsigned char *pesheaderbuf;
extern int pts_set; //0 = No, 1 = Just received but not an I-Frame yes, 2 = Ready
extern unsigned last_sync_tenth; // Last tenth of a second with perfect timing
extern unsigned c1count, c2count; // Number of CC 2-byte blocks written
                                // in this continuous chuck
extern unsigned c1global, c2global;
extern unsigned c1count_total, c2count_total;
#define MPEG_CLOCK_FREQ 90000 // This is part of the standard
extern unsigned pts_big_change;
// This two just for debugging
extern unsigned char ptsdata[5]; // Original 5 bytes the MPEG clock came from
extern unsigned char lastptsdata[5]; // Original 5 bytes the previous MPEG clock came from
extern unsigned total_frames_count;

extern LONG buffered_read_opt (unsigned char *buffer, unsigned int bytes);

extern unsigned char *filebuffer;
extern LONG filebuffer_start; // Position of buffer start relative to file
extern int filebuffer_pos; // Position of pointer relative to buffer start
extern int bytesinbuffer; // Number of bytes we actually have on buffer

#define FLUSH_CC_BUFFERS() printdata (captions_buffer_1,used_caption_buffer_1,0,0); \
    c1count+=used_caption_buffer_1/2; \
    used_caption_buffer_1=0; \
    printdata (0,0,captions_buffer_2,used_caption_buffer_2); \
    c2count+=used_caption_buffer_2/2; \
    used_caption_buffer_2=0;

#define buffered_skip(bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    filebuffer_pos+=bytes; \
    result=bytes; \
} else result=buffered_read_opt (NULL,bytes);

#define buffered_read(buffer,bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    if (buffer!=NULL) memcpy (buffer,filebuffer+filebuffer_pos,bytes); \
    filebuffer_pos+=bytes; \
    result=bytes; \
} else result=buffered_read_opt (buffer,bytes);

#define buffered_read_4(buffer) if (4<=bytesinbuffer-filebuffer_pos) { \
    if (buffer) { buffer[0]=filebuffer[filebuffer_pos]; \
    buffer[1]=filebuffer[filebuffer_pos+1]; \
    buffer[2]=filebuffer[filebuffer_pos+2]; \
    buffer[3]=filebuffer[filebuffer_pos+3]; \
    filebuffer_pos+=4; \
    result=4; } \
} else result=buffered_read_opt (buffer,4);

#define buffered_read_byte(buffer) if (bytesinbuffer-filebuffer_pos) { \
    if (buffer) { *buffer=filebuffer[filebuffer_pos]; \
    filebuffer_pos++; \
    result=1; } \
} else result=buffered_read_opt (buffer,1);

// extern FILE *in, *clean;
extern FILE *clean;
extern int in;
extern const char *aspect_ratio_types[16];
extern const char *pict_types[8];
extern const char *cc_types[4];
extern int false_pict_header;

extern int stat_numuserheaders;
extern int stat_dvdccheaders;
extern int stat_replay5000headers;
extern int stat_replay4000headers;
extern int stat_dishheaders;
extern int stat_hdtv;
extern int autopad ;
extern int gop_pad;
extern int ff_cleanup;
extern int ts_mode;
extern int debug;
extern int fix_padding;
extern int rawmode;
extern int extract;
extern int cc_stats[4];
extern LONG inputsize;
extern int cc_channel;
extern int encoding ; //encoding_type
extern int direct_rollup;
extern LONG subs_delay;
extern struct boundary_time extraction_start, extraction_end;
extern LONG screens_to_process;
extern int processed_enough;
extern int nofontcolor;
extern unsigned char usercolor_rgb[8];
extern int default_color; //color_code
extern int sentence_cap;

void dump (unsigned char *start, int l);
void printdata (const unsigned char *data1, int length1,const unsigned char *data2, int length2);
// void flush_cc_buffers (void);
int init_file_buffer(void);
void init_eia608 (struct eia608 *data);
unsigned totalblockswritten_thisfile (void);
int too_many_blocks ();
unsigned encode_line (unsigned char *buffer, unsigned char *text);
// LONG buffered_read (unsigned char *buffer, unsigned int bytes);
void buffered_seek (int offset);
void write_subtitle_file_header (struct s_write *wb);
void write_subtitle_file_footer (struct s_write *wb);
void general_loop(void);
void myth_loop(void);
void raw_loop (void);
void ts_loop (FILE *in_file);
extern void build_parity_table(void);

extern const unsigned char BROADCAST_HEADER[];
extern const unsigned char DVD_HEADER[];
extern const unsigned char lc1[];
extern const unsigned char lc2[];
extern const unsigned char lc3[];
extern const unsigned char lc4[];
extern const unsigned char lc5[];
extern const unsigned char lc6[];
extern unsigned char captions_buffer_1[MAX_CLOSED_CAPTION_DATA_PER_PICTURE*2];
extern unsigned int used_caption_buffer_1;
extern unsigned char captions_buffer_2[MAX_CLOSED_CAPTION_DATA_PER_PICTURE*2];
extern unsigned int used_caption_buffer_2;
extern int last_reported_progress;
extern int buffer_input;
extern int debug_608;
extern unsigned char *subline;
extern int frames_since_last_gop;
extern LONG net_fields;

extern char **spell_lower;
extern char **spell_correct;
extern int spell_words;
extern int spell_capacity;

extern unsigned char encoded_crlf[16]; // We keep it encoded here so we don't have to do it many times
extern unsigned int encoded_crlf_length;
extern unsigned char encoded_br[16];
extern unsigned int encoded_br_length;
extern int write_format; //output_format


