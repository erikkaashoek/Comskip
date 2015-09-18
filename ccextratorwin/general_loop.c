#include "../platform.h"
#include "ccextractor.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* General video information */
unsigned current_hor_size = 0;
unsigned current_vert_size = 0;
unsigned current_aspect_ratio = 0;
unsigned current_frame_rate = 0;
unsigned current_bit_rate = 0;
LONG current_pts = 0;
int ts_headers_total=0;
LONG result; // Number of bytes read/skipped in last read operation
LONG net_fields=20; // 0.333 to sync with video, not sure of this.

extern int cc608_parity_table[256]; // From myth

const unsigned char DO_NOTHING[] = {0x80, 0x80};
int full_pes=0;
LONG inbuf = 0; // Number of bytes loaded in buffer
unsigned char tsheader[4]; // Last TS header read - might be the next one
int next_ts_header_read=0; // Did we read the TS header for the next block already?
int end_of_file=0; // End of file?

int current_picture_coding_type = RESET_OR_UNKNOWN;
int last_picture_coding_type = RESET_OR_UNKNOWN; // JOHN BELL
int twoback_picture_coding_type = RESET_OR_UNKNOWN; // JOHN BELL
int p_caption_size = 0, p_caption_capacity = 0;
unsigned char *p_caption = NULL;

int non_compliant_DVD = 0; // Found extra captions in DVDs?

void calculate_ccblock_gop_time (struct gop_time_code *g)
{
	int seconds=(g->time_code_hours*3600)+(g->time_code_minutes*60)+g->time_code_seconds;
	g->ccblocks=(int) (seconds*29.97)+g->time_code_pictures; // Only correct if fps=29.97. FIX!
	if (gop_rollover)
		g->ccblocks+=(int) (86400*29.97);
}

int gop_accepted(struct gop_time_code* g )
{
	if (! ((g->time_code_hours <= 23)
		&& (g->time_code_minutes <= 59)
		&& (g->time_code_seconds <= 59)
		&& (g->time_code_pictures <= 59)))
		return 0;

	if (gop_time.time_code_hours==23 && gop_time.time_code_minutes==59 &&
		g->time_code_hours==0 && g->time_code_minutes==0)
	{
		gop_rollover = 1;
		return 1;
	}
	if (gop_time.inited)
	{
		if (gop_time.ccblocks>g->ccblocks)
		{
			// We are going back in time but it's not a complete day rollover
			return 0;
		}
	}
	return 1;
}

void set_cc_pts(int64_t pts)
{
	current_pts=pts;
	if (pts_set==0)
		pts_set=1;
}

void update_clock (void)
{
	if (pts_set!=0)
	{
		int dif;
		pts_set=2;
		if (current_pts<min_pts)
			min_pts=current_pts;
		if (current_pts>max_pts)
			max_pts=current_pts;
		dif=(int) (current_pts-last_pts);
		dif=dif/MPEG_CLOCK_FREQ;
		if (dif<0) // Only a problem for more than one sec
		{
			// TO DO: Deal with this.
			printf ("\nThe clock is going backwards -  %ld seconds)\n",
				(last_pts-current_pts)/MPEG_CLOCK_FREQ);
		}
		if (dif<0 || dif>=5)
		{
			// ATSC specs: More than 3501 ms means missing component
			printf ("\nWarning: Reference clock has changed abruptly (%d seconds), attempting to synchronize\n", (int) dif);
			printf ("Last PTS value: %lu\n",last_pts);
			dump (lastptsdata,5);
			printf ("Current PTS value: %lu\n",current_pts);
			dump (ptsdata,5);
			c1count_total+=c1count;
			c2count_total+=c2count;
			c1count=0;
			c2count=0;
			pts_set=1; // Force min and max to be set again
			min_pts=0xFFFFFFFF;
			max_pts=0;
			pts_big_change=1;
		}
		last_pts=current_pts;
		memcpy (lastptsdata, ptsdata,5);
	}
}

#define FILEBUFFERSIZE 1024*1024*16 // 16 Mbytes no less. Minimize number of real read calls()

unsigned char *filebuffer;
LONG filebuffer_start; // Position of buffer start relative to file
int filebuffer_pos; // Position of pointer relative to buffer start
int bytesinbuffer; // Number of bytes we actually have on buffer




void do_padding (int mis1, int mis2)
{
	int i;
	for (i=0; i<mis1; i++)
	{
		printdata (DO_NOTHING,2,0,0);
		c1count++;
	}

	for (i=0; i<mis2; i++)
	{
		printdata (0,0,DO_NOTHING,2);
		c2count++;
	}
}

void gop_padding ()
{
	if (first_gop_time.inited)
	{
		// TO-DO: We need to handle clock roll over.
		int mis1=(int) ((gop_time.ccblocks+frames_since_last_gop-first_gop_time.ccblocks)-c1count);
		int mis2=(int) ((gop_time.ccblocks+frames_since_last_gop-first_gop_time.ccblocks)-c2count);
		do_padding (mis1,mis2);
		// printf ("\nCurrent offset: %d\n",mis1);
	}
}

