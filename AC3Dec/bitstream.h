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

uint_32 bits_left;
uint_32 current_word;
uint_8 *buffer_start;

void bitstream_init(uint_8 *start);
uint_32 bitstream_get_bh(uint_32 num_bits);

__forceinline static uint_32 bitstream_get(uint_32 num_bits)
{
	uint_32 result;
	
	if (num_bits < bits_left)
	{
		result = (current_word << (32 - bits_left)) >> (32 - num_bits);
		bits_left -= num_bits;
		return result;
	}

	return bitstream_get_bh(num_bits);
}
