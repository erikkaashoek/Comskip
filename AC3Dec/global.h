/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <ddraw.h>
#include <vfw.h>
#include <winreg.h>
#include <direct.h>
#include <io.h>
#include "resource.h"
#endif
#include <fcntl.h>

#include "mpalib.h"

#include <sys/stat.h>

typedef struct
{
	MLVERSION mlVersion;
	MLINIT mlInit;
	MLEXIT mlExit;
	MLDECODE mlDecode;

	ML_VERSION Version;

#ifdef _WIN32
	HINSTANCE hDLL;
#endif

	struct mpstr mp;
	int nRet;
	int nSize;
	DWORD dwFree;

	BYTE *byInBuffer;
	WORD wFrameSize;

} MPGStruct;

MPGStruct MPGd;

BYTE byMPGLibInit;

#define szAppName "Test"


/* code definition */
#define PICTURE_START_CODE			0x100
#define SLICE_START_CODE_MIN		0x101
#define SLICE_START_CODE_MAX		0x1AF
#define USER_DATA_START_CODE		0x1B2
#define SEQUENCE_HEADER_CODE		0x1B3
#define EXTENSION_START_CODE		0x1B5
#define SEQUENCE_END_CODE			0x1B7
#define GROUP_START_CODE			0x1B8

#define SYSTEM_END_CODE				0x1B9
#define PACK_START_CODE				0x1BA
#define SYSTEM_START_CODE			0x1BB

#define VIDEO_ELEMENTARY_STREAM		0x1E0

#define PRIVATE_STREAM_1			0x1BD
#define PRIVATE_STREAM_2			0x1BF
#define AUDIO_ELEMENTARY_STREAM_0	0x1C0
#define AUDIO_ELEMENTARY_STREAM_1	0x1C1
#define AUDIO_ELEMENTARY_STREAM_2	0x1C2
#define AUDIO_ELEMENTARY_STREAM_3	0x1C3
#define AUDIO_ELEMENTARY_STREAM_4	0x1C4
#define AUDIO_ELEMENTARY_STREAM_5	0x1C5
#define AUDIO_ELEMENTARY_STREAM_6	0x1C6
#define AUDIO_ELEMENTARY_STREAM_7	0x1C7

#define SUB_SUB						0x20
#define SUB_AC3						0x80
#define SUB_DTS						0x88
#define SUB_PCM						0xA0

/* extension start code IDs */
#define SEQUENCE_EXTENSION_ID					1
#define SEQUENCE_DISPLAY_EXTENSION_ID			2
#define QUANT_MATRIX_EXTENSION_ID				3
#define COPYRIGHT_EXTENSION_ID					4
#define PICTURE_DISPLAY_EXTENSION_ID			7
#define PICTURE_CODING_EXTENSION_ID				8

#define ZIG_ZAG									0
#define MB_WEIGHT								32
#define MB_CLASS4								64

#define I_TYPE			1
#define P_TYPE			2
#define B_TYPE			3

#define MACROBLOCK_INTRA				1
#define MACROBLOCK_PATTERN				2
#define MACROBLOCK_MOTION_BACKWARD		4
#define MACROBLOCK_MOTION_FORWARD		8
#define MACROBLOCK_QUANT				16

#define TOP_FIELD		1
#define BOTTOM_FIELD	2
#define FRAME_PICTURE	3

#define MC_FIELD		1
#define MC_FRAME		2
#define MC_16X8			2
#define MC_DMV			3

#define MV_FIELD		0
#define MV_FRAME		1

#define CHROMA420		1
#define CHROMA422		2
#define CHROMA444		3

#define BUFFER_SIZE				10240
//2048
//10240
#define MAX_FILE_NUMBER			256

#define STORE_RGB24		1
#define STORE_YUY2		2

#define IDCT_MMX		1
#define IDCT_SSEMMX		2
#define	IDCT_FPU		3
#define IDCT_REF		4

#define LOCATE_INIT			0
#define LOCATE_FORWARD		1
#define LOCATE_BACKWARD		-1
#define LOCATE_FASTFORWARD	5
#define LOCATE_FASTBACKWARD	-5
#define LOCATE_SCROLL		2
#define LOCATE_RIP			3
#define LOCATE_PLAY			4

#define CHANNEL				8

