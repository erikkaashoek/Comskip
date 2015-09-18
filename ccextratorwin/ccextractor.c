/* CCExtractor, cfsmp3@gmail.com
Version 0.34
Credits: McPoodle for SCC_RIP (CCExtractor started as
optimized port with HDTV support added)
Scott Larson - Useful posts in several forums
Neuron2 - Documentation
Ken Schultz - Added something to McPoodle's
CCExtract.gpl that helped
John Bell- Samples, code.
License: GPL 2.0
*/
#include "../platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "ccextractor.h"
#include "608.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define false 0
#define true 1

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

extern unsigned char *filebuffer;
extern int bytesinbuffer; // Number of bytes we actually have on buffer

unsigned char captions_buffer_1[MAX_CLOSED_CAPTION_DATA_PER_PICTURE*2];
unsigned int used_caption_buffer_1 = 0;
unsigned char captions_buffer_2[MAX_CLOSED_CAPTION_DATA_PER_PICTURE*2];
unsigned int used_caption_buffer_2 = 0;

// Timing related stuff for HDTV
LONG min_pts, max_pts, last_pts;
int pts_set; //0 = No, 1 = Just received but not an I-Frame yet, 2 = Ready

// Number of CC 2-byte blocks written in this continuous chuck
unsigned c1count, c2count;
// Total IN THIS FILE
unsigned c1count_total, c2count_total;
// Grand total before the current file, used for the .srt timing
unsigned c1global=0, c2global=0;
unsigned pts_big_change;

#define MPEG_CLOCK_FREQ 90000 // This is part of the standard

// These two just for debugging
// Original 5 bytes the MPEG clock came from
unsigned char ptsdata[5];
// Original 5 bytes the previous MPEG clock came from
unsigned char lastptsdata[5];

// Stuff common to both loops
unsigned char *fbuffer = NULL;
LONG past; /* Position in file, if in sync same as ftell()  */
unsigned char *pesheaderbuf = NULL;
LONG inputsize;
int last_reported_progress;
int processed_enough; // If 1, we have enough lines, time, etc.

// Small buffer to help us with the initial sync
unsigned char startbytes[STARTBYTESLENGTH];
unsigned int startbytes_pos;
unsigned int startbytes_avail;

/* Stats */
int stat_numuserheaders;
int stat_dvdccheaders;
int stat_replay5000headers;
int stat_replay4000headers;
int stat_dishheaders;
int stat_hdtv;
unsigned total_frames_count;
int cc_stats[4];
int false_pict_header;

/* GOP-based timing */
struct gop_time_code gop_time, first_gop_time, printed_gop;
int frames_since_last_gop=0;
int gop_rollover=0;

/* Parameters */
int buffer_output = 0; // Buffer output? (doesn't help much)
#ifdef _WIN32
int buffer_input = 1; // In Windows buffering seems to help
#else
int buffer_input = 0; // In linux, not so much.
#endif
int autopad = 1; // Add do-nothing CC blocks to pad where needed?
int gop_pad = 0; // Use GOP instead of PTS for padding
int ff_cleanup = 1; // Verify end of CC block marker?
int ts_mode = 0; // Transport stream mode for current file
int input_bin = 0; // Processing raw files (bin) instead of mpeg
int auto_ts = 2; // 2=autodetect, 0=forced no, 1=forced yes
int debug = 0; // Dump strange things
int fix_padding = 0; // Replace 0000 with 8080 in HDTV (needed for some cards)
int rawmode = 0; // Broadcast or DVD
int extract = 1; // Extract 1st, 2nd or both fields
int cc_channel = 1; // Channel we want to dump in srt mode
int debug_608=0; // Show CC decoder debug?
LONG subs_delay=0; // ms to delay (or advance) subs
struct boundary_time extraction_start, extraction_end; // Segment we actually process
LONG screens_to_process=-1; // How many screenfuls we want?
char *basefilename=NULL; // Input filename without the extension
char **inputfile=NULL; // List of files to process
int direct_rollup=0; // Write roll-up captions directly instead of line by line?
int num_input_files=0; // How many?
int inputfile_capacity=0;
int nofontcolor=0; // 1 = don't put <font color> tags
int next_input_file=0;
int write_format=OF_SRT; // 0=Raw, 1=srt, 2=SMI
int encoding = ENC_LATIN_1;
int auto_myth = 2; // Use myth-tv mpeg code? 0=no, 1=yes, 2=auto
int sentence_cap =0 ; // FIX CASE? = Fix case?
char *sentence_cap_file=NULL; // Extra words file?

// Case arrays
char **spell_lower=NULL;
char **spell_correct=NULL;
int spell_words=0;
int spell_capacity=0;
int spell_builtin_added=0; // so we don't do it twice

struct s_write wbout1, wbout2; // Output structures

/* these are only used by DVD raw mode: */
int loopcount = 1; /* loop 1: 5 elements, loop 2: 8 elements,
                   loop 3: 11 elements, rest: 15 elements */
int datacount = 0; /* counts within loop */

const unsigned char BROADCAST_HEADER[]={0xff, 0xff, 0xff, 0xff};
const unsigned char LITTLE_ENDIAN_BOM[]={0xff, 0xfe};

const unsigned char DVD_HEADER[]={0x00,0x00,0x01,0xb2,0x43,0x43,0x01,0xf8};
const unsigned char lc1[]={0x8a};
const unsigned char lc2[]={0x8f};
const unsigned char lc3[]={0x16,0xfe};
const unsigned char lc4[]={0x1e,0xfe};
const unsigned char lc5[]={0xff};
const unsigned char lc6[]={0xfe};


/* File handles */
// FILE *in, *clean;
FILE *clean;
int in; // descriptor number to input