void pts_padding ()
{
	int exp=(int) ((current_pts-min_pts) * 29.97 / MPEG_CLOCK_FREQ);
	int mis1 = exp - c1count;
	int mis2 = exp - c2count;
	do_padding (mis1, mis2);
}

int init_file_buffer(void)
{
	filebuffer_start=0;
	filebuffer_pos=0;
	bytesinbuffer=0;
	if (filebuffer==NULL)
		filebuffer=(unsigned char *) malloc (FILEBUFFERSIZE);
	if (filebuffer==NULL)
		return -1;
	return 0;
}

void buffered_seek (int offset)
{
	if (offset<0)
	{
		filebuffer_pos+=offset;
		if (filebuffer_pos<0)
		{
			// We got into the start buffer (hopefully)
			startbytes_pos+=filebuffer_pos;
			filebuffer_pos=0;
			if (startbytes_pos<=0)
			{
				printf ("PANIC: Attempt to seek before buffer start, this is a bug!");
				exit (-4);
			}
		}
	}
	else
		buffered_read_opt (NULL, offset);
}


LONG buffered_read_opt (unsigned char *buffer, unsigned int bytes)
{
	LONG copied=0;
	int keep,copy,i;
	if (buffer_input || filebuffer_pos<bytesinbuffer)
	{
		// Needs to return data from filebuffer_start+pos to filebuffer_start+pos+bytes-1;
		int eof=0;

		while (!eof && bytes)
		{
			size_t ready = bytesinbuffer-filebuffer_pos;
			if (ready==0) // We really need to read more
			{
				if (!buffer_input)
				{
					// We got in the buffering code because of the initial buffer for
					// detection stuff. However we don't want more buffering so
					// we do the rest directly on the final buffer.
					int i;
					do
					{
						i=read (in,fbuffer,bytes);
						copied+=i;
						bytes-=i;
						fbuffer+=i;
					}
					while (i && bytes);
					return copied;
				}
				// Keep the last 8 bytes, so we have a guaranteed
				// working seek (-8) - needed by mythtv.
				keep = bytesinbuffer > 8 ? 8 : bytesinbuffer;
				memmove (filebuffer,filebuffer+(FILEBUFFERSIZE-keep),keep);
				i =read (in, filebuffer+keep,FILEBUFFERSIZE-keep);
				if (i==0)
					eof=1;
				filebuffer_pos=keep;
				bytesinbuffer=(int) i+keep;
				ready=i;
			}
			copy = (int) (ready>=bytes ? bytes:ready);
			if (fbuffer!=NULL)
			{
				memcpy (fbuffer, filebuffer+filebuffer_pos, copy);
				buffer+=copy;
			}
			filebuffer_pos+=copy;
			bytes-=copy;
			copied+=copy;
		}
		return copied;
	}
	else
	{
		if (fbuffer!=NULL)
			return copied+read(in,fbuffer,bytes);
		// return fread(buffer,1,bytes,in);
		//return FSEEK (in,bytes,SEEK_CUR);
		return copied + LSEEK (in,bytes,SEEK_CUR);
	}
}

