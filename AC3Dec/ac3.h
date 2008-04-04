/* 
 *	Copyright (C) Aaron Holtzman - May 1999
 *
 *  This file is part of ac3dec, a free Dolby AC-3 stream decoder.
 *	
 *  ac3dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  ac3dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include "../config.h"


typedef unsigned int	uint_32;
typedef unsigned short	uint_16;
typedef unsigned char	uint_8;

typedef signed int		sint_32;
typedef signed short	sint_16;
typedef signed char		sint_8;


uint_32 ac3_decode_data(uint_8 *data_start, uint_32 length, uint_32 *start, short *audio_buf_ptr);

/* Exponent strategy constants */
#define EXP_REUSE	(0)
#define EXP_D15		(1)
#define EXP_D25		(2)
#define EXP_D45		(3)

/* Delta bit allocation constants */
#define DELTA_BIT_REUSE		(0)
#define DELTA_BIT_NEW		(1)
#define DELTA_BIT_NONE		(2)
#define DELTA_BIT_RESERVED	(3)

/* samples work structure */
typedef double stream_samples_t[6][256];

typedef struct syncinfo_s
{
	/* Sync word == 0x0B77 */
	uint_16 syncword; 
	/* crc for the first 5/8 of the sync block */
	/* uint_16   crc1; */
	/* Stream Sampling Rate (kHz) 0 = 48, 1 = 44.1, 2 = 32, 3 = reserved */
	uint_16	fscod;	
	/* Frame size code */
	uint_16	frmsizecod;

	/* Information not in the AC-3 bitstream, but derived */
	/* Frame size in 16 bit words */
	uint_16 frame_size;
	/* Bit rate in kilobits */
	uint_16 bit_rate;
	/* sampling rate in hertz */
	uint_32 sampling_rate;
}	syncinfo_t;

typedef struct bsi_s
{
	/* Bit stream identification == 0x8 */
	uint_16 bsid;	
	/* Bit stream mode */
	uint_16 bsmod;
	/* Audio coding mode */
	uint_16 acmod;
	/* If we're using the centre channel then */
		/* centre mix level */
		uint_16 cmixlev;
	/* If we're using the surround channel then */
		/* surround mix level */
		uint_16 surmixlev;
	/* If we're in 2/0 mode then */
		/* Dolby surround mix level - NOT USED - */
		uint_16 dsurmod;
	/* Low frequency effects on */
	uint_16 lfeon;
	/* Dialogue Normalization level */
	uint_16 dialnorm;
	/* Compression exists */
	uint_16 compre;
		/* Compression level */
		uint_16 compr;
	/* Language code exists */
	uint_16 langcode;
		/* Language code */
		uint_16 langcod;
	/* Audio production info exists*/
	uint_16 audprodie;
		uint_16 mixlevel;
		uint_16 roomtyp;
	/* If we're in dual mono mode (acmod == 0) then extra stuff */
		uint_16 dialnorm2;
		uint_16 compr2e;
			uint_16 compr2;
		uint_16 langcod2e;
			uint_16 langcod2;
		uint_16 audprodi2e;
			uint_16 mixlevel2;
			uint_16 roomtyp2;
	/* Copyright bit */
	uint_16 copyrightb;
	/* Original bit */
	uint_16 origbs;
	/* Timecode 1 exists */
	uint_16 timecod1e;
		/* Timecode 1 */
		uint_16 timecod1;
	/* Timecode 2 exists */
	uint_16 timecod2e;
		/* Timecode 2 */
		uint_16 timecod2;
	/* Additional bit stream info exists */
	uint_16 addbsie;
		/* Additional bit stream length - 1 (in bytes) */
		uint_16 addbsil;
		/* Additional bit stream information (max 64 bytes) */
		uint_8	addbsi[64];

	/* Information not in the AC-3 bitstream, but derived */
	/* Number of channels (excluding LFE)
	 * Derived from acmod */
	uint_16 nfchans;
}	bsi_t;