enum
{
    NTSC_CC_f1         = 0,
    NTSC_CC_f2         = 1,
    DTVCC_PACKET_DATA  = 2,
    DTVCC_PACKET_START = 3,
};

const double framerates_values[16]=
{
	0,
	23.976,
	24.0,
	25.0,
	29.97,
	30.0,
	50.0,
	59.94,
	60.0,
	0,
	0,
	0,
	0,
	0
};

const char *framerates_types[16]=
{
    "00 - forbidden",
    "01 - 23.976",
    "02 - 24",
    "03 - 25",
    "04 - 29.97",
    "05 - 30",
    "06 - 50",
    "07 - 59.94",
    "08 - 60",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved"
};

const char *aspect_ratio_types[16]=
{
    "00 - forbidden",
    "01 - 1:1",
    "02 - 4:3",
    "03 - 16:9",
    "04 - 2.21:1",
    "05 - reserved",
    "06 - reserved",
    "07 - reserved",
    "08 - reserved",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved"
};


const char *pict_types[8]=
{
    "00 - ilegal (0)",
    "01 - I",
    "02 - P",
    "03 - B",
    "04 - ilegal (D)",
    "05 - ilegal (5)",
    "06 - ilegal (6)",
    "07 - ilegal (7)"
};

const char *cc_types[4] =
{
    "NTSC line 21 field 1 closed captions",
    "NTSC line 21 field 2 closed captions",
    "DTVCC Channel Packet Data",
    "DTVCC Channel Packet Start"
};

// Some basic English words, so user-defined doesn't have to
// include the common stuff
const char *spell_builtin[]=
{
	"I", "I'd",	"I've",	"I'd", "I'll",
	"January","February","March","April", // May skipped intentionally
	"June","July","August","September","October","November",
	"December","Monday","Tuesday","Wednesday","Thursday",
	"Friday","Saturday","Sunday","Halloween","United States",
	"Spain","France","Italy","England",
	NULL
};

LONG getfilesize (int in)
{
    LONG current=LSEEK (in, 0, SEEK_CUR);
    LONG length = LSEEK (in,0,SEEK_END);
    LSEEK (in,current,SEEK_SET);
    return length;
}


void header (void)
{
    printf ("CCExtractor v0.34, cfsmp3 at gmail\n");
    printf ("----------------------------------\n");
}