// TS specific data grabber
long ts_getmoredata(void)
{
	int paystart=0;
	int pes_start_in_this_pass = 0;
	int enough = 0;

	int got_pes_header = 0;
	int payload_read = 0;
	int dump_tspacket = 0;
	unsigned payload_start = 0;
	unsigned payload_length;
	int ts_adaptation = 0;
	unsigned error;
	int pid;
	unsigned adapt;
	int want;
	full_pes=0;
	do
	{
		if (BUFSIZE-inbuf<TS_PACKET_PAYLOAD_LENGTH)
			enough=1; // Not enough for a complete TS payload
		else
		{
			if (next_ts_header_read)
			{
				// We read it in the previous pass already
				result=4;
				next_ts_header_read=0;
			}
			else
			{
				buffered_read_4(tsheader);
				if (result!=4)
				{
					// Consider this the end of the show.
					end_of_file=1;
					break;
				}
				past+=result;
				ts_headers_total++;
			}

			if (tsheader[0]!=0x47)
			{
				printf ("\nProblem: No TS header mark. Received bytes:\n");
				dump (tsheader,4);
				printf ("Trying to continue anyway.\n");
			}
			error = (tsheader[1]&0x80)>>7;
			payload_start = (tsheader[1]&0x40)>>6;
			// unsigned priority = (tsheader[1]&0x20)>>5;
			pid =    (((tsheader[1] & 0x1F) << 8) | tsheader[2]) & 0x1FFF;
			if (pid < 0x10 || pid >= 0x1FFF) // Skip PATs and NULLs
			{
				buffered_skip (TS_PACKET_PAYLOAD_LENGTH);
				continue;
			}
			// unsigned scrambling = (tsheader[3] & 0xC0) >> 6;
			adapt = (tsheader[3] & 0x30) >> 4;
			// unsigned ccounter = tsheader[3] & 0xF;
			if (error)
			{
				printf ("Warning: Defective TS packet: %u\n", error);
				dump_tspacket=1;
			}
			ts_adaptation = adapt & 2;
			payload_length=TS_PACKET_PAYLOAD_LENGTH;

			if (payload_start)
			{
				paystart++;
				if (got_pes_header) // Starting new PES. Rollback and out
				{
					//FSEEK (in,-4,SEEK_CUR);
					//past=past-4;
					next_ts_header_read = 1;
					full_pes=1; // We have the whole PES in buffer
					break;
				}
				else
				{
					unsigned stream_id;
					// Start of our own PES. First, get rid of the
					// adaptation bytes if needed
					if (ts_adaptation)
					{
						// printf ("Packet with adaptation data.\n");
						unsigned char adlength;
						buffered_read (&adlength, 1);
						past=past + result;
						payload_length=payload_length - adlength -1;
						buffered_skip(adlength);
						past=past+adlength;
					}
					pes_start_in_this_pass=1;
					// Get the f*****g PES header, NOT touching the buffer
					buffered_read (pesheaderbuf,6);
					past=past+result;
					payload_length=payload_length-6;
					if (pesheaderbuf[0]!=0x00 || pesheaderbuf[1]!=0x00 ||
						pesheaderbuf[2]!=0x01)
					{
						// PMT or whatever. Useless, skip them
						buffered_skip ((int) payload_length);
						continue;
					}
					got_pes_header=1;
					stream_id = pesheaderbuf[3];
					if (stream_id!=0xBE && stream_id!=0xBF)
					{
						unsigned PTS_present;
						unsigned pes_header_length;
						int need_to_skip;
						// Extension present, get it
						buffered_read (pesheaderbuf+6,3);
						past=past+result;
						payload_length=payload_length-3;
						// unsigned pes_header10 = (pesheaderbuf[6] & 0xC0) >> 6; // Should always be 10
						//unsigned flags = ((pesheaderbuf[6] & 0x3F)<<8) | buffer[7];
						// unsigned PESSC = (pesheaderbuf[6] & 0x30) >> 4; // Scrambled? - --XX----
						//unsigned prio = (pesheaderbuf[6] & 0x8)>>3; // ----X---
						//unsigned ali = (pesheaderbuf[6] & 0x4)>>2; // -----X--
						// unsigned cy = (pesheaderbuf[6] & 0x2) >>1 ; // ------X-
						//unsigned ooc = (pesheaderbuf[6] & 0x1); // -------X
						PTS_present = (pesheaderbuf[7] & 0x80) >> 7; // XX------
						// unsigned DTS_present = (pesheaderbuf[7] & 0x40) >> 6; // -X------
						// unsigned ESCR = (pesheaderbuf[7] && 0x20) >> 5;
						// unsigned rate = (pesheaderbuf[7] && 0x10) >> 4;
						// unsigned DSM_trick = (pesheaderbuf[7] && 0x8) >> 3;
						// unsigned additional_copy = (pesheaderbuf[7] && 0x4) >> 2;
						// unsigned pes_crc = (pesheaderbuf[7] && 0x2) >> 1;
						// unsigned pes_extension = (pesheaderbuf[7] && 0x1) ;
						pes_header_length=pesheaderbuf[8];
						need_to_skip = pes_header_length;
						if (PTS_present)
						{
							// There is time info, read it
							unsigned char pts_raw[5];
							buffered_read (pts_raw,5);
							past=past+result;
							payload_length=payload_length-5;
							need_to_skip=need_to_skip-5;
							if ((pts_raw[0]&1) && (pts_raw[2]&1) && (pts_raw[4]&1))
							{
								unsigned bits_9 =  (pts_raw[0] & 0x0E) << 28;
								unsigned bits_10 = pts_raw[1] << 22;
								unsigned bits_11 = (pts_raw[2] & 0xFE) << 14;
								unsigned bits_12 = pts_raw[3] << 7;
								unsigned bits_13 = pts_raw[4] >> 1;
								current_pts = bits_9 | bits_10 | bits_11 | bits_12 | bits_13;
								if (pts_set==0)
									pts_set=1;
							}
							memcpy (ptsdata, pts_raw, 5);
						}
						if (need_to_skip<0)
							printf ("Something's wrong here.\n");

						buffered_skip(need_to_skip);
						past=past+need_to_skip;
						payload_length=payload_length-need_to_skip;
						/*
						int crap_length=0;
						if (DTS_present)
						crap_length+=5;
						if (ESCR)
						crap_length+=6;
						if (rate)
						crap_length+=3;
						if (DSM_trick)
						crap_length++;
						if (additional_copy)
						crap_length++;
						if (pes_crc)
						crap_length+=2;
						if (pes_extension)
						crap_length++;
						*/
					}
				}
			}
			want = (int) ((BUFSIZE-inbuf)>payload_length ? payload_length : (BUFSIZE-inbuf));

			buffered_read (fbuffer+inbuf,want);

			if (result>0)
				payload_read+=(int) result;
			past=past+result;
			if (dump_tspacket)
			{
				printf ("Payload dump:\n");
				dump_tspacket=0;
				dump (fbuffer+inbuf,TS_PACKET_PAYLOAD_LENGTH);
			}
			inbuf+=result;

		}
	}
	while (result!=0 && !enough && BUFSIZE!=inbuf);

	if ((pes_start_in_this_pass==0 || full_pes==0) && result) // result>0 means no EOF
	{
		printf ("Warning: We don't have the complete PES in buffer.\n");
		printf ("Things may start to go wrong from this point.\n");
	}

	return payload_read;
}


