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

struct rematrix_band_s
{
	uint_32 start;
	uint_32 end;
};

struct rematrix_band_s rematrix_band[] = {{13,24}, {25,36}, {37,60}, {61,252}};

/* This routine simply does stereo rematixing for the 2 channel stereo mode */
void rematrix(audblk_t *audblk, stream_samples_t samples)
{
	uint_32 num_bands;
	uint_32 start;
	uint_32 end;
	uint_32 i, j;
	double left, right;

	if(!audblk->cplinu || audblk->cplbegf > 2)
		num_bands = 4;
	else if (audblk->cplbegf > 0)
		num_bands = 3;
	else
		num_bands = 2;

	for(i=0; i<num_bands; i++)
	{
		if(!audblk->rematflg[i])
			continue;

		start = rematrix_band[i].start;

		if (i==num_bands-1 && audblk->cplinu)
			end = 12 * audblk->cplbegf + 36;
		else
			end = rematrix_band[i].end;

		for (j=start; j<=end; j++)
		{
			left  = samples[0][j] + samples[1][j];
			right = samples[0][j] - samples[1][j];
			samples[0][j] = left;
			samples[1][j] = right;
		}
	}
}