void usage (void)
{
    printf ("Heavily based on McPoodle's tools. Check his page for lots of information\n");
    printf ("on closed captions technical details.\n");
    printf ("(http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/SCC_TOOLS.HTML)\n\n");
    printf ("This tool home page:\n");
    printf ("http://ccextractor.sourceforge.net\n");
    printf ("  Extracts closed captions from MPEG files.\n");
    printf ("    (DVB, .TS, ReplayTV 4000 and 5000, dvr-ms, bttv and Dish Network are known\n");
	printf ("     to work).\n\n");
    printf ("  Syntax:\n");
    printf ("  ccextractor [options] inputfile1 [inputfile2...] [-o outputfilename]\n");
    printf ("               [-o1 outputfilename1] [-o2 outputfilename2]\n\n");
	printf ("File name related options:\n");
    printf ("            inputfile: file(s) to process\n");
    printf ("    -o outputfilename: Use -o parameters to define output filename if you don't\n");
    printf ("                       like the default ones (same as infile plus _1 or _2 when\n");
    printf ("                       needed and .bin or .srt extension).\n");
    printf ("                           -o or -o1 -> Name of the first (maybe only) output\n");
    printf ("                                        file.\n");
    printf ("                           -o2       -> Name of the second output file, when\n");
    printf ("                                        it applies.\n");
    printf ("         -cf filename: Write 'clean' data to a file. Cleans means the ES\n");
    printf ("                       without TS or PES headers.\n\n");
    printf ("You can pass as many input files as you need. They will be processed in order.\n");
    printf ("Output will be one single file (either raw or srt). Use this if you made your\n");
    printf ("recording in several cuts (to skip commercials for example) but you want one\n");
    printf ("subtitle file with contiguous timing.\n\n");
	printf ("Options that affect what will be processed:\n");
    printf ("          -1, -2, -12: Output Field 1 data, Field 2 data, or both\n");
    printf ("                       (DEFAULT is -1)\n");
    printf ("                 -cc2: When in srt/sami mode, process captions in channel 2\n");
    printf ("                       instead channel 1.\n\n");
    printf ("In general, if you want English subtitles you don't need to use these options\n");
	printf ("as they are broadcast in field 1, channel 1. If you want the second language\n");
	printf ("(usually Spanish) you may need to try -2, or -cc2, or both.\n\n");
	printf ("Options that affect how input files will be processed.\n");
    printf ("                  -ts: Force Transport Stream mode.\n");
    printf ("                -nots: Disable Transport Stream mode.\n");
	printf ("                 -bin: Process a raw (bin) closed captions dump instead of a\n");
	printf ("                       MPEG files. Requires that either -srt or -sami is used\n");
    printf ("                       as well.\n");
	printf ("                -myth: Force MythTV code branch.\n");
	printf ("              -nomyth: Disable MythTV code branch.\n");
    printf ("     -fp --fixpadding: Fix padding - some cards (or providers, or whatever)\n");
    printf ("                       seem to send 0000 as CC padding instead of 8080. If you\n");
    printf ("                       get bad timing, this might solve it.\n\n");
	printf ("Usually you only need to use -bin (if you want to produce srt/sami from a\n");
	printf ("dump of previously extracted closed captions). For MPEG files, transport\n");
	printf ("stream mode is autodetected. The MythTV branch is needed for analog captures\n");
	printf ("such as those with bttv cards (Hauppage 250 for example), which is detected\n");
	printf ("as well. You can however force whatever you need in case autodetection\n");
	printf ("doesn't work for you.\n\n");
	printf ("Options that affect what kind of output will be produced:\n");
    printf ("                   -d: Output raw captions in DVD format\n");
    printf ("                       (DEFAULT is broadcast format)\n");
    printf ("                 -srt: Generate .srt instead of .bin.\n");
	printf ("                -sami: Generate .sami instead of .bin.\n");
    printf ("                -utf8: Encode subtitles in UTF-8 instead of Latin-1\n");
    printf ("             -unicode: Encode subtitles in Unicode instead of Latin-1\n");
	printf ("  -nofc --nofontcolor: For .srt/.sami, don't add font color tags.\n\n");
	printf ("    -sc --sentencecap: Sentence capitalization. Use if you hate.\n");
	printf ("                       ALL CAPS in subtitles.\n");
	printf ("  --capfile -caf file: Add the contents of 'file' to the list of words\n");
	printf ("                       that must be capitalized. For example, if file\n");
	printf ("                       is a plain text file that contains\n\n");
	printf ("                       Tony\n");
	printf ("                       Alan\n\n");
	printf ("                       Whenever those words are found they will be written\n");
	printf ("                       exactly as they appear in the file.\n");
	printf ("                       Use one line per word. Lines starting with # are\n");
	printf ("                       considered comments and discarded.\n\n");
	printf ("Options that affect how ccextractor reads and writes (buffering):\n");
    printf ("    -bo -bufferoutput: Buffer writes. Might help a bit with performance.\n");
    printf ("     -bi -bufferinput: Forces input buffering.\n");
    printf (" -nobi -nobufferinput: Disables input buffering.\n\n");
	printf ("Options that affect the built-in closed caption decoder:\n");
	printf ("                 -dru: Direct Roll-Up. When in roll-up mode, write character by\n");
	printf ("                       character instead of line by line. Note that this\n");
	printf ("                       produces (much) larger files.\n");
    printf ("                -noff: Disable FF clean-up. This is extra sanity check when\n");
    printf ("                       processing CC blocks. FF clean-up usually gets rid of\n");
    printf ("                       garbage produced by false CC block, but might cause\n");
    printf ("                       good characters to be missed. Use this option if you\n");
    printf ("                       prefer not to have any character discarded. Note that\n");
	printf ("                       this option is probably no longer needed and will\n");
	printf ("                       be removed soon.\n\n");
	printf ("Options that affect timing:\n");
    printf ("    -noap --noautopad: Disable autopad. By default ccextractor pads closed\n");
    printf ("                       captions data to ensure that there's exactly 29.97 CC\n");
    printf ("                       2-byte blocks per second. Usually this fixes timing\n");
    printf ("                       issues, but you may disable it with this option.\n");
    printf ("                       Note that autopadding only happens in TS mode.\n");
	printf ("         -gp --goppad: Use GOP timing for padding instead of PTS. Use this\n");
	printf ("                       if you need padding on a non-TS file.\n");
	printf ("            -delay ms: For srt/sami, add this number of milliseconds to\n");
	printf ("                       all times. For example, -delay 400 makes subtitles\n");
	printf ("                       appear 400ms late. You can also use negative numbers\n");
	printf ("                       to make subs appear early.\n");
	printf ("Notes on times: -startat and -endat times are used first, then -delay.\n");
	printf ("So if you use -srt -startat 3:00 -endat 5:00 -delay 12000, ccextractor will\n");
	printf ("generate a .srt file, with only data from 3:00 to 5:00 in the input file(s)\n");
	printf ("and then add that (huge) delay, which would make the final file start at\n");
	printf ("5:00 and end at 7:00.\n\n");
	printf ("Options that affect what segment of the input file(s) to process:\n");
	printf ("        -startat time: For .srt/.sami, only write subtitles that start after\n");
	printf ("                       the given time. Time can be seconds, MM:SS or HH:MM:SS.\n");
	printf ("                       For example, -startat 3:00 means 'start writing from\n");
	printf ("                       minute 3.\n");
	printf ("                       This option is ignored in raw mode.\n");
	printf ("          -endat time: Stop processing after the given time (same format as\n");
	printf ("                       -startat). This option is honored in all output\n");
	printf ("                       formats.\n");
	printf ("-scr --screenfuls num: Write 'num' screenfuls and terminate processing.\n\n");
	printf ("Options that affect debug data:\n");
    printf ("               -debug: For HDTV dumps 'interesting' packets.\n");
    printf ("                 -608: Print debug traces from the EIA-608 decoder.\n");
    printf ("                       If you need to submit a bug report, please send\n");
    printf ("                       the output from this option.\n");
    printf ("\n");
}

unsigned totalblockswritten_thisfile (void)
{
    unsigned blocks=(c1count_total+c1count);
    if (blocks==0)
        blocks=(c2count_total+c2count);
    return blocks;
}

void init_write (struct s_write *wb)
{
    wb->fh=NULL;
    wb->filename=NULL;
    wb->buffer=(unsigned char *) malloc (BUFSIZE);
    wb->used=0;
    wb->data608=(struct eia608 *) malloc (sizeof (struct eia608));
    init_eia608 (wb->data608);
}

void writeraw (const unsigned char *data, int length, struct s_write *wb)
{
    if (buffer_output)
    {
        if (data==NULL || wb->used+length>BUFSIZE)
        {
            fwrite (wb->buffer,wb->used,1,wb->fh);
            if (data!=NULL)
                fwrite (data,length,1,wb->fh);
            wb->used=0;
        }
        else
        {
            memcpy (wb->buffer+wb->used, data, length);
            wb->used+=length;
        }
    }
    else
        fwrite (data,length,1,wb->fh);
}