// Returns number of bytes read, or zero for EOF
LONG general_getmoredata(void)
{
	do
	{
		int want = (int) (BUFSIZE-inbuf);
        buffered_read (fbuffer+inbuf,want);
        //result=read (in,buffer+inbuf,want);
		past=past+result;
		inbuf+=result;
	} while (result!=0 && BUFSIZE!=inbuf);
	return result;
}

// Raw file process
void raw_loop ()
{
	long i;
	do
	{
		inbuf=0;
		general_getmoredata();
		for (i=0; i<inbuf; i=i+2)
		{
			if (c1count<2 && *(fbuffer+i)==0xff && *(fbuffer+i+1)==0xff)
			{
				// Skip broadcast header
			}
			else
			{
				printdata (fbuffer+i,2,NULL,0);
				c1count++;
			}
		}
	}
	while (inbuf);
}

LONG process_block (unsigned char *data, LONG length)
{
	int limit;
    int printed;

	int j;
	unsigned char *header = data;
	unsigned char *endofbuffer = data+length; // -4 so we can look ahead 4 safely

	if (length<4)
		return length;

	for (;;)
	{
#ifdef undef
		header = (unsigned char *) memchr (header, 0, endofbuffer-header);

		if (header==NULL)
		{
			// We don't even have the starting 0x00
			header = data + length;
			break;
		}

		/* Here we are guaranteed to have at least 'onepass' bytes ready to be
		processed, or, if we reached the end of file, at least some zeros */
        /* Picture header? */
		if (header[1]==0x00 && header[2]==0x01 && header[3]==0x00
			&& (header[4]!=0x00 || header[5]!=0x00))
		{
			int temp;
			int se;
			int extension_present;
			if (header+26>endofbuffer) // Not enough for a picture header with ext.
				break;

			temp = header[5] >> 3 & 7;
			extension_present=0;
			for (se=0; se<16; se=se+1)
			{
				if (header[9+se]==0x01 && header[10+se]==0xb5 && (header[11+se]&0xf0)==0x80)
				{
                    if (header[14+se]&2) // Repeat first field?
                        extension_present=2;
                    else
					    extension_present=1;
					break;
				}
			}
			if ((temp==I_FRAME || temp==B_FRAME || temp==P_FRAME)
				&& extension_present)
			{
                net_fields+=extension_present; // So 2 if repeat field first==1
				current_picture_coding_type = temp;
				// printf ("New picture type: %d, frame type=%s\n", temp, pict_types[temp]);
				total_frames_count++;
				frames_since_last_gop++;
				if (current_picture_coding_type==B_FRAME)
				{
					update_clock();
				}
				header=header+5;
				continue;
			}
			else
			{
				false_pict_header++;
				// header=header+4; // If here, most likely it was a false positive
				//header=orig_header+1;
				header++;
				continue;
			}
        }

		/* Is this a user data header? */
		else
		if (header[1]==0x00 && header[2]==0x01 && header[3]==0xb2)
		{
		    if (header+ONEPASS>endofbuffer) // Not enough for a complete CC block. Later
			    break;

		    stat_numuserheaders++;
		    header+=4;
#endif
		    if (header+4>endofbuffer) // Not enough for a complete CC block. Later
			    break;

			/* DVD CC header */
		    if (header[0]==0x43 && header[1]==0x43)
		    {
				unsigned char pattern;
				int field1packet;
				int i,j;
				int capcount;
			    if (non_compliant_DVD &&
    				current_picture_coding_type==B_FRAME && autopad)
			    {
				    gop_padding();
			    }

			    stat_dvdccheaders++;
			    header+=4; /* Header plus 2 bytes (\x01, \xf8) */
			    pattern=header[0] & 0x80;
			    field1packet = 0; /* expect Field 1 first */
			    if (pattern==0x00)
				    field1packet=1; /* expect Field 1 second */
			    capcount=(header[0] & 0x1e) / 2;
			    header++;
			    for (i=0; i<capcount; i++)
			    {
				    unsigned char data1[2]={0x80,0x80}, data2[2]={0x80,0x80};
				    for (j=0;j<2;j++)
				    {
					    unsigned char data[3];
					    data[0]=header[0];
					    data[1]=header[1];
					    data[2]=header[2];
					    header+=3;
					    /* Field 1 and 2 data can be in either order,
					    with marker bytes of \xff and \xfe
					    Since markers can be repeated, use pattern as well */
					    if (data[0]==0xff && j==field1packet)
					    {
						    data1[0]=data[1];
						    data1[1]=data[2];
					    }
					    else
					    {
						    data2[0]=data[1];
						    data2[1]=data[2];
					    }
				    }
				    if (non_compliant_DVD==0 ||
					    (data1[0]!=0x80 && data1[0]!=0) ||
					    (data1[1]!=0x80 && data1[1]!=0) ||
					    !too_many_blocks())
				    {
					    printdata (data1,2,data2,2);
					    c1count++;
					    c2count++;
				    }
			    }
			    // Deal with extra closed captions some DVD have.
			    while (header[0]==0xfe || header[0]==0xff)
			    {
					int j;
				    unsigned char data1[2]={0x80,0x80}, data2[2]={0x80,0x80};
				    for (j=0;j<2;j++)
				    {
					    unsigned char data[3];
					    data[0]=header[0];
					    data[1]=header[1];
					    data[2]=header[2];
					    header+=3;
					    /* Field 1 and 2 data can be in either order,
					    with marker bytes of \xff and \xfe
					    Since markers can be repeated, use pattern as well */
					    if (data[0]==0xff && j==field1packet)
					    {
    						data1[0]=data[1];
						    data1[1]=data[2];
					    }
					    else
					    {
						    data2[0]=data[1];
						    data2[1]=data[2];
					    }
				    }
				    if ((data1[0]!=0x80 && data1[0]!=0) ||
					    (data1[1]!=0x80 && data1[1]!=0) ||
					    !too_many_blocks())
				    {
					    non_compliant_DVD=1;
					    printdata (data1,2,data2,2);
					    c1count++;
					    c2count++;
				    }
				    //c1count++;
				    //c2count++;
			    }
		    }
		    /* DVB closed caption header for ReplayTV  */
		    else
		    if ((header[0]==0xbb && header[1]==0x02) ||
			    (header[2]==0x99 && header[3]==0x02))
		    {
			    unsigned char data1[2], data2[2];
			    if (header[0]==0xbb)
				    stat_replay4000headers++;
			    else
				    stat_replay5000headers++;
			    header+=2;
			    data2[0]=header[0];
			    data2[1]=header[1];
			    header+=4; // Skip two bytes (\xcc\x02 for R4000 or \xaa\x02 for R5000)
			    data1[0]=header[0];
			    data1[1]=header[1];
			    printdata (data1,2,data2,2);
			    c1count++;
			    c2count++;
		    }
		    /* HDTV */
		    else
		    if (header[0]==0x47 && header[1]==0x41 &&
			    header[2]==0x39 && header[3]==0x34)
		    {
			    if (header+5>endofbuffer) // Not enough for CC captions
			    {
    				header=header-4; // So the user-data bytes are found later
				    break;
			    }
			    stat_hdtv++;
			    if (header[4]==0x03) // User data.
			    {
					unsigned char *cc_data;
				    unsigned char ud_header;
				    unsigned char cc_count;
				    unsigned char process_cc_data_flag;

				    // Untest stuff ported from John Bell's code
				    if (current_picture_coding_type==RESET_OR_UNKNOWN)
				    {
					    // printf ("Got new data but no previous picture header!\n");
					    if (last_picture_coding_type==P_FRAME)
					    {
						    current_picture_coding_type=B_FRAME;
						    twoback_picture_coding_type=P_FRAME;
						    last_picture_coding_type=B_FRAME;
					    }
					    if (last_picture_coding_type==B_FRAME &&
						    twoback_picture_coding_type==B_FRAME)
					    {
						    current_picture_coding_type=P_FRAME;
						    twoback_picture_coding_type=B_FRAME;
						    last_picture_coding_type=P_FRAME;
					    }
					    if (last_picture_coding_type==B_FRAME &&
						    twoback_picture_coding_type==P_FRAME)
					    {
						    current_picture_coding_type=B_FRAME;
						    twoback_picture_coding_type=B_FRAME;
						    last_picture_coding_type=B_FRAME;
					    }
					    if (debug)
						    printf ("\rPicture time assumed to be: %d\n",last_picture_coding_type);
				    }
				    if (current_picture_coding_type!=B_FRAME)
				    {
					    // I or P picture
					    // FLUSH_CC_BUFFERS();
				    }
				    if (current_picture_coding_type==B_FRAME && autopad)
				    {
					    if (gop_pad)
						    gop_padding();
					    else
						    if (pts_set==2)
							    pts_padding();
				    }
				    ud_header=header[5];
				    cc_count = ud_header & 0x1F;
				    process_cc_data_flag = (ud_header & 0x40) >> 6;
				    if (process_cc_data_flag)
				    {
					    int j,bail;
					    int proceed = 1;
					    cc_data = header+7;	// Skip header + EM data
					    if (cc_data+cc_count*3>endofbuffer) // Not enough for CC captions
					    {
						    header=header-4;
						    break;
					    }
					    if (cc_data[cc_count*3]!=0xFF)
					    {
    						proceed=0;
					    }
					    limit = cc_count*3;
					    printed = 0;
					    if (!proceed && debug)
					    {
						    printf ("\rThe following payload seems to be CC but is not properly terminated.\n");
						    printf ("(it will be processed anyway).\n");
						    dump (header-4, 128);
						    printed=1;
						    //header=orig_header+1;
						    //continue;
					    }
					    bail=0;
					    // Packet stinks. We give it a change to prove itself
					    // by passing all the parity checks. It it doesn't,
					    // reject it.
					    if (!proceed && debug)
					    {
						    // proceed=1;
						    printf ("This packet is not correctly terminated.\n");
						    printf ("Data start at offset %d of a %d bytes block.\n",
    							(int) ((header-data-4)),(int) length);
						    dump (data,256);
					    }
					    for (j=0; j<limit; j=j+3)
					    {
						    unsigned char cc_valid;
						    unsigned char cc_type;

						    //unsigned char marker=cc_data[j] & 0xF8;
						    if (!proceed && ff_cleanup)
						    {
							    // Packet stinks. Treat it with care
							    if (cc_data[j]==0xFA &&
								    cc_data[j+1]==0x00 &&
								    cc_data[j+2]==0x00)
								    break;
							    if (!cc608_parity_table[cc_data[j+1]] ||
								    !cc608_parity_table[cc_data[j+2]] )
								    break;
						    }
						    cc_valid = (cc_data[j] & 4) >>2;
						    cc_type = cc_data[j] & 3;
						    if (cc_valid==0 && cc_data[j+1]==0 &&
							    cc_data[j+2]==0 && fix_padding)
						    {
								/* Padding */
								cc_valid=1;
								cc_data[j+1]=0x80;
								cc_data[j+2]=0x80;
						    }
						    if (cc_valid)
						    {
							    cc_stats[cc_type]++;
							    if (bail && (cc_type==0 || cc_type==1) && debug)
							    {
								    if (!printed)
								    {
									    printf ("\rThis packet is not supposed to have more CC but it does!\n");
									    dump (header-4,128);
								    }
								    else
									    printf ("\rThe PREVIOUSLY dumped packet was not supposed to have more CC but it did!\n");
							    }
							    if (cc_type==0) // Field 1
							    {
								    if (1 /* current_picture_coding_type==B_FRAME */)
								    {
									    printdata (cc_data+j+1,2,0,0);
									    c1count++;
								    }
								    else
								    {
									    if (used_caption_buffer_1<MAX_CLOSED_CAPTION_DATA_PER_PICTURE)
									    {
										    captions_buffer_1[used_caption_buffer_1++]=cc_data[j+1];
										    captions_buffer_1[used_caption_buffer_1++]=cc_data[j+2];
									    }
								    }
							    }
							    else
							    {
								    if (cc_type==1) // Field 2
								    {
									    if (1 /* current_picture_coding_type==B_FRAME */ )
									    {
										    printdata (0,0,cc_data+j+1,2);
										    c2count++;
									    }
									    else
									    {
										    if (used_caption_buffer_2<MAX_CLOSED_CAPTION_DATA_PER_PICTURE)
										    {
											    captions_buffer_2[used_caption_buffer_2++]=cc_data[j+1];
											    captions_buffer_2[used_caption_buffer_2++]=cc_data[j+2];
										    }
									    }
								    }
								    else
								    {
									    bail=1;
									    // Remove the // in the break and you'll lose
									    // characters. So much for following standards :-(
									    // break;
								    }
							    }
						    } // cc_valid
					    }
					    // printf ("-----------------------------------\n");
					    header=header + 7 + limit;
					    if (header>endofbuffer) // using 'limit' could cause problems
						    header=endofbuffer;
				    }
				    // JOHN BELL STUFF
				    twoback_picture_coding_type=last_picture_coding_type;
				    if (current_picture_coding_type==B_FRAME)
					    last_picture_coding_type=B_FRAME;
				    else
    					last_picture_coding_type=P_FRAME;
				    current_picture_coding_type=RESET_OR_UNKNOWN;
				    // End of JOHN BELL STUFF
			    }
				header++;
				continue;
		    }
		    /* DVB closed caption header for Dish Network (Field 1 only) */
		    else
		    if (header[0]==0x05 && header[1]==0x02)
		    {
			    unsigned char type;
			    unsigned char hi;
			    unsigned char data1[2];

			    if (current_picture_coding_type==B_FRAME && autopad)
			    {
				    gop_padding();
			    }
			    stat_dishheaders++;
			    header+=7; /* skip 7 bytes (the CC header we just read plus \x04, a 2 byte counter, and 2 varied bytes) */
			    type = header[0]; /* pattern type (\x02, \x04 or \x05) */
			    header++;
			    switch (type)
			    {
			        case 0x02:
				    header++; /* skip 1 byte (\x09) */
				    data1[0]=header[0];
				    data1[1]=header[1];
				    header+=2;
				    type=header[0];  /* repeater (\x02 or \x04) */
				    header++;
				    if ((data1[0]!=0x80 && data1[0]!=0) ||
					    (data1[1]!=0x80 && data1[1]!=0) ||
					    !too_many_blocks())
				    {
					    printdata (data1, 2, NULL, 0);
					    c1count++;
				    }
				    hi = data1[0] & 0x7f; // Get only the 7 low bits */
				    if (type==0x04 && hi<32) /*	repeat (only for non-character pairs) */
				    {
					    if ((data1[0]!=0x80 && data1[0]!=0) ||
						    (data1[1]!=0x80 && data1[1]!=0) ||
						    !too_many_blocks())
					    {
						    printdata (data1, 2, DO_NOTHING, sizeof (DO_NOTHING));
						    c1count++;
						    c2count++;
					    }
				    }
				    header+=3; /* skip 3 bytes (\x0a, followed by 2-byte checksum?) */
				    break;
			    case 0x04:
				    header++; /* skip 1 byte (\x09) */
				    data1[0]=header[0];
				    data1[1]=header[1];
				    header+=2;
				    if ((data1[0]!=0x80 && data1[0]!=0) ||
					    (data1[1]!=0x80 && data1[1]!=0) ||
					    !too_many_blocks())
				    {
					    printdata (data1, 2, DO_NOTHING, sizeof (DO_NOTHING));
					    c1count++;
					    c2count++;
				    }
				    data1[0]=header[0];
				    data1[1]=header[1];
				    header+=2;
				    if ((data1[0]!=0x80 && data1[0]!=0) ||
					    (data1[1]!=0x80 && data1[1]!=0) ||
					    !too_many_blocks())
				    {
					    printdata (data1, 2, DO_NOTHING, sizeof (DO_NOTHING));
					    c1count++;
					    c2count++;
				    }
				    header+=4; /* skip 4 bytes (\x020a, followed by 2-byte checksum?) */
				    break;
			    case 0x05:
				    /* play the previous P-caption first */
				    for (j=0; j<p_caption_size; j+=2)
				    {
					    printdata (p_caption+j, 2, NULL, 0);
					    c1count++;
				    }
				    p_caption_size=0;
				    header+=6; /* skip 6 bytes (\x04, followed by 5 bytes from last 0x05 pattern) */
				    type=header[0]; /* number of caption bytes (\x02 or \x04) */
				    header+=2; /* Skip an additional byte, (\x09) */
				    data1[0]=header[0];
				    data1[1]=header[1];
				    header+=2;
				    if (p_caption_capacity<2)
				    {
					    p_caption=(unsigned char *) realloc (p_caption,1024);
					    p_caption_capacity=1024;
				    }
				    p_caption[0]=data1[0];
				    p_caption[1]=data1[1];
				    p_caption_size=2;
				    if (type==0x02)
				    {
					    type=header[0];  /* repeater (\x02 or \x04) */
					    header++;
					    hi = data1[0] & 0x7f; // Get only the 7 low bits */
					    if (type==0x04 && hi<32)
					    {
						    if (p_caption_capacity<(p_caption_size+2))
						    {
							    p_caption=(unsigned char *) realloc (p_caption,p_caption_capacity+1024);
							    p_caption_capacity+=1024;
						    }
						    p_caption[p_caption_size]=data1[0];
						    p_caption[p_caption_size+1]=data1[1];
						    p_caption_size+=2;
					    }
				    }
				    else
				    {
					    /* More caption bytes */
					    data1[0]=header[0];
					    data1[1]=header[1];
					    header+=2;
					    if (p_caption_capacity<(p_caption_size+2))
					    {
    						p_caption=(unsigned char *) realloc (p_caption,p_caption_capacity+1024);
						    p_caption_capacity+=1024;
					    }
					    p_caption[p_caption_size]=data1[0];
					    p_caption[p_caption_size+1]=data1[1];
					    p_caption_size+=2;
					    header++; /* repeater (always \x02) */
				    }
				    header+=3; /* skip 3 bytes (\x0a, followed by 2-byte checksum?) */
				    break;
			    default:
				    // printf ("Unknown?\n");
				    break;
			    } // switch
				header++;
				continue;
		    } // if (Dish)
#ifdef undef
        }
		/* Group of pictures */
		else
		if (header[1]==0x00 && header[2]==0x01 && header[3]==0xb8 && ((header[7]&0x1f)==0))
		{
			struct gop_time_code gtc;

			if (header+7>endofbuffer) // Not enough for GOP
				break;

			gtc.time_code_hours = header[4] >> 2 & 0x1f;
			gtc.time_code_minutes = (header[4] & 3)<<4 | ((header[5]>>4) & 0xf);
			gtc.time_code_seconds = (header[5] & 7) << 3 | ((header[6]>>5) & 0x7);
			gtc.time_code_pictures = (header[6] & 0x1f) << 1 | ((header[7]>>7) & 1);
			gtc.inited = 1;
			calculate_ccblock_gop_time(&gtc);

			if (gop_accepted(&gtc))
			{
				if (gtc.ccblocks>gop_time.ccblocks+300
					&& first_gop_time.inited)
				{
					if (debug)
						printf ("\rWarning: Large GAP in GOP timing, ignoring new timing.\n");
				}
				else
				{
					if (first_gop_time.inited == 0)
					{
						first_gop_time = gtc;
						//first_gop_time.ccblocks-=net_fields; // Compensate for those written before
						first_gop_time.ccblocks-=c1count;
					}
					gop_time = gtc;
					/* printf ("\n%02u:%02u:%02u:%02u",gtc.time_code_hours,gtc.time_code_minutes,
					gtc.time_code_seconds,gtc.time_code_pictures); */
					frames_since_last_gop=0;
				}
			}
			header++;
			continue;
		}
        else
		/* Sequence? We could use the frame rate. We only trust sequence
		candidates early in the PES though: The rest are usually false positives */
		if (ts_mode && header[1]==0x00 && header[2]==0x01 &&
			header[3]==0xB3 && ((header-data)<0x80))
		{
			unsigned hor_size;
			unsigned vert_size;
			unsigned aspect_ratio;
			unsigned frame_rate;
			unsigned bit_rate;

			if (header+9>endofbuffer) // Not enough for a sequence
				break;

			hor_size = (header[4]<<4) | ((header[5] & 0xF) >> 4);
			vert_size = ((header[5] & 0xF)<<8) | header[6];
			aspect_ratio = header[7]>>4;
			frame_rate = header[7] & 0xF;
			bit_rate = (header[8]<<10) | (header[9]<<2) |
				(header[10]>>6);
			bit_rate = (bit_rate * 50); // Original = units of 400 bits, so *50 = kb
			if (hor_size!=current_hor_size ||
				vert_size!=current_vert_size ||
				aspect_ratio!=current_aspect_ratio ||
				frame_rate!=current_frame_rate ||
				bit_rate!=current_bit_rate)
			{
				// If framerate and/or aspect
				// ratio are ilegal, we discard the
				// whole sequence info.
				if (frame_rate>0 && frame_rate<9 &&
					aspect_ratio>0 && aspect_ratio<5)
				{
					printf ("\rNew video information found");
					if (pts_set==2)
					{
						unsigned cur_sec = (unsigned) ((current_pts - min_pts)
							/ MPEG_CLOCK_FREQ);
						printf ("at %02u:%02u\n",cur_sec/60, cur_sec % 60);
					}
					printf ("\n");
					printf ("[%u * %u] [AR: %s] [FR: %s] [Bitrate: %u bytes/sec]\n",
						hor_size,vert_size,	aspect_ratio_types[aspect_ratio],
						framerates_types[frame_rate], bit_rate);
					current_hor_size=hor_size;
					current_vert_size=vert_size;
					current_aspect_ratio=aspect_ratio;
					current_frame_rate=frame_rate;
					current_bit_rate=bit_rate;
				}
				//header=orig_header+1;
			}
			header++;
			continue;
		}
#endif
        else
        {
        	header++;
			continue; /* Keep looking */
        }

	} // for (;;)
	return (LONG) (header - data);
}