/* more pain */
typedef struct audblk_s
{
	/* block switch bit indexed by channel num */
	uint_16 blksw[5];
	/* dither enable bit indexed by channel num */
	uint_16 dithflag[5];
	/* dynamic range gain exists */
	uint_16 dynrnge;
		/* dynamic range gain */
		uint_16 dynrng;
	/* if acmod==0 then */
	/* dynamic range 2 gain exists */
	uint_16 dynrng2e;
		/* dynamic range 2 gain */
		uint_16 dynrng2;
	/* coupling strategy exists */
	uint_16 cplstre;
		/* coupling in use */
		uint_16 cplinu;
			/* channel coupled */
			uint_16 chincpl[5];
			/* if acmod==2 then */
				/* Phase flags in use */
				uint_16 phsflginu;
			/* coupling begin frequency code */
			uint_16 cplbegf;
			/* coupling end frequency code */
			uint_16 cplendf;
			/* coupling band structure bits */
			uint_16 cplbndstrc[18];
			/* Do coupling co-ords exist for this channel? */
			uint_16 cplcoe[5];
			/* Master coupling co-ordinate */
			uint_16 mstrcplco[5];
			/* Per coupling band coupling co-ordinates */
			uint_16 cplcoexp[5][18];
			uint_16 cplcomant[5][18];
			/* Phase flags for dual mono */
			uint_16 phsflg[18];
	/* Is there a rematrixing strategy */
	uint_16 rematstr;
		/* Rematrixing bits */
		uint_16 rematflg[4];
	/* Coupling exponent strategy */
	uint_16 cplexpstr;
	/* Exponent strategy for full bandwidth channels */
	uint_16 chexpstr[5];
	/* Exponent strategy for lfe channel */
	uint_16 lfeexpstr;
	/* Channel bandwidth for independent channels */
	uint_16 chbwcod[5];
		/* The absolute coupling exponent */
		uint_16 cplabsexp;
		/* Coupling channel exponents (D15 mode gives 18 * 12 /3  encoded exponents */
		uint_16 cplexps[18 * 12 / 3];

	/* fbw channel exponents */
	uint_16 exps[5][252 / 3];
	/* channel gain range */
	uint_16 gainrng[5];
	/* low frequency exponents */
	uint_16 lfeexps[3];

	/* Bit allocation info */
	uint_16 baie;
		/* Slow decay code */
		uint_16 sdcycod;
		/* Fast decay code */
		uint_16 fdcycod;
		/* Slow gain code */
		uint_16 sgaincod;
		/* dB per bit code */
		uint_16 dbpbcod;
		/* masking floor code */
		uint_16 floorcod;

	/* SNR offset info */
	uint_16 snroffste;
		/* coarse SNR offset */
		uint_16 csnroffst;
		/* coupling fine SNR offset */
		uint_16 cplfsnroffst;
		/* coupling fast gain code */
		uint_16 cplfgaincod;
		/* fbw fine SNR offset */
		uint_16 fsnroffst[5];
		/* fbw fast gain code */
		uint_16 fgaincod[5];
		/* lfe fine SNR offset */
		uint_16 lfefsnroffst;
		/* lfe fast gain code */
		uint_16 lfefgaincod;
	
	/* Coupling leak info */
	uint_16 cplleake;
		/* coupling fast leak initialization */
		uint_16 cplfleak;
		/* coupling slow leak initialization */
		uint_16 cplsleak;
	
	/* delta bit allocation info */
	uint_16 deltbaie;
		/* coupling delta bit allocation exists */
		uint_16 cpldeltbae;
		/* fbw delta bit allocation exists */
		uint_16 deltbae[5];
		/* number of cpl delta bit segments */
		uint_16 cpldeltnseg;
			/* coupling delta bit allocation offset */
			uint_16 cpldeltoffst[8];
			/* coupling delta bit allocation length */
			uint_16 cpldeltlen[8];
			/* coupling delta bit allocation length */
			uint_16 cpldeltba[8];
		/* number of delta bit segments */
		uint_16 deltnseg[5];
			/* fbw delta bit allocation offset */
			uint_16 deltoffst[5][8];
			/* fbw delta bit allocation length */
			uint_16 deltlen[5][8];
			/* fbw delta bit allocation length */
			uint_16 deltba[5][8];

	/* skip length exists */
	uint_16 skiple;
		/* skip length */
		uint_16 skipl;

	//Removed Feb 2000 -ah
	/* channel mantissas */
	//sint_16 chmant[5][256];

	/* coupling mantissas */
	sint_16 cplmant[256];

	//Removed Feb 2000 -ah
	/* coupling mantissas */
	//sint_16 lfemant[7];

	/* Number of coupling sub-bands */
	uint_16 ncplsubnd;

	/* Number of combined coupling sub-bands
	 * Derived from ncplsubnd and cplbndstrc */
	uint_16 ncplbnd;

	/* Number of exponent groups by channel
	 * Derived from strmant, endmant */
	uint_16 nchgrps[5];

	/* Number of coupling exponent groups
	 * Derived from cplbegf, cplendf, cplexpstr */
	uint_16 ncplgrps;
			
	/* End mantissa numbers of fbw channels */
	uint_16 endmant[5];

	/* Start and end mantissa numbers for the coupling channel */
	uint_16 cplstrtmant;
	uint_16 cplendmant;

	/* Decoded exponent info */
	uint_16 fbw_exp[5][256];
	uint_16 cpl_exp[256];
	uint_16 lfe_exp[7];

	/* Bit allocation pointer results */
	uint_16 fbw_bap[5][256];
	uint_16 cpl_bap[256];
	uint_16 lfe_bap[7];
}	audblk_t;

/* coeff */
void mantissa_init(void);
void coeff_unpack(bsi_t *bsi, audblk_t *audblk, stream_samples_t samples);

/* crc */
int crc_process_frame(uint_8 *data,uint_32 num_bytes);

/* downmix */
void drc_init(void);
void downmix(audblk_t *audblk, bsi_t* bsi, stream_samples_t stream_samples, sint_16 *s16_samples);

/* exponent */
#define UNPACK_FBW  1
#define UNPACK_CPL  2
#define UNPACK_LFE  4

void exponent_init(void);
void exponent_unpack( bsi_t *bsi, audblk_t *audblk);

/* imdct */
void imdct(bsi_t *bsi,audblk_t *audblk, stream_samples_t samples);
void imdct_init(void);

/* parse */
void parse_syncinfo(syncinfo_t *syncinfo,uint_8 *data);
void parse_audblk(bsi_t *bsi,audblk_t *audblk);
void parse_bsi(bsi_t *bsi);

/* rematrix */
void rematrix(audblk_t *audblk, stream_samples_t samples);

/* sanity check */
void sanity_check(bsi_t *bsi, audblk_t *audblk);

void InitialAC3(void);
//unsigned char AC3Dec_Buffer[49152];		// 48KB/frame for 64~448 Kbps

uint_32 sampling_rate;

uint_32 error_flag;