void writedata (const unsigned char *data, int length, struct s_write *wb)
{
	if (extraction_end.set)
	{
		unsigned pg = c1global? c1global:c2global; // Any non-zero global will do
		if ((pg+totalblockswritten_thisfile())>extraction_end.time_in_ccblocks)
		{
			processed_enough=1;
			return;
		}
	}
    if (write_format==OF_RAW)
        writeraw (data,length,wb);
    else
        if (data!=NULL)
			process608 (data,length,wb);
}

void flushbuffer (struct s_write *wb, int closefile)
{
    if (buffer_output)
        writedata (NULL,0,wb);

    if (closefile && wb!=NULL && wb->fh!=NULL)
        fclose (wb->fh);
}

int too_many_blocks ()
{
    if (first_gop_time.inited)
    {
        int mis1=(int) ((gop_time.ccblocks-first_gop_time.ccblocks+
			frames_since_last_gop)-c1count);
        if (mis1<0) // More than we need. Skip this block
            return 1;
    }
    return 0;
}

void printdata (const unsigned char *data1, int length1,
				const unsigned char *data2, int length2)
{
    if (rawmode==0) /* Broadcast */
    {
        if (length1 && extract!=2)
        {
            writedata (data1,length1,&wbout1);
        }
        if (length2 && extract!=1)
        {
            writedata (data2,length2,&wbout2);
        }
    }
    else /* DVD */
    {
        if (datacount==0)
        {
            writedata (DVD_HEADER,sizeof (DVD_HEADER),&wbout1);
            if (loopcount==1)
                writedata (lc1,sizeof (lc1),&wbout1);
            if (loopcount==2)
                writedata (lc2,sizeof (lc2),&wbout1);
            if (loopcount==3)
            {
                writedata (lc3,sizeof (lc3),&wbout1);
                writedata (data2,length2,&wbout1);
            }
            if (loopcount>3)
            {
                writedata (lc4,sizeof (lc4),&wbout1);
                writedata (data2,length2,&wbout1);
            }
        }
        datacount++;
        writedata (lc5,sizeof (lc5), &wbout1);
        writedata (data1,length1,&wbout1);
        if (((loopcount == 1) && (datacount < 5)) || ((loopcount == 2) &&
            (datacount < 8)) || (( loopcount == 3) && (datacount < 11)) ||
            ((loopcount > 3) && (datacount < 15)))
        {
            writedata (lc6,sizeof (lc6), &wbout1);
            writedata (data2,length2,&wbout1);
        }
        else
        {
            if (loopcount==1)
            {
                writedata (lc6,sizeof (lc6), &wbout1);
                writedata (data2,length2,&wbout1);
            }
            loopcount++;
            datacount=0;
        }
    }
}

void dump (unsigned char *start, int l)
{
	int x;
    for (x=0; x<l; x=x+16)
    {
		int j;
        printf ("%03d | ",x);
        for (j=0; j<16; j++)
        {
            if (x+j==34)
            {
                x+=4;
                l+=4;
            }
            if (x+j<l)
                printf ("%02X ",start[x+j]);
            else
                printf ("   ");
        }
        printf (" | ");
        for (j=0; j<16; j++)
        {
            if (x+j<=l && start[x+j]>=' ')
                printf ("%c",start[x+j]);
            else
                printf (" ");
        }
        printf ("\n");
    }
}


void prepare_for_new_file (void)
{
    // Init per file variables
    min_pts=0xFFFFFFFF;
    max_pts=0;
    last_pts=0;
    pts_set = 0;
    inputsize=0;
    last_reported_progress=-1;
    stat_numuserheaders = 0;
    stat_dvdccheaders = 0;
    stat_replay5000headers = 0;
    stat_replay4000headers = 0;
    stat_dishheaders = 0;
    stat_hdtv = 0;
    total_frames_count = 0;
    cc_stats[0]=0; cc_stats[1]=0; cc_stats[2]=0; cc_stats[3]=0;
    false_pict_header=0;
    frames_since_last_gop=0;
    gop_time.inited=0;
    first_gop_time.inited=0;
	gop_rollover=0;
    printed_gop.inited=0;
    c1count =0;c2count=0;
    c1count_total =0;
    c2count_total=0;
    past=0;
    pts_big_change=0;
    startbytes_pos=0;
    startbytes_avail=0;
    net_fields=20;
    init_file_buffer();
}


int parsedelay (char *par)
{
	int sign=0;
	char *c=par;
	while (*c)
	{
		if (*c=='-' || *c=='+')
		{
			if (c!=par) // Sign only at the beginning
				return 1;
			if (*c=='-')
				sign=1;
		}
		else
		{
			if (!isdigit (*c))
				return 1;
			subs_delay=subs_delay*10 + (*c-'0');
		}
		c++;
	}
	if (sign)
		subs_delay=-subs_delay;
	return 0;
}

void init_boundary_time (struct boundary_time *bt)
{
	bt->hh=0;
	bt->mm=0;
	bt->ss=0;
	bt->set=0;
	bt->time_in_ms=0;
	bt->time_in_ccblocks=0;
}

int stringztoms (char *s, struct boundary_time *bt)
{
	unsigned ss=0, mm=0, hh=0;
	int value=-1;
	int colons=0;
	LONG secs;
	char *c=s;
	while (*c)
	{
		if (*c==':')
		{
			if (value==-1) // : at the start, or ::, etc
				return -1;
			colons++;
			if (colons>2) // Max 2, for HH:MM:SS
				return -1;
			hh=mm;
			mm=ss;
			ss=value;
			value=-1;
		}
		else
		{
			if (!isdigit (*c)) // Only : or digits, so error
				return -1;
			if (value==-1)
				value=*c-'0';
			else
				value=value*10+*c-'0';
		}
		c++;
	}
	hh=mm;
	mm=ss;
	ss=value;
	if (mm>59 || ss>59)
		return -1;
	bt->set=1;
	bt->hh=hh;
	bt->mm=mm;
	bt->ss=ss;
	secs =(hh*3600+mm*60+ss);
	bt->time_in_ms=secs*1000;
	bt->time_in_ccblocks=(LONG) (secs*29.97);
	return 0;
}