void general_loop(void)
{
	LONG overlap=0;
	LONG pos = 0; /* Current position in buffer */
	// int ts_blocks=0;
	end_of_file = 0;
	current_picture_coding_type = 0;
	p_caption_size = 0, p_caption_capacity = 0;
	p_caption = NULL;

	while (!end_of_file && !processed_enough)
	{
		LONG i,got;
		/* Get rid of the bytes we already processed */
		overlap=inbuf-pos;
		memmove (fbuffer,fbuffer+pos,(size_t) (inbuf-pos));
		inbuf-=pos;

		pos = 0;

		// GET MORE DATA IN BUFFER
		if (ts_mode)
			i = ts_getmoredata();
		else
			i = general_getmoredata();

		if (clean!=NULL)
			fwrite (fbuffer+overlap,1,(size_t) (inbuf-overlap),clean);

		if (i==0)
		{
			end_of_file = 1;
			memset (fbuffer+inbuf, 0, (size_t) (BUFSIZE-inbuf)); /* Clear buffer at the end */
		}

		if (inbuf == 0)
		{
			/* Done: Get outta here */
			break;
		}


		got = process_block (fbuffer, inbuf);
		if (got>inbuf)
		{
			printf ("BUG BUG\n");
		}
		pos+=got;

		if (inputsize>0)
		{
			int progress = (int) (((past>>8)*100)/(inputsize>>8));
			if (last_reported_progress != progress)
			{
				int cur_sec;
				printf ("\r%3d%%",progress);
				cur_sec = (int) ((c1count + c1count_total) / 29.97);
				printf ("  |  %02d:%02d", cur_sec/60, cur_sec%60);
				last_reported_progress = progress;
			}
		}
	}
}