#define TRACK_NONE		255
#define TRACK_1			0
#define TRACK_2			1
#define TRACK_3			2
#define TRACK_4			3
#define TRACK_5			4
#define TRACK_6			5
#define TRACK_7			6
#define TRACK_8			7

#define FORMAT_AUTO			0
#define FORMAT_AC3			1
#define FORMAT_MPA			2
#define FORMAT_LPCM			3
#define FORMAT_DTS			4

#define AUDIO_DEMUXALL		0
#define AUDIO_DEMUXONE		1
#define AUDIO_DECODE		2

#define DRC_NONE		0
#define DRC_LIGHT		1
#define DRC_NORMAL		2
#define DRC_HEAVY		3

#define FO_NONE			0
#define FO_FILM			1
#define FO_SWAP			2

#define SRC_NONE		0
#define SRC_LOW			1
#define SRC_MID			2
#define SRC_HIGH		3
#define SRC_UHIGH		4

#define TRACK_PITCH		500
#define MIN_WIDTH		288
#define MIN_HEIGHT		128

#define CRITICAL_ERROR_LEVEL	50

typedef struct {
	int			type;
	int			file;
	__int64		lba;
	int			pf;
	int			trf;
}	D2VData;
D2VData d2v_backward, d2v_forward, d2v_current;

int CH[CHANNEL];
short Sound_Max;

typedef struct {
//	FILE					*file;
	int						rip;
	unsigned int			mode;
	unsigned int			rate;
}	AC3Stream;
AC3Stream ac3[CHANNEL];

typedef struct {
//	FILE					*file;
	int						rip;
	int						delay;
}	MPAStream;
MPAStream mpa[CHANNEL];

struct PCMStream {
//	FILE					*file;
	char					filename[MAX_PATH];
	int						rip;
	int						size;
	int						delay;
}	pcm;

struct PROCESS {
	__int64					length[MAX_FILE_NUMBER];
	__int64					total;
	__int64					run;
	__int64					start;
	__int64					end;
	int						trackleft;
	int						trackright;
	int						locate;
	int						startfile;
	__int64					startloc;
	int						endfile;
	__int64					endloc;
	int						file;
	__int64					lba;
	int						leftfile;
	__int64					leftlba;
	int						rightfile;
	__int64					rightlba;
	unsigned int			op;
	unsigned int			mi;
	unsigned int			ed;
	unsigned int			elapsed;
	unsigned int			remain;
	float					percent;
}	process;

struct CPU {
	BOOL					mmx;
	BOOL					_3dnow;
	BOOL					ssemmx;
	BOOL					ssefpu;
}	cpu;

/* decoder operation control flags */
int AVI_Flag;
int Check_Flag;
int D2V_Flag;
int DDOverlay_Flag;
int Display_Flag;
int Fault_Flag;
int File_Flag;
int File_Limit;
int FO_Flag;
int iDCT_Flag;
int Pause_Flag;
int Rip_Flag;
int Scale_Flag;
int Statistics_Flag;
int Stop_Flag;
int Store_Flag;
int SystemStream_Flag;

int Luminance_Flag;
int ClipResize_Flag;

unsigned int Track_Flag;
int Format_Flag;
int AC3_Flag;
int MPA_Flag;
int DRC_Flag;
int DSDown_Flag;
int Decision_Flag;
int SRC_Flag;
int Normalization_Flag;
int Norm_Ratio;
double PreScale_Ratio;

BYTE byPlay,byPlayInit;

int PlugIn_Flag;
int KeyOp_Flag;
int lfsr0, lfsr1;

#define		KEY_OFF		0
#define		KEY_INPUT	1
#define		KEY_OP		2

#ifdef _WIN32
typedef __int64 (WINAPI *pfnKeyOp) (int, char*[], HWND);
pfnKeyOp KeyOp;
typedef void (WINAPI *pfnBufferOp) (unsigned char*, int, int);
pfnBufferOp BufferOp;

/* DirectDraw & GDI resources */
LPDIRECTDRAW lpDD;
LPDIRECTDRAW2 lpDD2;
LPDIRECTDRAWSURFACE lpPrimary, lpOverlay;
DDCAPS halcaps;
DDSURFACEDESC ddsd;
DDOVERLAYFX ddofx;
RECT wrect, crect, orect, prect;
HMENU hMenu; HDC hDC;