int add_word (const char *word)
{
	size_t len;
	size_t i;
	char *new_lower;
	char *new_correct;
	if (spell_words==spell_capacity)
	{
		// Time to grow
		spell_capacity+=50;
		spell_lower=(char **) realloc (spell_lower, sizeof (char *) *
			spell_capacity);
		spell_correct=(char **) realloc (spell_correct, sizeof (char *) *
			spell_capacity);
	}
	len =strlen (word);
	new_lower = (char *) malloc (len+1);
	new_correct = (char *) malloc (len+1);
	if (spell_lower==NULL || spell_correct==NULL ||
		new_lower==NULL || new_correct==NULL)
	{
		printf ("\rNot enough memory.\n");
		return -1;
	}
	strcpy (new_correct, word);
	for (i=0; i<len; i++)
	{
		char c;
		c=new_correct[i];
		c=tolower (c); // TO-DO: Add Spanish characters
		new_lower[i]=c;
	}
	new_lower[len]=0;
	spell_lower[spell_words]=new_lower;
	spell_correct[spell_words]=new_correct;
	spell_words++;
	return 0;
}

int add_built_in_words()
{
	if (!spell_builtin_added)
	{
		int i=0;
		while (spell_builtin[i]!=NULL)
		{
			if (add_word(spell_builtin[i]))
				return -1;
			i++;
		}
		spell_builtin_added=1;
	}
	return 0;
}


int process_cap_file (char *filename)
{
	FILE *fi = fopen (filename,"rt");
	int num=0;
	char line[35]; // For screen width (32)+CRLF+0
	if (fi==NULL)
	{
		printf ("\rUnable to open capitalization file: %s\n", filename);
		return -1;
	}
	while (fgets (line,35,fi))
	{
		char *c;
		num++;
		if (line[0]=='#') // Comment
			continue;
		c =line+strlen (line)-1;
		while (c>=line && (*c==0xd || *c==0xa))
		{
			*c=0;
			c--;
		}
		// printf ("%d - %s\n", num,line);
		if (strlen (line)>32)
		{
			printf ("Word in line %d too long, max = 32 characters.\n",num);
			fclose (fi);
			return -1;
		}
		if (strlen (line)>0)
		{
			if (add_word (line))
				return -1;
		}
	}
	fclose (fi);
	return 0;
}
#ifdef undef
int main(int argc, char *argv[])
#else

void CEW_reinit()
{
	if (wbout1.fh)
		fclose(wbout1.fh);
	if (wbout1.filename != NULL && strlen(wbout1.filename) > 0 )
		wbout1.fh=FOPEN (wbout1.filename, "wb");
	init_boundary_time (&extraction_start);
	init_boundary_time (&extraction_end);

	wbout1.used=0;
	if (wbout1.data608)
		init_eia608 (wbout1.data608);

	prepare_for_new_file();
	if (wbout1.fh) {
		if (write_format==OF_RAW)
            writeraw (BROADCAST_HEADER,sizeof (BROADCAST_HEADER),&wbout1);
        else
        {
            if (encoding==ENC_UNICODE) // Write BOM
                writeraw (LITTLE_ENDIAN_BOM, sizeof (LITTLE_ENDIAN_BOM), &wbout1);
           write_subtitle_file_header (&wbout1);
        }
	}
}


