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

#include "ac3.h"

static sint_16 EXPT[125][3];

static void exp_unpack_ch(uint_16 type,uint_16 expstr,uint_16 ngrps,uint_16 initial_exp, uint_16 exps[], uint_16 *dest);

void exponent_init()
{
	int i;

	for (i=0; i<125; i++)
	{
		EXPT[i][0] = i / 25;
		EXPT[i][1] = (i - EXPT[i][0] * 25) / 5;
		EXPT[i][2] = i - EXPT[i][0] * 25 - EXPT[i][1] * 5;
		EXPT[i][0] -= 2;
		EXPT[i][1] -= 2;
		EXPT[i][2] -= 2;
	}
}

void exponent_unpack( bsi_t *bsi, audblk_t *audblk)
{
	uint_16 i;

	for(i=0; i< bsi->nfchans; i++)
		exp_unpack_ch(UNPACK_FBW, audblk->chexpstr[i], audblk->nchgrps[i], audblk->exps[i][0], 
				&audblk->exps[i][1], audblk->fbw_exp[i]);

	if(audblk->cplinu)
		exp_unpack_ch(UNPACK_CPL, audblk->cplexpstr, audblk->ncplgrps, audblk->cplabsexp << 1,	
				audblk->cplexps, &audblk->cpl_exp[audblk->cplstrtmant]);

	if(bsi->lfeon)
		exp_unpack_ch(UNPACK_LFE, audblk->lfeexpstr, 2, audblk->lfeexps[0], 
				&audblk->lfeexps[1], audblk->lfe_exp);
}


static void exp_unpack_ch(uint_16 type,uint_16 expstr,uint_16 ngrps,uint_16 initial_exp, 
		uint_16 exps[], uint_16 *dest)
{
	uint_16 i,j;
	sint_16 exp_acc;

	if(expstr == EXP_REUSE)
		return;

	/* Handle the initial absolute exponent */
	exp_acc = initial_exp;
	j = 0;

	/* In the case of a fbw channel then the initial absolute values is 
	 * also an exponent */
	if(type != UNPACK_CPL)
		dest[j++] = exp_acc;

	/* Loop through the groups and fill the dest array appropriately */
	for(i=0; i<ngrps; i++)
	{
		if (exps[i] > 124)
		{
			error_flag = 1;
			return;
		}

		exp_acc += EXPT[exps[i]][0];

		switch (expstr)
		{
			case EXP_D45:
				dest[j++] = exp_acc;
				dest[j++] = exp_acc;
			case EXP_D25:
				dest[j++] = exp_acc;
			case EXP_D15:
				dest[j++] = exp_acc;
		}

		exp_acc += EXPT[exps[i]][1];

		switch (expstr)
		{
			case EXP_D45:
				dest[j++] = exp_acc;
				dest[j++] = exp_acc;
			case EXP_D25:
				dest[j++] = exp_acc;
			case EXP_D15:
				dest[j++] = exp_acc;
		}

		exp_acc += EXPT[exps[i]][2];

		switch (expstr)
		{
			case EXP_D45:
				dest[j++] = exp_acc;
				dest[j++] = exp_acc;
			case EXP_D25:
				dest[j++] = exp_acc;
			case EXP_D15:
				dest[j++] = exp_acc;
		}
	}
}