/* Global Value */
FILE *D2VFile;
HWND hWnd, hDlg, hTrack, hLeftButton, hLeftArrow, hRightArrow, hRightButton;
char szInput[_MAX_PATH], szOutput[_MAX_PATH], szBuffer[_MAX_PATH];
#endif

unsigned char *backward_reference_frame[3], *forward_reference_frame[3];
unsigned char *auxframe[3], *current_frame[3];
unsigned char *u422, *v422, *u444, *v444, *rgb24, *yuy2, *lum;
__int64 RGB_Scale, RGB_Offset, RGB_CRV, RGB_CBU, RGB_CGX;
int LumOffset, LumGain;

int Edge_Width, Edge_Height;
unsigned short VOB_ID, CELL_ID;
int VideoPTS, AudioPTS;
unsigned int Frame_Number;

int Coded_Picture_Width, Coded_Picture_Height, Chroma_Width, Chroma_Height;
int block_count, Second_Field;
int horizontal_size, vertical_size, mb_width, mb_height;

float frame_rate, Frame_Rate;
int Video_Type, FILM_Purity, NTSC_Purity, Bitrate_Meter;

int Clip_Top, Clip_Bottom, Clip_Left, Clip_Right, Squeeze_Width, Squeeze_Height;

int Infile[MAX_FILE_NUMBER];
char *Infilename[MAX_FILE_NUMBER];

int intra_quantizer_matrix[64];
int non_intra_quantizer_matrix[64];
int chroma_intra_quantizer_matrix[64];
int chroma_non_intra_quantizer_matrix[64];

int load_intra_quantizer_matrix;
int load_non_intra_quantizer_matrix;
int load_chroma_intra_quantizer_matrix;
int load_chroma_non_intra_quantizer_matrix;

int q_scale_type;
int alternate_scan;
int quantizer_scale;

short *block[8], *p_block[8];
void *fTempArray, *p_fTempArray;

/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_header() */
int frame_rate_code;
int aspect_ratio_information;

/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
int progressive_sequence;
int chroma_format;

/* ISO/IEC 13818-2 section 6.2.3: picture_header() */
int picture_coding_type;
int temporal_reference;

/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
int f_code[2][2];
int picture_structure;
int frame_pred_frame_dct;
int progressive_frame;
int concealment_motion_vectors;
int intra_dc_precision;
int top_field_first;
int repeat_first_field;
int intra_vlc_format;

/* gethdr.c */
int Get_Hdr(void);
void sequence_header(void);
int slice_header(void);

/* getpic.c */
void Decode_Picture(void);

/* gui.cpp */
void ThreadKill(void);
void CheckDirectDraw(void);
void ResizeWindow(int width, int height);

/* idct */
#ifndef _WIN32
#ifdef __POWERPC__
#define __fastcall
#else
#define __fastcall __attribute__((fastcall))
#endif
#endif

extern void __fastcall MMX_IDCT(short *block);
extern void __fastcall SSEMMX_IDCT(short *block);
void Initialize_FPU_IDCT(void);
void FPU_IDCT(short *block);
void Initialize_REF_IDCT(void);
void REF_IDCT(short *block);

/* motion.c */
void motion_vectors(int PMV[2][2][2], int dmvector[2], int motion_vertical_field_select[2][2], 
	int s, int motion_vector_count, int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale);
void Dual_Prime_Arithmetic(int DMV[][2], int *dmvector, int mvx, int mvy);

/* mpeg2dec.c */
#ifdef _WIN32
DWORD WINAPI MPEG2Dec(LPVOID n);
#endif

/* norm.c */
void Normalize(FILE *WaveIn, int WaveInPos, char *filename, FILE *WaveOut, int WaveOutPos, int size);

/* store.c */
void Write_Frame(unsigned char *src[], D2VData d2v, DWORD frame);
void RenderYUY2(void);
void RenderRGB24(void);
int DetectVideoType(int frame, int rff);

/* wavefs44.c */
//void InitialSRC(void);
//void Wavefs44(FILE *file, int size, unsigned char *buffer);
//void EndSRC(FILE *file);
//void Wavefs44File(int delay, int now, int total);
//void EndWAV(FILE *file, int size);
//void DownWAV(FILE *file);
//BOOL CheckWAV(void);

