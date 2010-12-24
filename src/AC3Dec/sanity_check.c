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

void sanity_check(bsi_t *bsi, audblk_t *audblk)
{
	int i;

	for(i=0; i<5 ; i++)
	{
		if (audblk->fbw_exp[i][255] !=0 || audblk->fbw_exp[i][254] !=0 || audblk->fbw_exp[i][253] !=0)
			error_flag = 1;

		if (audblk->fbw_bap[i][255] !=0 || audblk->fbw_bap[i][254] !=0 || audblk->fbw_bap[i][253] !=0)
			error_flag = 1;
	}

	if (audblk->cpl_exp[255] !=0 || audblk->cpl_exp[254] !=0 || audblk->cpl_exp[253] !=0)
		error_flag = 1;

	if (audblk->cpl_bap[255] !=0 || audblk->cpl_bap[254] !=0 || audblk->cpl_bap[253] !=0)
		error_flag = 1;

	if (audblk->cplmant[255] !=0 || audblk->cplmant[254] !=0 || audblk->cplmant[253] !=0)
		error_flag = 1;

	for(i=0; i < bsi->nfchans; i++)
		if(audblk->chincpl[i]==0 && audblk->chbwcod[i]>60)
			error_flag = 1;
}	