int CEW_init(int argc, char *argv[])
#endif
{
    char *output_filename=NULL;
    char *clean_filename=NULL;
    char *c;
    char *extension;
	int i;
    time_t start;

//    header();

    // Prepare write structures
    init_write(&wbout1);
    init_write(&wbout2);

	// Prepare time structures
	init_boundary_time (&extraction_start);
	init_boundary_time (&extraction_end);

    // Parse parameters
    for (i=1; i<argc; i++)
    {
        if (argv[i][0]!='-')
        {
            if (inputfile_capacity<=num_input_files)
            {
                inputfile_capacity+=10;
                inputfile=(char **) realloc (inputfile,sizeof (char *) * inputfile_capacity);
            }
            inputfile[num_input_files]=argv[i];
            num_input_files++;
        }
        if (strcmp (argv[i],"-bo")==0 ||
            strcmp (argv[i],"--bufferoutput")==0)
            buffer_output = 1;
        if (strcmp (argv[i],"-bi")==0 ||
            strcmp (argv[i],"--bufferinput")==0)
            buffer_input = 1;
        if (strcmp (argv[i],"-nobi")==0 ||
            strcmp (argv[i],"--nobufferinput")==0)
            buffer_input = 0;
        if (strcmp (argv[i],"-d")==0)
            rawmode = 1;
        if (strcmp (argv[i],"-dru")==0)
            direct_rollup = 1;
        if (strcmp (argv[i],"-nots")==0)
            auto_ts = 0;
        if (strcmp (argv[i],"-nofc")==0 ||
            strcmp (argv[i],"--nofontcolor")==0)
			nofontcolor=1;
        if (strcmp (argv[i],"-ts")==0)
            auto_ts = 1;
        if (strcmp (argv[i],"-12")==0)
            extract = 12;
        if (strcmp (argv[i],"-noff")==0)
            ff_cleanup = 0;
        if (strcmp (argv[i],"-fp")==0 ||
            strcmp (argv[i],"--fixpadding")==0)
            fix_padding = 1;
        if (strcmp (argv[i],"-noap")==0 ||
            strcmp (argv[i],"--noautopad")==0)
            autopad = 0;
        if (strcmp (argv[i],"-gp")==0 ||
            strcmp (argv[i],"--goppad")==0)
			gop_pad = 1;
        if (strcmp (argv[i],"-debug")==0)
            debug = 1;
		if (strcmp (argv[i],"--sentencecap")==0 ||
			strcmp (argv[i],"-sc")==0)
		{
			if (add_built_in_words())
				exit (-1);
			sentence_cap=1;
		}
		if ((strcmp (argv[i],"--capfile")==0 ||
			strcmp (argv[i],"-caf")==0)
			&& i<argc-1)
		{
			if (add_built_in_words())
				exit (-1);
			if (process_cap_file (argv[i+1])!=0)
				exit (-1);
			sentence_cap=1;
			sentence_cap_file=argv[i+1];
			i++;
		}
		if ((strcmp (argv[i],"--defaultcolor")==0 ||
			strcmp (argv[i],"-dc")==0)
			&& i<argc-1)
		{
			if (strlen (argv[i+1])!=7 || argv[i+1][0]!='#')
			{
				printf ("\r--defaultcolor expects a 7 character parameter that starts with #\n");
				exit (-1);
			}
			strcpy ((char *) usercolor_rgb,argv[i+1]);
			default_color=COL_USERDEFINED;
			i++;
		}
		if (strcmp (argv[i],"-delay")==0 && i<argc-1)
		{
			if (parsedelay (argv[i+1]))
			{
				printf ("\r-delay only accept integers (such as -300 or 300)\n");
				exit (-1);
			}
			i++;
		}
		if ((strcmp (argv[i],"-scr")==0 ||
			strcmp (argv[i],"--screenfuls")==0) && i<argc-1)
		{
			screens_to_process=atoi (argv[i+1]);
			if (screens_to_process<0)
			{
				printf ("\r--screenfuls only accepts positive integers.\n");
				exit (-1);
			}
			i++;
		}
		if (strcmp (argv[i],"-startat")==0 && i<argc-1)
		{
			if (stringztoms (argv[i+1],&extraction_start)==-1)
			{
				printf ("\r-startat only accepts SS, MM:SS or HH:MM:SS\n");
				exit (-1);
			}
			i++;
		}
		if (strcmp (argv[i],"-endat")==0 && i<argc-1)
		{
			if (stringztoms (argv[i+1],&extraction_end)==-1)
			{
				printf ("\r-endat only accepts SS, MM:SS or HH:MM:SS\n");
				exit (-1);
			}
			i++;
		}
        if (strcmp (argv[i],"-1")==0)
            extract = 1;
        if (strcmp (argv[i],"-2")==0)
            extract = 2;
        if (strcmp (argv[i],"-srt")==0)
            write_format=OF_SRT;
        if (strcmp (argv[i],"-sami")==0)
            write_format=OF_SAMI;
		if (strcmp (argv[i],"-bin")==0)
			input_bin=1;
        if (strcmp (argv[i],"-cc2")==0 || strcmp (argv[i],"-CC2")==0)
            cc_channel=2;
        if (strcmp (argv[i],"-608")==0)
            debug_608 = 1;
        if (strstr (argv[i],"-unicode")!=NULL)
            encoding=ENC_UNICODE;
        if (strstr (argv[i],"-utf8")!=NULL)
            encoding=ENC_UTF_8;
        if (strstr (argv[i],"-myth")!=NULL)
            auto_myth=1;
        if (strstr (argv[i],"-nomyth")!=NULL)
            auto_myth=0;
        if (strcmp (argv[i],"-o")==0 && i<argc-1)
        {
            output_filename=argv[i+1];
            i++;
        }
        if (strcmp (argv[i],"-cf")==0 && i<argc-1)
        {
            clean_filename=argv[i+1];
            i++;
        }
        if (strcmp (argv[i],"-o1")==0 && i<argc-1)
        {
            wbout1.filename=argv[i+1];
            i++;
        }
        if (strcmp (argv[i],"-o2")==0 && i<argc-1)
        {
            wbout2.filename=argv[i+1];
            i++;
        }
    }
    if (num_input_files==0)
    {
        usage ();
        exit (2);
    }
    if (output_filename!=NULL)
    {
        if (extract==2)
            wbout2.filename=output_filename;
        else
            wbout1.filename=output_filename;
    }

    switch (write_format)
    {
        case OF_RAW:
            extension = ".bin";
            break;
        case OF_SRT:
            extension = ".srt";
            break;
        case OF_SAMI:
            extension = ".smi";
            break;
        default:
            printf ("write_format doesn't have any legal value, this is a bug.\n");
            exit(500);
    }
#ifdef DONT_DISPLAY
    // Display parsed parameters
    printf ("Input: ");
    for (i=0;i<num_input_files;i++)
        printf ("%s%s",inputfile[i],i==(num_input_files-1)?"":",");
    printf ("\n");
    printf ("[Raw Mode: %s] ", rawmode ? "DVD" : "Broadcast");
    printf ("[Extract: %d] ", extract);
    printf ("[TS mode: ");
    switch (auto_ts)
    {
        case 0:
            printf ("Disabled");
            break;
        case 1:
            printf ("Forced");
            break;
        case 2:
            printf ("Auto");
            break;
    }
    printf ("] ");
    printf ("[Use MythTV code: ");
	switch (auto_myth)
    {
        case 0:
            printf ("Disabled");
            break;
        case 1:
            printf ("Forced");
            break;
        case 2:
            printf ("Auto");
            break;
    }
    printf ("]\n");

    printf ("[Debug: %s] ", debug ? "Yes": "No");
    printf ("[Buffer output: %s] ", buffer_output ? "Yes": "No");
    printf ("[Buffer input: %s]\n", buffer_input ? "Yes": "No");
    printf ("[Autopad: %s] ", autopad ? "Yes": "No");
	printf ("[GOP pad: %s] ", gop_pad ? "Yes": "No");
    printf ("[Print CC decoder traces: %s]\n", debug_608 ? "Yes": "No");
    printf ("[Target format: %s] ",extension);
    printf ("[Encoding: [");
    switch (encoding)
    {
        case ENC_UNICODE:
            printf ("Unicode");
            break;
        case ENC_UTF_8:
            printf ("UTF-8");
            break;
        case ENC_LATIN_1:
            printf ("Latin-1");
            break;
    }
	printf ("] ");
	printf ("[Delay: %ld] ",subs_delay);
	printf ("[Input type: %s]\n",input_bin?".bin":"MPEG");
	printf ("[Add font color data: %s] ", nofontcolor? "No" : "Yes");
	printf ("[Convert case: ");
	if (sentence_cap_file!=NULL)
		printf ("Yes, using %s", sentence_cap_file);
	else
	{
		printf ("%s",sentence_cap?"Yes, but only built-in words":"No");
	}
	printf ("]\n");
	printf ("[Extraction start time: ");
	if (extraction_start.set==0)
		printf ("not set (from start)");
	else
		printf ("%02d:%02d:%02d", extraction_start.hh,
		     extraction_start.mm,extraction_start.ss);
	printf ("]\n");
	printf ("[Extraction end time: ");
	if (extraction_end.set==0)
		printf ("not set (to end)");
	else
		printf ("%02d:%02d:%02d", extraction_end.hh,extraction_end.mm,
		extraction_end.ss);
	printf ("]\n");
#endif
	if (input_bin && write_format==OF_RAW)
	{
		printf ("-bin can only be used if the output is a subtitle file.\n");
		printf ("If you want to produce a raw closed captions dump from\n");
		printf ("a raw closed captions dump just copy the file.\n");
		exit(-5);
	}
    fbuffer = (unsigned char *) malloc (BUFSIZE);
    subline = (unsigned char *) malloc (SUBLINESIZE);
    pesheaderbuf = (unsigned char *) malloc (188); // Never larger anyway

    basefilename = (char *) malloc (strlen (inputfile[0])+1);

    if (wbout1.filename==NULL)
    {
        wbout1.filename = (char *) malloc (strlen (inputfile[0])+3+strlen (extension));
        wbout1.filename[0]=0;
    }
    if (wbout2.filename==NULL)
    {
        wbout2.filename = (char *) malloc (strlen (inputfile[0])+3+strlen (extension));
        wbout2.filename[0]=0;
    }
    if (fbuffer == NULL || basefilename == NULL || pesheaderbuf==NULL ||
        wbout1.filename == NULL || wbout2.filename == NULL ||
        subline==NULL || init_file_buffer())
    {
        printf ("Not enough memory\n");
        exit (1);
    }

    strcpy (basefilename, inputfile[0]);
    for (c=basefilename+strlen (basefilename)-1; c>basefilename &&
        *c!='.'; c--) {;} // Get last .
    if (*c=='.')
        *c=0;
    /* # DVD format uses one raw file for both fields, while Broadcast requires 2 */
    if (rawmode==1)
    {
        if (wbout1.filename[0]==0)
        {
            strcpy (wbout1.filename,basefilename);
            strcat (wbout1.filename,".bin");
        }
        printf ("Creating %s\n", wbout1.filename);
        wbout1.fh=FOPEN (wbout1.filename, "wb");
        if (wbout1.fh==NULL)
        {
            printf ("Failed\n");
            exit (3);
        }
    }
    else
    {
        if (extract!=2)
        {
            if (wbout1.filename[0]==0)
            {
                strcpy (wbout1.filename,basefilename);
//                strcat (wbout1.filename,"_1");
                strcat (wbout1.filename,(const char *) extension);
            }
            printf ("Creating %s\n", wbout1.filename);
            wbout1.fh=FOPEN (wbout1.filename, "wb");
            if (wbout1.fh==NULL)
            {
                printf ("Failed\n");
                exit (3);
            }
            if (write_format==OF_RAW)
                writeraw (BROADCAST_HEADER,sizeof (BROADCAST_HEADER),&wbout1);
            else
            {
                if (encoding==ENC_UNICODE) // Write BOM
                    writeraw (LITTLE_ENDIAN_BOM, sizeof (LITTLE_ENDIAN_BOM), &wbout1);
                write_subtitle_file_header (&wbout1);
            }

        }
        if (extract == 12)
            printf (" and \n");
        if (extract!=1)
        {
            if (wbout2.filename[0]==0)
            {
                strcpy (wbout2.filename,basefilename);
                strcat (wbout2.filename,"_2");
                strcat (wbout2.filename,(const char *) extension);
            }
            printf ("Creating %s\n", wbout2.filename);
            wbout2.fh=FOPEN (wbout2.filename, "wb");
            if (wbout2.fh==NULL)
            {
                printf ("Failed\n");
                exit (3);
            }
            if (write_format==OF_RAW)
                writeraw (BROADCAST_HEADER,sizeof (BROADCAST_HEADER),&wbout2);
            else
            {
                if (encoding==ENC_UNICODE) // Write BOM
                    writeraw (LITTLE_ENDIAN_BOM, sizeof (LITTLE_ENDIAN_BOM), &wbout1);
                write_subtitle_file_header (&wbout2);
            }
        }
    }
    clean = NULL;
    if (clean_filename!=NULL)
    {
        if ((clean = fopen (clean_filename,"wb"))==NULL)
        {
            printf ("Unable to open clean file: %s\n",clean_filename);
            exit (-4);
        }
    }
    encoded_crlf_length = encode_line (encoded_crlf,(unsigned char *) "\r\n");
    encoded_br_length = encode_line (encoded_br, (unsigned char *) "<br>");
	build_parity_table();

    time(&start);

#ifdef undef


    // in = FOPEN (inputfile,"rb");
	processed_enough=0;
    for (i=0;i<num_input_files && !processed_enough;i++)
    {
		unsigned secs;
        printf ("\r-----------------------------------------------------------------\n");
        printf ("\rOpening file: %s\n", inputfile[i]);
#ifdef _WIN32
        in=OPEN (inputfile[i],O_RDONLY | O_BINARY);
#else
        in=OPEN (inputfile[i],O_RDONLY);
#endif
        if (in == -1)
        {
            printf ("\rWarning: Unable to open input file [%s]\n", inputfile[i]);
            continue;
        }
        prepare_for_new_file();
        next_input_file++;

        inputsize = getfilesize (in);
		if (!input_bin)
		{
			switch (auto_ts)
			{
				case 0: // Forced no
	                ts_mode=0;
					break;
				case 1: // Forced yes
					ts_mode=1;
					break;
				case 2: // Autodetect
					ts_mode=0; // Not found
					startbytes_avail=read (in,startbytes,STARTBYTESLENGTH);
					if (startbytes_avail==STARTBYTESLENGTH) // Otherwise, assume no TS
					{
						unsigned i;
						for (i=0; i<188;i++)
						{
							if (startbytes[i]==0x47 && startbytes[i+188]==0x47 &&
	                            startbytes[i+188*2]==0x47 && startbytes[i+188*3]==0x47)
							{
								// Four sync bytes, that's good enough
								startbytes_pos=i;
								ts_mode=1;
								break;
							}
						}
					}
					else
					{
						startbytes_pos=0;
						ts_mode=0;
					}
					if (ts_mode)
	                    printf ("\rFile seems to be a transport stream, enabling TS mode\n");
                    memcpy (filebuffer, startbytes, STARTBYTESLENGTH);
                    bytesinbuffer=STARTBYTESLENGTH;
					break;
			}

		/* -----------------------------------------------------------------
		MAIN LOOP
		----------------------------------------------------------------- */
            switch (auto_myth)
            {
                case 0:
                    general_loop();
                    break;
                case 1:
//                    myth_loop();
                    break;
                case 2:
                    if (ts_mode)
                        general_loop();
                    else
					{
						int vbi_blocks=0;
						// VBI data? if yes, use myth loop
						if (startbytes_avail==STARTBYTESLENGTH)
						{
							unsigned int i;
							unsigned int c=0;
							unsigned char uc[3];
							memcpy (uc,startbytes,3);
							for (i=3;i<startbytes_avail;i++)
							{
								if ((uc[0]=='t') && (uc[1]=='v') && (uc[2] == '0') ||
									(uc[0]=='V') && (uc[1]=='V') && (uc[2] == '0'))
									vbi_blocks++;
								uc[0]=uc[1];
								uc[1]=uc[2];
								uc[2]=startbytes[i];
							}
						}
//						if (vbi_blocks>10) // Too much coincidence
//							myth_loop();
//						else
							general_loop();
					}
                    break;
            }

			if (stat_hdtv)
			{
				printf ("\rCC type 0: %d (%s)\n", cc_stats[0], cc_types[0]);
				printf ("CC type 1: %d (%s)\n", cc_stats[1], cc_types[1]);
				printf ("CC type 2: %d (%s)\n", cc_stats[2], cc_types[2]);
				printf ("CC type 3: %d (%s)\n", cc_stats[3], cc_types[3]);
			}
			if (ts_mode)
			{
				int total_secs;
				printf ("Min PTS: %u\n",min_pts);
				printf ("Max PTS: %u\n",max_pts);
				total_secs = (int) ((max_pts-min_pts)/MPEG_CLOCK_FREQ);
				printf ("Total frames: %u\n", total_frames_count);
				if (pts_big_change)
	                printf ("Reference clock was reset at some point, unable to provide length\n");
				else
					printf ("Length according to PTS (MM:SS:100s): %u (%u:%02u:%03u)\n", total_secs,
					total_secs/60, total_secs%60, (max_pts-min_pts)%900);
				printf ("Total 2-byte blocks for field 1: %u\n",c1count+c1count_total);
				printf ("Total 2-byte blocks for field 2: %u\n",c2count+c2count_total);
			}
			if (gop_time.inited && first_gop_time.inited)
			{
				printf ("\rInitial GOP time: %02u:%02u:%02u:%02u\n",first_gop_time.time_code_hours,
				first_gop_time.time_code_minutes,
				first_gop_time.time_code_seconds,first_gop_time.time_code_pictures);

				printf ("Final GOP time: %02u:%02u:%02u:%02u\n",gop_time.time_code_hours,
					gop_time.time_code_minutes,
				gop_time.time_code_seconds,gop_time.time_code_pictures);
			}
		}
		else
		{
			raw_loop();
		}
		close (in);
        secs = (int) (totalblockswritten_thisfile()/29.97);

        printf ("\rTotal length according to CC blocks (MM:SS): %02u:%02u\n",secs/60,secs%60);
        printf ("Number of likely false picture headers (discarded): %d\n",false_pict_header);

        c1global+=c1count+c1count_total;
        c2global+=c2count+c2count_total;
    }
    if (clean!=NULL)
        fclose (clean);
    flushbuffer (&wbout1,false);
    flushbuffer (&wbout2,false);
    if (wbout1.fh!=NULL)
        write_subtitle_file_footer (&wbout1);
    if (wbout2.fh!=NULL)
        write_subtitle_file_footer (&wbout2);
    flushbuffer (&wbout1,true);
    flushbuffer (&wbout2,true);
    time (&final);
    printf ("\rDone, processing time = %d seconds\n", (int) (final-start));
	if (processed_enough)
	{
		printf ("\rNote: Processing was cancelled before all data was processed because\n");
		printf ("\rone or more user-defined limits were reached.\n");
	}
    printf ("This is alpha software. Report issues to cfsmp3 at gmail...\n");
#endif
    return 0;
}