unsigned char WAVHeader[44]
#ifdef GLOBAL
=
{
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,
	0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
	0x80, 0xbb, 0x00, 0x00, 0x00, 0xee, 0x02, 0x00,
	0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61,
	0x00, 0x00, 0x00, 0x00
}
#endif
;

char *AC3Mode[8]
#ifdef GLOBAL
=
{
	"1+1", "1/0", "2/0", "3/0", "2/1", "3/1", "2/2", "3/2"
}
#endif
;

int AC3Rate[32]
#ifdef GLOBAL
=
{
	32, 40, 48, 56, 64, 80, 96, 112, 128, 160,
	192, 224, 256, 320, 384, 448, 512, 576, 640,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}
#endif
;

/* default intra quantization matrix */
unsigned char default_intra_quantizer_matrix[64]
#ifdef GLOBAL
=
{
	8, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
}
#endif
;

/* zig-zag and alternate scan patterns */
unsigned char scan[2][64]
#ifdef GLOBAL
=
{
	{ // Zig-Zag scan pattern
		0,  1,  8, 16,  9,  2,  3, 10,
	   17, 24, 32, 25, 18, 11,  4,  5,
	   12, 19, 26, 33, 40, 48, 41, 34,
	   27, 20, 13,  6,  7, 14, 21, 28,
	   35, 42, 49, 56, 57, 50, 43, 36,
	   29, 22, 15, 23, 30, 37, 44, 51,
	   58, 59, 52, 45, 38, 31, 39, 46,
	   53, 60, 61, 54, 47, 55, 62, 63
	}
	,
	{ // Alternate scan pattern
		0,  8, 16, 24,  1,  9,  2, 10,
	   17, 25, 32, 40, 48, 56, 57, 49,
	   41, 33, 26, 18,  3, 11, 4,  12,
	   19, 27, 34, 42, 50, 58, 35, 43,
	   51, 59, 20, 28,  5, 13,  6, 14,
	   21, 29, 36, 44, 52, 60, 37, 45,
	   53, 61, 22, 30,  7, 15, 23, 31,
	   38, 46, 54, 62, 39, 47, 55, 63
	}
}
#endif
;

/* non-linear quantization coefficient table */
unsigned char Non_Linear_quantizer_scale[32]
#ifdef GLOBAL
=
{
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 10, 12, 14, 16, 18, 20, 22,
	24, 28, 32, 36, 40, 44, 48, 52,
	56, 64, 72, 80, 88, 96, 104, 112
}
#endif
;

#define ERROR_VALUE	(-1)

typedef struct {
	char run, level, len;
}	DCTtab;

typedef struct {
	char val, len;
}	VLCtab;

/* Table B-10, motion_code, codes 0001 ... 01xx */
VLCtab MVtab0[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {3,3}, {2,2}, {2,2}, {1,1}, {1,1}, {1,1}, {1,1}
}
#endif
;

/* Table B-10, motion_code, codes 0000011 ... 000011x */
VLCtab MVtab1[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {7,6}, {6,6}, {5,6}, {4,5}, {4,5}
}
#endif
;

