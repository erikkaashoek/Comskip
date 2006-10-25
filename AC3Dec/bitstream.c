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
#include "bitstream.h"

uint_32 bitstream_get_bh(uint_32 num_bits)
{
	uint_32 result;

	num_bits -= bits_left;
	result = (current_word << (32 - bits_left)) >> (32 - bits_left);

	current_word = (*buffer_start << 24) + (*(buffer_start+1) << 16) + (*(buffer_start+2) << 8) + *(buffer_start+3);
	buffer_start +=4;

	if(num_bits != 0)
		result = (result << num_bits) + (current_word >> (32 - num_bits));
	
	bits_left = 32 - num_bits;

	return result;
}

void bitstream_init(uint_8 *start)
{
	// initialize the start of the buffer
	buffer_start = start;
	bits_left = 0;
}