/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
VLCtab MVtab2[12]
#ifdef GLOBAL
=
{
	{16,9}, {15,9}, {14,9}, {13,9},
	{12,9}, {11,9}, {10,8}, {10,8},
	{9,8},  {9,8},  {8,8},  {8,8}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 01000 ... 111xx */
VLCtab CBPtab0[32]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
	{ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
	{62,5}, {2,5},  {61,5}, {1,5},  {56,5}, {52,5}, {44,5}, {28,5},
	{40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
	{8,4},  {8,4},  {4,4},  {4,4},  {60,3}, {60,3}, {60,3}, {60,3}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 00000100 ... 001111xx */
VLCtab CBPtab1[64]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
	{58,8}, {54,8}, {46,8}, {30,8},
	{57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
	{43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
	{41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
	{34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7},  {6,7},
	{33,7}, {33,7}, {17,7}, {17,7}, {9,7},  {9,7},  {5,7},  {5,7},
	{63,6}, {63,6}, {63,6}, {63,6}, {3,6},  {3,6},  {3,6},  {3,6},
	{36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 000000001 ... 000000111 */
VLCtab CBPtab2[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
}
#endif
;

/* Table B-1, macroblock_address_increment, codes 00010 ... 011xx */
VLCtab MBAtab1[16]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, {ERROR_VALUE,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4},
	{4,4}, {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
}
#endif
;

/* Table B-1, macroblock_address_increment, codes 00000011000 ... 0000111xxxx */
VLCtab MBAtab2[104]
#ifdef GLOBAL
=
{
	{33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
	{25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
	{19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
	{15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},
	{14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
	{13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
	{12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
	{11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
	{10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
	{9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
	{9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
	{8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},
	{8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7}
}
#endif
;

/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
VLCtab DClumtab0[32]
#ifdef GLOBAL
=
{
	{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
	{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
	{0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
	{4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {ERROR_VALUE, 0}
}
#endif
;

/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
VLCtab DClumtab1[16]
#ifdef GLOBAL
=
{
	{7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
	{8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
}
#endif
;

/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
VLCtab DCchromtab0[32]
#ifdef GLOBAL
=
{
	{0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
	{1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
	{2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
	{3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {ERROR_VALUE, 0}
}
#endif
;

/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
VLCtab DCchromtab1[32]
#ifdef GLOBAL
=
{
	{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
	{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
	{7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
	{8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for first (DC) coefficient)
 */
DCTtab DCTtabfirst[12]
#ifdef GLOBAL
=
{
	{0,2,4}, {2,1,4}, {1,1,3}, {1,1,3},
	{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1},
	{0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for all other coefficients)
 */
DCTtab DCTtabnext[12]
#ifdef GLOBAL
=
{
	{0,2,4},  {2,1,4},  {1,1,3},  {1,1,3},
	{64,0,2}, {64,0,2}, {64,0,2}, {64,0,2}, /* EOB */
	{0,1,2},  {0,1,2},  {0,1,2},  {0,1,2}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 000001xx ... 00111xxx
 */
DCTtab DCTtab0[60]
#ifdef GLOBAL
=
{
	{65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
	{2,2,7}, {2,2,7}, {9,1,7}, {9,1,7},
	{0,4,7}, {0,4,7}, {8,1,7}, {8,1,7},
	{7,1,6}, {7,1,6}, {7,1,6}, {7,1,6},
	{6,1,6}, {6,1,6}, {6,1,6}, {6,1,6},
	{1,2,6}, {1,2,6}, {1,2,6}, {1,2,6},
	{5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
	{13,1,8}, {0,6,8}, {12,1,8}, {11,1,8},
	{3,2,8}, {1,3,8}, {0,5,8}, {10,1,8},
	{0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
	{0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
	{4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
	{4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
	{3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
	{3,1,5}, {3,1,5}, {3,1,5}, {3,1,5}
}
#endif
;

/* Table B-15, DCT coefficients table one,
 * codes 000001xx ... 11111111
*/
DCTtab DCTtab0a[252]
#ifdef GLOBAL
=
{
	{65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
	{7,1,7}, {7,1,7}, {8,1,7}, {8,1,7},
	{6,1,7}, {6,1,7}, {2,2,7}, {2,2,7},
	{0,7,6}, {0,7,6}, {0,7,6}, {0,7,6},
	{0,6,6}, {0,6,6}, {0,6,6}, {0,6,6},
	{4,1,6}, {4,1,6}, {4,1,6}, {4,1,6},
	{5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
	{1,5,8}, {11,1,8}, {0,11,8}, {0,10,8},
	{13,1,8}, {12,1,8}, {3,2,8}, {1,4,8},
	{2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
	{2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
	{1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
	{1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
	{3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
	{3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
	{64,0,4}, {64,0,4}, {64,0,4}, {64,0,4}, /* EOB */
	{64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
	{64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
	{64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
	{0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
	{0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
	{0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
	{0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
	{0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
	{0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
	{0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
	{0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
	{9,1,7}, {9,1,7}, {1,3,7}, {1,3,7},
	{10,1,7}, {10,1,7}, {0,8,7}, {0,8,7},
	{0,9,7}, {0,9,7}, {0,12,8}, {0,13,8},
	{2,3,8}, {4,2,8}, {0,14,8}, {0,15,8}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0000001000 ... 0000001111
 */
DCTtab DCTtab1[8]
#ifdef GLOBAL
=
{
	{16,1,10}, {5,2,10}, {0,7,10}, {2,3,10},
	{1,4,10}, {15,1,10}, {14,1,10}, {4,2,10}
}
#endif
;

/* Table B-15, DCT coefficients table one,
 * codes 000000100x ... 000000111x
 */
DCTtab DCTtab1a[8]
#ifdef GLOBAL
=
{
	{5,2,9}, {5,2,9}, {14,1,9}, {14,1,9},
	{2,4,10}, {16,1,10}, {15,1,9}, {15,1,9}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000010000 ... 000000011111
 */
DCTtab DCTtab2[16]
#ifdef GLOBAL
=
{
	{0,11,12}, {8,2,12}, {4,3,12}, {0,10,12},
	{2,4,12}, {7,2,12}, {21,1,12}, {20,1,12},
	{0,9,12}, {19,1,12}, {18,1,12}, {1,5,12},
	{3,3,12}, {0,8,12}, {6,2,12}, {17,1,12}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000010000 ... 0000000011111
 */
DCTtab DCTtab3[16]
#ifdef GLOBAL
=
{
	{10,2,13}, {9,2,13}, {5,3,13}, {3,4,13},
	{2,5,13}, {1,7,13}, {1,6,13}, {0,15,13},
	{0,14,13}, {0,13,13}, {0,12,13}, {26,1,13},
	{25,1,13}, {24,1,13}, {23,1,13}, {22,1,13}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 00000000010000 ... 00000000011111
 */
DCTtab DCTtab4[16]
#ifdef GLOBAL
=
{
	{0,31,14}, {0,30,14}, {0,29,14}, {0,28,14},
	{0,27,14}, {0,26,14}, {0,25,14}, {0,24,14},
	{0,23,14}, {0,22,14}, {0,21,14}, {0,20,14},
	{0,19,14}, {0,18,14}, {0,17,14}, {0,16,14}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000000010000 ... 000000000011111
 */
DCTtab DCTtab5[16]
#ifdef GLOBAL
=
{
	{0,40,15}, {0,39,15}, {0,38,15}, {0,37,15},
	{0,36,15}, {0,35,15}, {0,34,15}, {0,33,15},
	{0,32,15}, {1,14,15}, {1,13,15}, {1,12,15},
	{1,11,15}, {1,10,15}, {1,9,15}, {1,8,15}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000000010000 ... 0000000000011111
 */
DCTtab DCTtab6[16]
#ifdef GLOBAL
=
{
	{1,18,16}, {1,17,16}, {1,16,16}, {1,15,16},
	{6,3,16}, {16,2,16}, {15,2,16}, {14,2,16},
	{13,2,16}, {12,2,16}, {11,2,16}, {31,1,16},
	{30,1,16}, {29,1,16}, {28,1,16}, {27,1,16}
}
#endif
;

/* Table B-3, macroblock_type in P-pictures, codes 001..1xx */
VLCtab PMBtab0[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0},
	{MACROBLOCK_MOTION_FORWARD,3},
	{MACROBLOCK_PATTERN,2}, {MACROBLOCK_PATTERN,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}, 
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}, 
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}
}
#endif
;

/* Table B-3, macroblock_type in P-pictures, codes 000001..00011x */
VLCtab PMBtab1[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0},
	{MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
	{MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
	{MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
	{MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
}
#endif
;

/* Table B-4, macroblock_type in B-pictures, codes 0010..11xx */
VLCtab BMBtab0[16]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0}, 
	{ERROR_VALUE,0},
	{MACROBLOCK_MOTION_FORWARD,4},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,4},
	{MACROBLOCK_MOTION_BACKWARD,3}, 
	{MACROBLOCK_MOTION_BACKWARD,3},
	{MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3}, 
	{MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2}, 
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2}, 
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
	{MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2}
}
#endif
;

/* Table B-4, macroblock_type in B-pictures, codes 000001..00011x */
VLCtab BMBtab1[8]
#ifdef GLOBAL
=
{
	{ERROR_VALUE,0},
	{MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
	{MACROBLOCK_QUANT|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,6},
	{MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,6},
	{MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
	{MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
	{MACROBLOCK_INTRA,5}, 
	{MACROBLOCK_INTRA,5}
}
#endif
;
