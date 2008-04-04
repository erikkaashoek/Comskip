/*
 * mpeg2dec.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include<excpt.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#endif
#ifdef LIBVO_SDL
#include <SDL/SDL.h>
#endif
#include <inttypes.h>

#include "config.h"

#include "mpeg2.h"
#include "AC3Dec/ac3.h"

//#include "mpeg2convert.h"
#include "comskip.h"

extern int coding_type;

#ifdef _WIN32
void gettimeofday (struct timeval * tp, void * dummy);
#endif

static int buffer_size = 409600;
static FILE * in_file;
static FILE * sample_file;
static FILE * timing_file = 0;
static uint8_t *	buffer; // = (uint8_t *) malloc (buffer_size);


static int video_track = 0xe0;  // changed to true
static int current_audio_track = -1;	//
static int audio_ac3_track = 0x80;	//
int is_AC3;
int AC3_rate;
int AC3_mode;
extern int AC3_byterate;
int demux_pid=0;
int demux_asf=0;
int last_pid;
#define PIDS	100
#define PID_MASK	0x1fff
int pids[PIDS];
int pid_type[PIDS];
int pid_pcr[PIDS];
int pid_pid[PIDS];
int top_pid_count[PID_MASK+1];
int top_pid_pid;
int pid;
int selected_video_pid=0;
int selected_audio_pid=0;
int found_pids=0;

#define uint64_t unsigned __int64
__int64 new_pts,pts, dts;
__int64 apts, adts,initial_apts;
static int initial_apts_set = 0;
__int64 initial_pts;
int initial_pts_set = 0;

int bitrate,muxrate,byterate=10000;
//#define PTS_FRAME 3004
#define PTS_FRAME (int) (90000 / get_fps())
#define SAMPLE_TO_FRAME 2.8125
//#define SAMPLE_TO_FRAME (90000.0/(get_fps() * 1000.0))

//#define BYTERATE	((int)(21400 * 25 / get_fps()))

#define   FSEEK    _fseeki64
#define   FTELL    _ftelli64
// The following two functions are undocumented and not included in any public header,
// so we need to declare them ourselves
extern int  _fseeki64(FILE *, __int64, int);
extern __int64 _ftelli64(FILE *);

int soft_seeking=0;
extern char	basename[];
static char tempstring[512];
char field_t;
#define DUMP_OPEN if (output_timing) { sprintf(tempstring, "%s.timing.csv", basename); timing_file = fopen(tempstring, "w"); DUMP_HEADER }
#define DUMP_HEADER if (timing_file) fprintf(timing_file, \
"type      ,pos       ,framenum  ,framen_inf,exp_frame ,PTS frame ,PTS       ,DTS       ,APTS      ,oframe-PTS,initialPTS,field\n");
#define DUMP_TIMING(T, P, D, A)  \
if (timing_file && !csStepping && !csJumping && !csStartJump) \
	fprintf(timing_file, "%s,%10u,%10u,%10u,%10u,%10u,%10u,%10u,%10u,%10u,%6.6f, %c \n", T, (int)headerpos, \
 framenum, framenum_infer, expected_frame_count, (int)((pts-initial_pts)/PTS_FRAME) , (int) ((P)/PTS_FRAME), \
(int) ((D)/PTS_FRAME),(int)((A)/PTS_FRAME), framenum - (uint32_t)(pts-initial_pts)/PTS_FRAME, \
(double)(initial_pts/90000), field_t); 
#define DUMP_CLOSE if (timing_file) { fclose(timing_file); timing_file = NULL; }


extern int skip_B_frames;
static mpeg2dec_t * mpeg2dec;
static int sigint = 0;
static int total_offset = 0;
static int verbose = 0;

#ifdef _DEBUG
static int dump_seek = 1;		// Set to 1 to dump the seeking process
#else
static int dump_seek = 0;		// Set to 1 to dump the seeking process
#endif

#include <sys/stat.h>
//#include <unistd.h>


struct stat instat;
#define filesize instat.st_size

extern int frame_count;
int						framenum;

fpos_t		filepos;
fpos_t		fileendpos;
extern int		standoff;
__int64			goppos,infopos,packpos,ptspos,headerpos,frompos,SeekPos;

extern int max_repair_size;
extern int variable_bitrate;
int max_internal_repair_size = 40;
int framenum_infer;
int expected_frame_count = 0;
int reviewing = 0;
int count=0;
int currentSecond=0;
int cur_hour = 0;
int cur_minute = 0;
int cur_second = 0;

extern char HomeDir[256];
int processCC=false;
//int live_tv;
int csRestart;
int csStartJump;
int csStepping;
int csJumping;
int csFound;
int	seekIter = 0;
int	seekDirection = 0;

extern FILE * out_file;
extern uint8_t ccData[500];
extern int ccDataLen;
extern int height,width,framenum;
extern int output_debugwindow;
extern int output_timing;

extern unsigned char *frame_ptr;
extern int lastFrameWasSceneChange;
extern int live_tv_retries;
static unsigned int frame_period;
extern void set_fps(unsigned int frame_period);
extern void dump_video (char *start, char *end);
extern void dump_audio (char *start, char *end);
extern void	Debug(int level, char* fmt, ...);
extern void dump_video_start(void);
extern void dump_audio_start(void);


int DetectCommercials(int);
int BuildMasterCommList(void);
FILE* LoadSettings(int argc, char ** argv);
void ProcessCCData(void);



void dump_state (FILE * f, mpeg2_state_t state, const mpeg2_info_t * info,
		 int offset, int verbose);


static void signal_handler (int sig)
{
    sigint = 1;
    signal (sig, SIG_DFL);
    //return (RETSIGTYPE)0;
	return;
}



#define AUDIOBUFFER	40000

static int audio_buffer_size = AUDIOBUFFER;
static short audio_buffer[AUDIOBUFFER];
static short *audio_buffer_ptr = audio_buffer;
static int audio_samples = 0;
static short *sum_ptr;

static int sound_frame_counter = 0;
extern double get_fps();
extern int get_samplerate();
extern int get_channels();
extern void add_volumes(int *volumes, int nr_frames);
extern void set_frame_volume(uint32_t framenr, int volume);

static int max_volume_found = 0;

int ms_audio_delay = 5;
int frames_without_sound = 0;
int frames_with_loud_sound = 0;

void sound_to_frames(short *buffer, int samples)
{
static __int64 expected_apts = 0;
static int old_is_AC3 = -2;
	
	int i;
	int volume;
	int delta = 0;
	int s_per_frame;

	if (!samples)
		return;
	if (old_is_AC3 != is_AC3) {
		if (is_AC3)
			Debug(6, "AC3 sound at %d khz\n", sampling_rate/1000);
		else
			Debug(6, "MPEG2 sound at %d khz\n", get_samplerate()/1000);
		old_is_AC3 = is_AC3;
	}

	if (is_AC3) {
		s_per_frame = (int) ((double)sampling_rate * (double)2 / get_fps());
	} else {
		s_per_frame = (int) ((double)get_samplerate() * (double) get_channels() / get_fps());
	}
//	s_per_frame = (int) (s_per_frame * 1.0 / 3.0);
	if (s_per_frame == 0)
		return;
	audio_samples += samples;
	buffer = audio_buffer;
	while (audio_samples >= s_per_frame)
	{
//		if (sample_file) fprintf(sample_file, "Frame %i\n", sound_frame_counter); 
		volume = 0;
		for (i = 0; i < s_per_frame; i++)
		{
			if (sample_file) fprintf(sample_file, "%i\n", *buffer); 
			volume += (*buffer>0 ? *buffer : - *buffer);
			buffer++;
		}
		volume = volume/s_per_frame;
		audio_samples -= s_per_frame;

		if (max_volume_found < volume)
			max_volume_found = volume;
		
		if (sound_frame_counter == 44)
			sound_frame_counter = sound_frame_counter;
		
		if (volume == 0)
			frames_without_sound++;
		if (volume > 10000) {
			frames_with_loud_sound++;
			volume = -1;
		}
		
		delta = (__int64)(apts/PTS_FRAME) - (__int64)(expected_apts/PTS_FRAME);
		if (expected_apts != 0 && delta != 0 && ! demux_asf) {
			if (delta < max_repair_size && delta > 0) {
				Debug(1, "Audio PTS jumped %d frames at frame %d, repairing timeline\n", delta, sound_frame_counter);
				while (delta-- > 0) {
					set_frame_volume((demux_asf?ms_audio_delay:0)+sound_frame_counter++, volume);
					DUMP_TIMING("a fill    ", (__int64)0,(__int64)0,apts);
				}									
			} else {
				Debug(1, "Audio PTS jumped %d frames at frame %d, audio may get out of sync\n", delta, sound_frame_counter);
				DUMP_TIMING("a skip    ", (__int64)0,(__int64)0,apts);
			}
		}
		
		DUMP_TIMING("a         ", (__int64)0,(__int64)0,apts);


		delta = (__int64)(apts/PTS_FRAME) - (__int64)(pts/PTS_FRAME);
		if (-max_internal_repair_size < delta && delta < max_internal_repair_size && sound_frame_counter != delta + framenum) {
			sound_frame_counter = delta + framenum;
		}
		set_frame_volume((demux_asf?ms_audio_delay:0)+sound_frame_counter++, volume);

		apts += PTS_FRAME;
		expected_apts = apts;
	}
	audio_buffer_ptr = audio_buffer;
	if (audio_samples > 0)
	{
		for (i = 0; i < audio_samples; i++)
		{
				*audio_buffer_ptr++ = *buffer++;
		}		
	}
}

#define STORAGE_SIZE 100000

static char storage_buf[STORAGE_SIZE];
static char *write_buf = storage_buf;
static char *read_buf = storage_buf;


void decode_audio (char *input_buf, int bytes)
{
static int min_buffer = 1800;
	int done_bytes, avail_bytes;
	int samples=0;
	int tries = 4;
	int size;
	short *sum_ptr;
	int sum;

	if (bytes <= 0)
		return;
	if (input_buf[0] == 0x0b && input_buf[1] == 0x77)
		is_AC3 = 1;
#ifdef SINGLE_BUFFERING
	samples = 0;
	if (is_AC3) {
		done_bytes = ac3_decode_data(input_buf, bytes, &samples , audio_buffer_ptr);		\
	} else {
		done_bytes = decode_frame(0, audio_buffer_ptr, &samples, input_buf, bytes);
	}
	samples = samples / 2;
	sound_to_frames(audio_buffer_ptr, samples);

#else
	if (&write_buf[bytes] - storage_buf > STORAGE_SIZE)
			printf("Panic, unknown audio format\n");
	else {
		if (&write_buf[bytes] - storage_buf > STORAGE_SIZE*2/3)
			is_AC3 = ! is_AC3;
		memcpy(write_buf, input_buf, bytes);
		write_buf = &write_buf[bytes];
	}
	avail_bytes = write_buf - read_buf;

	if (frames_with_loud_sound > 50 && !is_AC3) {
		frames_with_loud_sound = 0;
		is_AC3 = 1;
	}
	done_bytes = 1;
need_more:
	while ( avail_bytes > min_buffer && done_bytes) {
		samples = 0;
		if (is_AC3) {
try_AC3:
			done_bytes = ac3_decode_data(read_buf, write_buf - read_buf, &samples , audio_buffer_ptr);		\
//			done_bytes = write_buf - read_buf;
/*
			if (samples == 0) {
					if (tries == 0) {
						min_buffer += 400;
						goto need_more;
					}
					is_AC3 = 0;
					if (tries-- > 0) goto try_MPEG;
				}
*/
		} else {
try_MPEG:
		done_bytes = decode_frame(0, audio_buffer_ptr, &samples, read_buf, (int)(write_buf - read_buf));
				if (samples == 0 && min_buffer < 5000) {
//					if (tries == 0) {
						min_buffer += 400;
						goto need_more;
//					}
//					is_AC3 = 1;
//					if (tries-- > 0) goto try_AC3;
				}

		}
skip:
		read_buf = &read_buf[done_bytes];
		avail_bytes -= done_bytes;
		samples = samples / 2;
		sound_to_frames(audio_buffer_ptr, samples);
	}
	bytes = 0;
	while (read_buf < write_buf) {
		storage_buf[bytes++] = *read_buf++;
	}
	write_buf = &storage_buf[bytes];
	read_buf = storage_buf;
#endif
}

void decode_mpeg2_audio_reset()
{
	int del = write_buf - read_buf;
//	read_buf = write_buf = storage_buf;
}



void decode_mpeg2_audio(char *buf, char *end)
{
	if (!reviewing)
	{
		if (!csStepping && !csJumping && !csStartJump) {
			dump_audio(buf, end);
			decode_audio(buf, end - buf);
			
		}
	}
}

static void print_fps (int final)
{
static uint32_t frame_counter = 0;
static struct timeval tv_beg, tv_start;
static int total_elapsed;
static int last_count = 0;
    struct timeval tv_end;
    double fps, tfps;
    int frames, elapsed;
	char cur_pos[10] = "0:00:00";

    if (verbose)
		return;
	
	if(csStepping)
		return;

	if(final < 0) {
		frame_counter = 0;
		last_count = 0;
		return;
	}

    gettimeofday (&tv_end, NULL);

    if (!frame_counter) {
		tv_start = tv_beg = tv_end;
		signal (SIGINT, signal_handler);
    }

    elapsed = (tv_end.tv_sec - tv_beg.tv_sec) * 100 + (tv_end.tv_usec - tv_beg.tv_usec) / 10000;
    total_elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 100 + (tv_end.tv_usec - tv_start.tv_usec) / 10000;

    if (final) {
		if (total_elapsed)
			tfps = frame_counter * 100.0 / total_elapsed;
		else
			tfps = 0;

		fprintf (stderr,"\n%d frames decoded in %.2f seconds (%.2f fps)\n",
			 frame_counter, total_elapsed / 100.0, tfps);

		return;
    }

    frame_counter++;

    if (elapsed < 100)	/* only display every 1.00 seconds */
	return;

    tv_beg = tv_end;
    frames = frame_counter - last_count;
	
	sprintf(cur_pos, "%2i:%.2i:%.2i", cur_hour, cur_minute, cur_second);

    fps = frames * 100.0 / elapsed;
    tfps = frame_counter * 100.0 / total_elapsed;

    fprintf (stderr, "%s - %d frames in %.2f sec (%.2f fps), "
	     "%.2f sec (%.2f fps), %d%%\r", cur_pos, frame_counter,
	     total_elapsed / 100.0, tfps, elapsed / 100.0, fps, (int) (100 * headerpos / fileendpos));

    last_count = frame_counter;
}


void asf_tag_picture(__int64 new_pts)
{
//	int nf = (int)(new_pts/PTS_FRAME);

	DUMP_TIMING("v settag  ", new_pts,(__int64)0, (__int64)0);

	mpeg2_tag_picture (mpeg2dec, new_pts, new_pts);
}

static __int64 prev_new_pts = 0;
void set_pts(__int64 new_pts)
{
	int f,nf,i;
//	if (demux_asf)
//		mpeg2_tag_picture (mpeg2dec, new_pts, new_pts);

	if (!csStepping && !csJumping) {

		if (!initial_pts_set) {
			initial_pts = new_pts - (framenum) * PTS_FRAME;
			initial_pts_set = 1;
		}
	} else {
		if (!initial_pts_set) {
			initial_pts = new_pts;
			initial_pts_set = 1;
		}
	}			

	
	if ( prev_new_pts/PTS_FRAME !=  new_pts/PTS_FRAME) {
		prev_new_pts = new_pts;
		f = (int)(pts/PTS_FRAME);
		nf = (int)(new_pts/PTS_FRAME);
/*
		Debug(10, "PTS delta %d\n", nf - f);
*/		
		if (csStepping || csJumping) {
			if ((nf > f + 1 || nf < f) || csJumping) {
				pts = new_pts;
				if (pts > initial_pts)
					framenum_infer = (int)((pts-initial_pts)/PTS_FRAME  + 0.2);
				else
					framenum_infer = 0;
				i = framenum_infer - (int)(headerpos/byterate);
				if (abs(i) > 1000 && ! variable_bitrate) {
					framenum_infer = (int)(headerpos/byterate);
					soft_seeking=1;
				}
				if (dump_seek)  printf("[%6d] set_pts, headerpos = %6d\n",framenum_infer, (int)(headerpos/byterate));
			}
			return;
		}				
		
		if (max_repair_size == 0) {
			if (f < nf || 
				f > nf + max_internal_repair_size ) {
				pts = new_pts;
			}
			return;
		}
		if (f < nf || 
			f > nf + max_internal_repair_size ) {
			pts = new_pts;

			framenum_infer = (int)((pts-initial_pts)/PTS_FRAME  + 0.2);
//			if (framenum_infer < 1)
//				framenum_infer = 1;
			
			if (abs( framenum_infer - framenum) > max_repair_size) {
				// Oops, probably the initial_pts is wrong!!!!!
				framenum_infer = framenum_infer;
				if (!skip_B_frames)
					Debug(1, "Video PTS jumped %d frames at frame %d, frame numbers may be incorrect\n", framenum_infer - framenum, framenum);
				initial_pts = pts - (framenum) * PTS_FRAME;
				framenum_infer = (int)((pts-initial_pts)/PTS_FRAME  + 0.2);
		//		if (framenum_infer < 0)
		//			framenum_infer = 0;
			}

			DUMP_TIMING("change pts", new_pts,(__int64)0, (__int64)0);

		}
	}

}

__int64 prev_new_apts = 0;
void set_apts(__int64 new_apts)
{
	DUMP_TIMING("a settag  ", (__int64)0,(__int64)0,new_apts);
	if (((int)prev_new_apts/PTS_FRAME) != ((int)new_apts/PTS_FRAME)) {
		prev_new_apts = new_apts;

		if ((int)(apts/PTS_FRAME) < (int)(new_apts/PTS_FRAME) || 
			(int)(apts/PTS_FRAME) > (int)(new_apts/PTS_FRAME) + max_internal_repair_size ) {
			apts = new_apts;
		}
	}

	if (!initial_apts_set) {
		initial_apts = new_apts;
		initial_apts_set = 1;
//		Debug(1, "Initial Audio Frame time is %d\n", (int)(initial_apts / PTS_FRAME)); 
	}
}


static void * malloc_hook (unsigned size, mpeg2_alloc_t reason)
{
	static int prev_framenum=0;
    void * buf;

    /*
     * Invalid streams can refer to fbufs that have not been
     * initialized yet. For example the stream could start with a
     * picture type onther than I. Or it could have a B picture before
     * it gets two reference frames. Or, some slices could be missing.
     *
     * Consequently, the output depends on the content 2 output
     * buffers have when the sequence begins. In release builds, this
     * does not matter (garbage in, garbage out), but in test code, we
     * always zero all our output buffers to:
     * - make our test produce deterministic outputs
     * - hint checkergcc that it is fine to read from all our output
     *   buffers at any time
     */
    if ((int)reason < 0) {
        return NULL;
    }
    buf = mpeg2_malloc (size, (mpeg2_alloc_t)-1);
    if (buf && (reason == MPEG2_ALLOC_YUV || reason == MPEG2_ALLOC_CONVERTED))
        memset (buf, 0, size);
    return buf;
}


char field_t;
void SetField(char t)
{
	field_t = t;
//	printf(" %c", t);
}

static int is32 = 0;

int SubmitFrame(int frame_count)
{	
	static int last_back_frame=0;
	int res=0;
	int missing_frames;
	if (is32 >= 2) {
		DUMP_TIMING("v32       ", pts,(__int64)0,apts);
		if (!reviewing)
			res = DetectCommercials(framenum); // 3/2 pulldown
		framenum++;
		print_fps (0);
		pts += PTS_FRAME;
		framenum_infer++;
		expected_frame_count++;
		is32 -= 2;
//		Debug(1, "Video pulldown\n");
	}
	if (expected_frame_count != 0 && framenum_infer  != expected_frame_count /* && ! demux_asf */ ) {
		missing_frames = framenum_infer - expected_frame_count;
		if (missing_frames != 0)
			missing_frames = missing_frames;
		if (0 < missing_frames && missing_frames < max_repair_size) {
			if (!skip_B_frames)
				Debug(1, "Video PTS jumped %d frames at frame %d, repairing timeline\n", missing_frames, framenum);
			while (missing_frames-- > 0 && res == 0) {
				DUMP_TIMING("v fill    ",pts,(__int64)0,apts);
				if (!reviewing) {
					res = DetectCommercials(framenum);
				}
				print_fps (0);
				framenum++;
//				pts += PTS_FRAME;
//				framenum_infer++;
			}
		} 
//		else 
//			Debug(1, "Video PTS jumped %d frames at frame %d, frame numbers may be incorrect\n", missing_frames, framenum);
	}
	if (framenum_infer < expected_frame_count) {
		DUMP_TIMING("v skip    ", pts,(__int64)0,apts);
		if (last_back_frame != framenum) {
			if (!skip_B_frames)
				Debug(1, "Video PTS jumped %d frames at frame %d, repairing timeline\n", framenum_infer - expected_frame_count,framenum);
			last_back_frame = framenum;
		}
		pts += PTS_FRAME;
		framenum_infer++;
		expected_frame_count = framenum;
	} else { 
		DUMP_TIMING("v         ", pts,(__int64)0,apts);
		if (!reviewing)
			if (res == 0) 
				res = DetectCommercials((int)framenum);
		framenum++;
		pts += PTS_FRAME;
		framenum_infer++;
		expected_frame_count = framenum_infer;
	}
	return (res);
}


void decode_mpeg2 (uint8_t * current, uint8_t * end)
{
	static int c = 0;

    const mpeg2_info_t * info;
    const mpeg2_picture_t * pic;
	mpeg2_state_t state;

	if (csJumping)
		return;
	if (!reviewing)
		dump_video(current, end);
    mpeg2_buffer (mpeg2dec, current, end);
    total_offset += (int)(end - current);

    info = mpeg2_info (mpeg2dec);

    while (1) {
		state = mpeg2_parse (mpeg2dec);

		if (verbose)
			dump_state (stderr, state, info, total_offset - mpeg2_getpos (mpeg2dec), verbose);
		
		switch (state) {

		case STATE_BUFFER:
			return;

		case STATE_SEQUENCE:

			frame_period = info->sequence->frame_period;
			if (frame_period == 450450 || frame_period == 900900 || frame_period == 900000 || frame_period == 1080000 || frame_period == 540000) {
				if (height != info->sequence->height && info->sequence->height > 100 && info->sequence->height < 1200)
				height= info->sequence->height;
				if (width != info->sequence->width && info->sequence->width > 100 && info->sequence->width  < 2000)
				width= info->sequence->width;
				bitrate = info->sequence->byte_rate*8;
			}
			if (width > 2000 || height > 1200 || frame_period == 0 || bitrate == 0) {
//				printf("Panic: illegal height, width or frame period\n");
				return; // exit(2);
			}
			set_fps(frame_period);
			infopos = headerpos;
			/* might set nb fbuf, convert format, stride */
			/* might set fbufs */

			break;

		case STATE_SEQUENCE_REPEATED:
			frame_period = info->sequence->frame_period;
			break;

		case STATE_SLICE:
		case STATE_END:
		case STATE_INVALID_END:
			/* draw current picture */
			/* might free frame buffer */			


			if (info->display_fbuf) {
				frame_ptr = info->display_fbuf->buf[0];
  
				if (info->gop) {
					cur_hour = info->gop->hours;
					cur_minute = info->gop->minutes;
					cur_second = info->gop->seconds;
					goppos = packpos;
				}


				pic = info->current_picture;
				if (pic != NULL) {
					if (pic->nb_fields == 3)
						is32++;
					if (pic->flags & PIC_FLAG_TAGS) {
						new_pts = pic->tag;
						set_pts(new_pts);
					}

					
					if ((pic->flags & 7) == PIC_FLAG_CODING_TYPE_P)
						SetField('P');
					if ((pic->flags & 7) == PIC_FLAG_CODING_TYPE_B) 
						SetField('B');
					if ((pic->flags & 7) == PIC_FLAG_CODING_TYPE_I)
						SetField('I');
					if ((pic->flags & 7) == PIC_FLAG_CODING_TYPE_D)
						SetField('D');
/*
					if (pic->flags & PIC_FLAG_PROGRESSIVE_FRAME)
						printf (" PROG");
					if (pic->flags & PIC_FLAG_SKIP)
						printf (" SKIP");
					printf (" fields %d", pic->nb_fields);
					if (pic->flags & PIC_FLAG_TOP_FIELD_FIRST)
						printf (" TFF");
					if (pic->flags & PIC_FLAG_TAGS)
						printf (" pts %08x dts %08x", pic->tag, pic->tag2);
					printf("\n");
 */
					
				}
				print_fps (0);

				if (csStepping) {
					if (dump_seek)  printf("[%6d] Stepping to %d, headerpos=%6d\n",framenum_infer, (int)SeekPos, (int)(headerpos/byterate));
					if (framenum_infer >= SeekPos) {
//						SubmitFrame((int)framenum_infer);
//						vo_init(width, height, "Dump");
//						vo_draw(frame_ptr);
						framenum = framenum_infer;
						csFound = 1;
						return;
					} else 
						frame_ptr = NULL;

					pts += PTS_FRAME;
					framenum++;
					framenum_infer++;
				} else {
					if (SubmitFrame((int)framenum_infer)==1) {
						csRestart = 1;
						audio_samples = 0;
						print_fps(-1);
						return;
					}
				}
			} else {
				frame_ptr = NULL;
			}

			break;
		default:

			if ((processCC) && (info->user_data_len > 4) && (info->user_data_len < 100)) {
				int i;
				ccDataLen = (int)info->user_data_len;
				for (i=0; i<ccDataLen; i++) {
					ccData[i] = info->user_data[i];
				}
				ProcessCCData();
			}
			break;
		}
    }
}

#define DEMUX_PAYLOAD_START 1
int demux (uint8_t * buf, uint8_t * end, int flags)
{
    static int mpeg1_skip_table[16] = {
	0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /*
     * the demuxer keeps some state between calls:
     * if "state" = DEMUX_HEADER, then "head_buf" contains the first
     *     "bytes" bytes from some header.
     * if "state" == DEMUX_DATA, then we need to copy "bytes" bytes
     *     of ES data before the next header.
     * if "state" == DEMUX_SKIP, then we need to skip "bytes" bytes
     *     of data before the next header.
     *
     * NEEDBYTES makes sure we have the requested number of bytes for a
     * header. If we dont, it copies what we have into head_buf and returns,
     * so that when we come back with more data we finish decoding this header.
     *
     * DONEBYTES updates "buf" to point after the header we just parsed.
     */

#define DEMUX_HEADER 0
#define DEMUX_DATA 1
#define DEMUX_SKIP 2
    static int state = DEMUX_SKIP;
    static int state_bytes = 0;
    static int was_audio = 0;
    static uint8_t head_buf[264];
	static uint8_t * start_buf;
	__int64	new_apts;

    uint8_t * header;
    int bytes;
    int len=0;
	int samples;
	int found_audio_track;
	//	int ss;
	__int64 new_pts;

	start_buf = buf;
//Definitions of macros

#define NEEDBYTES(x)						\
    do {							\
		int missing;						\
									\
		missing = (x) - bytes;					\
		if (missing > 0) {					\
			if (header == head_buf) {				\
			if (missing <= end - buf) {			\
				memcpy (header + bytes, buf, missing);	\
				buf += missing;				\
				bytes = (x);				\
			} else {					\
				memcpy (header + bytes, buf, end - buf);	\
				state_bytes = bytes + (int)(end - buf);		\
				return 0;					\
			}						\
			} else {						\
			memcpy (head_buf, header, bytes);		\
			state = DEMUX_HEADER;				\
			state_bytes = bytes;				\
			return 0;					\
			}							\
		}							\
    } while (0)

#define DONEBYTES(x)		\
    do {			\
		if (header != head_buf)	\
			buf = header + (x);	\
    } while (0)

// End macro definitions

	

    if (flags & DEMUX_PAYLOAD_START)
		goto payload_start;

    switch (state) {

    case DEMUX_HEADER:
		if (state_bytes > 0) {
			header = head_buf;
			bytes = state_bytes;
			goto continue_header;
		}
		break;

    case DEMUX_DATA:
		if (demux_pid || (state_bytes > end - buf)) {
			if (was_audio)
				decode_mpeg2_audio(buf, end);
			else {
				decode_mpeg2 (buf, end);
				if (csFound || csStartJump) return 0;
			}
			state_bytes -= (int)(end - buf);
			return 0;
		}
		if (was_audio) {
			decode_mpeg2_audio(buf, buf + state_bytes);
		} else {
			decode_mpeg2 (buf, buf + state_bytes);
			if (csFound || csStartJump) return 0;
		}
		was_audio = 0;
		buf += state_bytes;
		break;

    case DEMUX_SKIP:
		if (demux_pid || (state_bytes > end - buf)) {
			state_bytes -= (int)(end - buf);
			return 0;
		}
		buf += state_bytes;
		break;
    }

    while (1) {
	if (demux_pid) {
	    state = DEMUX_SKIP;
	    return 0;
	} 		
payload_start:

		header = buf;
		bytes = (int)(end - buf);
		
continue_header:
		if (start_buf > buf ||  buf > end)
			return *(int *)0;
		NEEDBYTES (4);
		if (header[0] || header[1] || (header[2] != 1)) {
			if (demux_pid) {
				state = DEMUX_SKIP;
				return 0; 	
			} else if (header != head_buf) {
				buf++;
				goto payload_start;
			} else {
				header[0] = header[1];
				header[1] = header[2];
				header[2] = header[3];
				bytes = 3;
				goto continue_header;
			}
		}

		headerpos = filepos + (int)(buf - buffer);
//		headerpos = filepos - (int)(end - header);
	if (demux_pid) {
//	    if ((header[3] >= 0xe0) && (header[3] <= 0xef))
		goto pes_video;
//	    if ((header[3] >= 0xc0) && (header[3] <= 0xcf))
//		goto pes_audio;
	    fprintf (stderr, "bad stream id %x\n", header[3]);
	    return 0; //exit (1);
	} 
		switch (header[3]) {

		case 0xb9:	/* program end code */
			/* DONEBYTES (4); */
			/* break;         */
			return 1;

		case 0xba:	/* pack header */
			NEEDBYTES (5);
			if ((header[4] & 0xc0) == 0x40) {	/* mpeg2 */
			NEEDBYTES (14);

			muxrate = header[4+6] << 14;
			muxrate |= header[4+7] << 6;
			muxrate |= header[4+8] >> 2;
			muxrate *= 400;
			
			len = 14 + (header[13] & 7);
			NEEDBYTES (len);
			DONEBYTES (len);
			/* header points to the mpeg2 pack header */
			} else if ((header[4] & 0xf0) == 0x20) {	/* mpeg1 */
			NEEDBYTES (12);
			DONEBYTES (12);
			/* header points to the mpeg1 pack header */
			} else {
			fprintf (stderr, "weird pack header\n");
			DONEBYTES (5);
			}
			packpos = headerpos;
//			DUMP_TIMING("pack head ", 0, 0, 0);

			break;

		default:
			if (header[3] == video_track) {
	pes_video:
				NEEDBYTES (7);
				if ((header[6] & 0xc0) == 0x80) {	/* mpeg2 */
					NEEDBYTES (9);
					len = 9 + header[8];
					NEEDBYTES (len);
					/* header points to the mpeg2 pes header */
					if (header[7] & 0x80) {
						ptspos = headerpos;						
						new_pts = (((__int64)((header[9] >> 1) & 0x07) << 30) |
							(header[10] << 22) | ((header[11] >> 1) << 15) |
							(header[12] << 7) | (header[13] >> 1));
//						new_pts = 
							dts	= (!(header[7] & 0x40) ? new_pts :
						(((__int64)((header[14] >> 1)  & 0x07) << 30) |
							(header[15] << 22) |
							((header[16] >> 1) << 15) |
							(header[17] << 7) | (header[18] >> 1)));
						if (csJumping) 
							set_pts(new_pts);
//						if (csStepping || csJumping) set_pts(new_pts);
						DUMP_TIMING("settag    ", new_pts, dts, (__int64)0);
						mpeg2_tag_picture (mpeg2dec, new_pts, dts);
#define SEEKWINDOW	20
#define SEEKBEFORE	40
#define SEEKOFFSET  10

						if (csJumping) {
							if (framenum_infer < SeekPos - SEEKBEFORE) {
								seekIter++;
								seekDirection++;
/*
								if (seekIter > 8) {
									if (abs(seekDirection) < abs(seekIter)/2) 
										byterate = (int)(byterate * 0.95);
									else
										byterate = (int)(byterate * 1.05);
									seekIter -= 4;
								}
 */
								frompos += (abs(SeekPos - framenum_infer - SEEKBEFORE + SEEKWINDOW*2/3) - SEEKOFFSET) * (__int64) byterate;
								if (seekIter < 20)
								{
									if (dump_seek)  printf("[%6d] Jumping: Jump forward to %6i\n",framenum_infer, (int)(frompos/byterate));
									csStartJump = 1;
									csJumping = 0;
									return 0;
								}
							}
							if (SeekPos > SEEKBEFORE && framenum_infer > SeekPos - SEEKBEFORE + SEEKWINDOW) {
								seekIter++;
								seekDirection--;
/*
								if (seekIter > 8) {
									if (abs(seekDirection) < abs(seekIter)/2) 
										byterate = (int)(byterate * 0.95);
									else
										byterate = (int)(byterate * 1.05);
									seekIter -= 4;
								}
 */
								frompos -= (abs(SeekPos - framenum_infer  - SEEKBEFORE + SEEKWINDOW*2/3) + SEEKOFFSET) * (__int64)byterate;
								if (seekIter < 20)
								{
									if (dump_seek)  printf("[%6d] Jumping: Jump backward to %6i\n",framenum_infer, (int)(frompos/byterate));
									csStartJump = 1;
									csJumping = 0;
									return 0;
								}
							}
							csJumping = 0;
							csStepping = 1;
							if (dump_seek)  printf("[%6d] Jumping: To Stepping, headerpos=%6d\n",framenum_infer, (int)(frompos/byterate),(int)(headerpos/byterate));
						}
					}
				} else {	/* mpeg1 */
					int len_skip;
					uint8_t * ptsbuf;

					len = 7;
					while (header[len - 1] == 0xff) {
						len++;
						NEEDBYTES (len);
						if (len > 23) {
							fprintf (stderr, "too much stuffing\n");
							break;
						}
					}
					if ((header[len - 1] & 0xc0) == 0x40) {
					len += 2;
					NEEDBYTES (len);
					}
					len_skip = len;
					len += mpeg1_skip_table[header[len - 1] >> 4];
					NEEDBYTES (len);
					/* header points to the mpeg1 pes header */
					ptsbuf = header + len_skip;
					if ((ptsbuf[-1] & 0xe0) == 0x20) {

					pts = (((__int64)((ptsbuf[-1] >> 1)  & 0x07)<< 30) |
						   (ptsbuf[0] << 22) | ((ptsbuf[1] >> 1) << 15) |
						   (ptsbuf[2] << 7) | (ptsbuf[3] >> 1));
//					pts = 
					dts = (((ptsbuf[-1] & 0xf0) != 0x30) ? pts :
						   (((__int64)((ptsbuf[4] >> 1) & 0x07) << 30) |
						(ptsbuf[5] << 22) | ((ptsbuf[6] >> 1) << 15) |
						(ptsbuf[7] << 7) | (ptsbuf[18] >> 1)));
						DUMP_TIMING("pv1       ", pts, dts, (__int64)0);
						mpeg2_tag_picture (mpeg2dec, pts, dts);
					}
				}
				DONEBYTES (len);
				bytes = 6 + (header[4] << 8) + header[5] - len;
				if (!reviewing)
					dump_video_start();
				if (demux_pid || (bytes > end - buf)) {
					decode_mpeg2 (buf, end);
					if (csFound || csStartJump) return 0;
					state = DEMUX_DATA;
					state_bytes = bytes - (int)(end - buf);
					return 0;
				} else if (bytes > 0) {
					decode_mpeg2 (buf, buf + bytes);
					if (csFound || csStartJump) return 0;
					buf += bytes;
				}
			} else if ((header[3] & 0xc0) == 0xc0 || header[3] == 0xbd) {
pes_audio:
			is_AC3 = (header[3] == 0xbd);
			NEEDBYTES (7);
			if ((header[6] & 0xc0) == 0x80) {	/* mpeg2 */
				NEEDBYTES (9);
				len = 9 + header[8];
				NEEDBYTES (len);
				/* header points to the mpeg2 pes header */
				if (header[7] & 0x80) {
					
					new_apts = (((__int64)((header[9] >> 1)  & 0x07) << 30) |
						(header[10] << 22) | ((header[11] >> 1) << 15) |
						(header[12] << 7) | (header[13] >> 1));
					new_apts = 
						adts = (!(header[7] & 0x40) ? new_apts :
					(((__int64)((header[14] >> 1) & 0x07) << 30) |
						(header[15] << 22) |
						((header[16] >> 1) << 15) |
						(header[17] << 7) | (header[18] >> 1)));
					new_apts -= (int)(audio_samples * SAMPLE_TO_FRAME);
					set_apts(new_apts);
				}
			}
			DONEBYTES (len);
			bytes = 6 + (header[4] << 8) + header[5] - len;
      found_audio_track = -1;
			if (is_AC3 && buf[0] >= 0x80 && buf[0] < 0xbf) {
				found_audio_track = buf[0] - 0x80;
				if (current_audio_track == -1) {
					current_audio_track = found_audio_track;
					Debug(6, "Autoselected audio track %d\n", current_audio_track);
				}
				DONEBYTES(len+4);
				bytes -= 4;
				if (current_audio_track != found_audio_track) {
					if (frames_without_sound > 50) {
						frames_without_sound = 0;
						current_audio_track = found_audio_track; // No sound, try another track
						Debug(6, "Switched to audio track %d because no sound\n", found_audio_track );
					}
				}
			}
			if (bytes > buffer_size)
				return 0;
			if (bytes > (int)end - (int)buf) {
				if (current_audio_track == found_audio_track)
  				decode_mpeg2_audio(buf, end);
				state = DEMUX_DATA;
				state_bytes = bytes - (int)(end - buf);
				was_audio = 1;
				return 0;
			} else if (bytes > 0) {
				if (current_audio_track == found_audio_track)
  				decode_mpeg2_audio(buf, buf + bytes);
				buf += bytes;
			}
			} else if (header[3] < 0xb9) {
				DONEBYTES (4);
				header = buf;
				bytes = (int)(end - buf);
				goto continue_header;
				fprintf (stderr,"looks like a video stream, not system stream\n");
			} else {
				NEEDBYTES (6);
				DONEBYTES (6);
				bytes = (header[4] << 8) + header[5];
//				DUMP_TIMING("skip      ", bytes, 0, 0);
				if (bytes > end - buf) {
					state = DEMUX_SKIP;
					state_bytes = bytes - (int)(end - buf);
					return 0;
				}
				buf += bytes;
			}
		}
    }
  return 1;
}

#define DEMUX_PAYLOAD_START 1
static int demux_audio (uint8_t * buf, uint8_t * end, int flags)
{
    static int mpeg1_skip_table[16] = {
	0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /*
     * the demuxer keeps some state between calls:
     * if "state" = DEMUX_HEADER, then "head_buf" contains the first
     *     "bytes" bytes from some header.
     * if "state" == DEMUX_DATA, then we need to copy "bytes" bytes
     *     of ES data before the next header.
     * if "state" == DEMUX_SKIP, then we need to skip "bytes" bytes
     *     of data before the next header.
     *
     * NEEDBYTES makes sure we have the requested number of bytes for a
     * header. If we dont, it copies what we have into head_buf and returns,
     * so that when we come back with more data we finish decoding this header.
     *
     * DONEBYTES updates "buf" to point after the header we just parsed.
     */

#define DEMUX_HEADER 0
#define DEMUX_DATA 1
#define DEMUX_SKIP 2
    static int state = DEMUX_SKIP;
    static int state_bytes = 0;
    static int was_audio = 0;
    static uint8_t head_buf[264];
	static uint8_t * start_buf;
    uint8_t * header;
    int bytes;
    int len;
	int samples;
	int ss;
	__int64 new_apts;

	start_buf = buf;
//Definitions of macros

#undef NEEDBYTES
#define NEEDBYTES(x)						\
    do {							\
		int missing;						\
									\
		missing = (x) - bytes;					\
		if (missing > 0) {					\
			if (header == head_buf) {				\
				if (missing <= end - buf) {			\
					memcpy (header + bytes, buf, missing);	\
					buf += missing;				\
					bytes = (x);				\
				} else {					\
					memcpy (header + bytes, buf, end - buf);	\
					state_bytes = bytes + (int)(end - buf);		\
					return 0;					\
				}						\
			} else {						\
				memcpy (head_buf, header, bytes);		\
				state = DEMUX_HEADER;				\
				state_bytes = bytes;				\
				return 0;					\
			}							\
		}							\
    } while (0)

#undef DONEBYTES
#define DONEBYTES(x)		\
    do {			\
		if (header != head_buf)	\
			buf = header + (x);	\
    } while (0)

// End macro definitions

	

	if (flags & DEMUX_PAYLOAD_START) {
		goto payload_start;
	}
    switch (state) {
		
    case DEMUX_HEADER:
		if (state_bytes > 0) {
			header = head_buf;
			bytes = state_bytes;
			goto continue_header;
		}
		break;
		
    case DEMUX_DATA:
		decode_mpeg2_audio(buf, end);
		//		printf("Use %d bytes\n", (int)(end - buf));
		state_bytes -= (int)(end - buf);
		return 0;
		
    case DEMUX_SKIP:
		//		printf("Skip %d bytes\n", (int)(end - buf));
		state_bytes -= (int)(end - buf);
		return 0;
    }
	
    while (1) {
		if (demux_pid) {
			state = DEMUX_SKIP;
			return 0;
		} 		
payload_start:
		
		header = buf;
		bytes = (int)(end - buf);
		
continue_header:
		if (start_buf > buf ||  buf > end)
			return *(int *)0;
		NEEDBYTES (4);
		if (header[0] || header[1] || (header[2] != 1)) {
			if (demux_pid) {
				state = DEMUX_SKIP;
				return 0; 	
			} else if (header != head_buf) {
				buf++;
				goto payload_start;
			} else {
				header[0] = header[1];
				header[1] = header[2];
				header[2] = header[3];
				bytes = 3;
				goto continue_header;
			}
		}
		//		printf("Found MPEG header\n");
		
		headerpos = filepos + (int)(buf - buffer);
		is_AC3 = (header[3] == 0xbd);
		NEEDBYTES (7);
		if ((header[6] & 0xc0) == 0x80) {	/* mpeg2 */
			NEEDBYTES (9);
			len = 9 + header[8];
			NEEDBYTES (len);
			/* header points to the mpeg2 pes header */
			if (header[7] & 0x80) {
				
				new_apts = (((__int64)((header[9] >> 1)  & 0x07) << 30) |
					(header[10] << 22) | ((header[11] >> 1) << 15) |
					(header[12] << 7) | (header[13] >> 1));
				new_apts = 
					adts = (!(header[7] & 0x40) ? new_apts :
				(((__int64)((header[14] >> 1)  & 0x07) << 30) |
					(header[15] << 22) |
					((header[16] >> 1) << 15) |
					(header[17] << 7) | (header[18] >> 1)));
				//					if (abs(new_apts-apts) > 5*PTS_FRAME)
				//						printf("APTS jumps %d frames\n",(new_apts-apts)/PTS_FRAME);
				new_apts -= (int)(audio_samples * SAMPLE_TO_FRAME);
				set_apts(new_apts);
			}
		}
		DONEBYTES (len);
		bytes = 6 + (header[4] << 8) + header[5] - len;
		//			if (bytes > buffer_size)
		//				return;
		dump_audio_start();
		if (bytes > end - buf) {
			samples = 0;
			decode_mpeg2_audio_reset();
			decode_mpeg2_audio(buf, end);
			state = DEMUX_DATA;
			//				printf("Used %d bytes\n", end - buf);
			state_bytes = bytes - (int)(end - buf);
			was_audio = 1;
			return 0;
		} else if (bytes > 0) {
			decode_mpeg2_audio_reset();
			decode_mpeg2_audio(buf, buf+bytes);
			//				printf("Used %d bytes\n", bytes);
			buf += bytes;
		}
	}
}


static void ResetInputFile()
{
	if (demux_asf) {
#ifdef USE_ASF
		ASF_Control(NULL, 1, 0.0); 
#endif
  }
	else {
		rewind(in_file);
	}
	mpeg2_close (mpeg2dec);
	mpeg2dec = mpeg2_init ();
	if (mpeg2dec == NULL)
		exit (2);
	framenum = 0;	
	framenum_infer = 0;
//	frame_count = 0;
	expected_frame_count = 0;
	sound_frame_counter = 0;
	initial_pts = 0;
	initial_pts_set = 0;
	initial_apts_set = 0;
	initial_apts = 0;
	audio_samples = 0;
#ifndef _DEBUG
	DUMP_CLOSE
	DUMP_OPEN
#endif
}


static void asf_loop(void)
{

#ifdef USE_ASF
	do {
		int demuxparm = 0;
		if(csRestart) {
			ResetInputFile();
			frompos = 0;
			csRestart = 0;
		}
		if (csStartJump) {
			csStepping = 0;
			csJumping = 1;
			csFound = 0;
			csStartJump = 0;
			ASF_Control(NULL, 1, ((double) frompos)/fileendpos); 
//			own_stream_Seek(in_file, frompos);
		}

		if (csJumping) {
			ASF_Control(NULL, 1, ((double) frompos)/fileendpos); 
		}
/*
			fgetpos(in_file, &filepos);
			end = buffer + fread (buffer, 1, buffer_size, in_file);
			if (end != buffer + buffer_size) {
				fsetpos(in_file, &filepos);
				Sleep(1000L);
				end = buffer + fread (buffer, 1, buffer_size, in_file);
			}
			filepos += buffer_size;
*/
more:
		if (!ASF_Demux()) break;	/* hit program_end_code */
//		if (!csJumping && !csFound) goto more;


#undef SEEKWINDOW
#define SEEKWINDOW	25
#undef SEEKBEFORE
#define SEEKBEFORE	50
		
//		mpeg2_tag_picture (mpeg2dec, pts, dts);
		if (csJumping) {
			if (framenum_infer < SeekPos - SEEKBEFORE) {
				seekIter++;
				seekDirection++;
				/*
				if (seekIter > 8) {
				if (abs(seekDirection) < abs(seekIter)/2) 
				byterate = (int)(byterate * 0.95);
				else
				byterate = (int)(byterate * 1.05);
				seekIter -= 4;
				}
				*/
				frompos += abs(SeekPos - framenum_infer - SEEKBEFORE + SEEKWINDOW*2/3) * (__int64) byterate / 2;
				if (seekIter > 20)
				{
					if (dump_seek) printf("Jumping: Jump forward from %6i to %6lli\n",framenum_infer,SeekPos);
					csStartJump = 1;
					csJumping = 0;
            return;
				}
			} else
			if (SeekPos > SEEKBEFORE && framenum_infer > SeekPos - SEEKBEFORE + SEEKWINDOW) {
				seekIter++;
				seekDirection--;
				/*
				if (seekIter > 8) {
				if (abs(seekDirection) < abs(seekIter)/2) 
				byterate = (int)(byterate * 0.95);
				else
				byterate = (int)(byterate * 1.05);
				seekIter -= 4;
				}
				*/
				frompos -= abs(SeekPos - framenum_infer  - SEEKBEFORE + SEEKWINDOW*2/3) * (__int64)byterate / 2;
				if (seekIter > 20)
				{
					if (dump_seek)  printf("Jumping: Jump backward from %6i to %6lli\n",framenum_infer,SeekPos);
					csStartJump = 1;
					csJumping = 0;
					return;
				}
			}
			else {
				csJumping = 0;
				csStepping = 1;
			}
		}
	} while (!sigint && !csFound);
#endif
}



static void ps_loop (void)
{
//    uint8_t *	buffer = (uint8_t *) malloc (buffer_size);
    uint8_t *	end;
	int retries = 0;
    buffer = (uint8_t *) malloc (buffer_size);

    if (buffer == NULL)
	exit (2);

	do {
		int demuxparm = 0;
		if(csRestart) {
			ResetInputFile();
			demuxparm = DEMUX_PAYLOAD_START;
			csRestart = 0;
		}
		if (csStartJump) {
			csStepping = 0;
			csJumping = 1;
			csFound = 0;
			csStartJump = 0;
			demuxparm = DEMUX_PAYLOAD_START;
			fsetpos(in_file, &frompos);
		}
		fgetpos(in_file, &filepos);
		retries = 0;
again:
		FSEEK(in_file, (__int64)0, SEEK_END);
		fileendpos = FTELL(in_file);
		fsetpos(in_file, &filepos);

		if (standoff > 0 && fileendpos < filepos + standoff) {
			if (retries < live_tv_retries/2) {
				Debug( 11,"Sleep due to standoff, retries = %d \n", retries);
				retries++;
				goto dosleep;
			} else {
				standoff = 0;
			}
		}
		end = buffer + fread (buffer, 1, buffer_size, in_file);
		if (end != buffer + buffer_size && retries++ < live_tv_retries) {
			Debug( 11,"%8i:Read only %d bytes, retries = %d \n",(int) filepos, end - buffer, retries);
			fsetpos(in_file, &filepos);
dosleep:
			Sleep(1000L);
			goto again;
			end = buffer + fread (buffer, 1, buffer_size, in_file);
		}
		headerpos = filepos;
//		filepos += buffer_size;
		if (demux (buffer, end, demuxparm))
			break;	/* hit program_end_code */
	} while (end == buffer + buffer_size && !sigint && !csFound);
	Debug( 11,"Read only %d bytes, stopping\n", end - buffer, retries);

    free (buffer);
}

int pnum[40];
int ppid[40];
int max_pid=0;
int first_pat = 1;
int first_pmt = 1;



void GetPAT(uint8_t * buf)
{
	int length,i,n;
	int first;
	int last;
	if (buf[1] != 0)
		return;
	length = (((buf[2] & 0x0F) << 8) | buf[3]);
	if ( ( (length - 9)%4) != 0)
		return;
	n = (length - 9)/4;
	Debug( 11,"%8i:PAT[0:%d]\n", (int)filepos,n-1);
	if ((buf[6] & 0x01) == 0) {
		return;
	}
	first = buf[7];
	last = buf[8];
	for (i=0; i<n; i++) {
		pnum[i] = (((buf[9+i*4]) << 8) | buf[9+i*4+1]);
		ppid[i] = ((buf[11+i*4] << 8) | buf[11+i*4+1]) & PID_MASK;
		Debug( 11,"%8i:num[%2d] = %4d, pid[%2d] = %4x\n", (int) filepos, i, pnum[i], i, ppid[i]);
		if (pnum[i] == 0)
			ppid[i] = 0;
	}
	max_pid = n;
	first_pat = 0;
}

#define MAX_PID	400
int next_ES = 0;
int ES_type[MAX_PID];
int ES_pid[MAX_PID];
int PMT_failed = 0;

void GetPMT(uint8_t * buf, int pid)
{
	int length,i,j,n;
	int info, type, tag, len;
	int doffset,maxoffset;
	int number, last;
	int pcr;
	if (buf[1] != 2) {
		PMT_failed++;
		return;
	}
	if ((buf[2] & 0xc0) != 0x80) {
		return;
	}

	if ((buf[2] & 0x0c) != 0) {
		PMT_failed++;
		return;
	}

	length = (((buf[2] & 0x0F) << 8) | buf[3]);
	n = (((buf[4]) << 8) | buf[5]);

	if ((buf[6] & 0x01) == 0) { // Check for next PMT
		return;
	}
	number = buf[7];
	last = buf[8];

	pcr = ((buf[9]<< 8) | buf[9+1]) & PID_MASK;
	info = (((buf[11] & 0x0F) << 8) | buf[11+1]);
	Debug( 11,"%8i:PCR pid = %4x\n", (int) filepos, pcr);
	
/*	if (info > 0) {
		tag = buf[12];
		doffset = buf[13]+13+2;
		while (doffset < length) {
			tag = buf[doffset];
			doffset = buf[doffset+1] + 2;
		}
	}
*/
	doffset = 13+info;
	maxoffset = length + 4 - 4; //CRC

	i = 0;
	Debug( 11,"%8i:PMT[%d:%d]:\n",(int) filepos, doffset, maxoffset );
	while (doffset < maxoffset) {
		ES_type[i] = buf[doffset];
		ES_pid[i] = ((buf[doffset+1] << 8) | buf[doffset+2]) & PID_MASK;
		Debug( 11,"%8i:ES[%2d] pid = %4x type = %3d pcr = %4x\n", (int) filepos, i, ES_pid[i], ES_type[i], pcr);
		doffset += 5 + (((buf[doffset+3] & 0x0F) << 8) | buf[doffset+4]);
		for (j = 0; j < last_pid; j++) {
			if (pids[j] == ES_pid[i])
				break;
		}
		if (j == last_pid && pids[j] != ES_pid[i]) {
			pids[last_pid] = ES_pid[i];
			pid_type[last_pid] = ES_type[i];
			pid_pcr[last_pid] = pcr;
			pid_pid[last_pid++] = pid;
			if (last_pid >= PIDS) {
				Debug(9,"Too many PID's, discarded\n");
				last_pid--;
			}

		}
		i++;
	}
	while (i<8)
	{
		ES_type[i] =0;
		ES_pid[i] = 0;
		i++;
	}
	first_pmt = 0;
	if (demux_pid != 1) {
		for (i = 0; i < last_pid; i++) {
			if (pids[i] == demux_pid) {
				if (selected_video_pid == 0) {
					Debug( 8,"%8i:Found video pid = %4x\n", (int) filepos, demux_pid);
					csRestart = 1;
					selected_video_pid = demux_pid;
				}
				pcr = pid_pcr[i];
				pid = pid_pid[i];
				for (i = 0; i < last_pid; i++) {
					if (pid_pid[i] == pid && pid_pcr[i] == pcr && (pid_type[i] == 3 || pid_type[i] == 4 || pid_type[i] == 6 || pid_type[i] == 129)) {
						if (selected_audio_pid == 0 && selected_audio_pid != pids[i]) {
							selected_audio_pid = pids[i];
							Debug( 1,"%8i: Auto selected audio pid = %4x\n", (int)filepos, selected_audio_pid);
							found_pids = 1;
							//							csRestart = 1;
							return;
						}
					}
				}
				
			}
		}
	}
}

static void ts_loop (void)
{
//    uint8_t * buffer = (uint8_t *) malloc (buffer_size);
    uint8_t * buf;
    uint8_t * nextbuf;
    uint8_t * data;
    uint8_t * end;
	int prev_pid=0;
	int i;
	int bad_sync=1;
	int cur_buffer_size;
	int audio_state;
	int video_state;
	int retries = 0;

//	found_pids=1;
write_buf = storage_buf;
read_buf = storage_buf;

	buffer = (uint8_t *) malloc (buffer_size);
    if (buffer == NULL || buffer_size < 188)
		exit (1);
    buf = buffer;
    do {

		
//		end = buf + fread (buf, 1, cur_buffer_size, in_file);


		int demuxparm = 0;

		
		
		if(csRestart) {
			ResetInputFile();
			demuxparm = DEMUX_PAYLOAD_START;
			csRestart = 0;
			buf = buffer; // Clear the input buffer when restarting!!!!!!!!
		}
		if (csStartJump) {
			csStepping = 0;
			csJumping = 1;
			csFound = 0;
			csStartJump = 0;
			demuxparm = DEMUX_PAYLOAD_START;
			fsetpos(in_file, &frompos);
			buffer_size = 4096; // Change to smaller read ahead buffer
			buf = buffer; // Clear the input buffer when seeking!!!!!!!!
		}

		cur_buffer_size = buffer + buffer_size - buf;
		fgetpos(in_file, &filepos);
		retries = 0;
again:

		FSEEK(in_file, (__int64)0, SEEK_END);
		fileendpos = FTELL(in_file);
		fsetpos(in_file, &filepos);

		if (standoff > 0 && fileendpos < filepos + standoff) {
			if (retries < live_tv_retries/2) {
				Debug( 11,"Sleep due to standoff, retries = %d \n", retries);
				retries++;
				goto dosleep;
			} else {
				standoff = 0;
			}
		}
		
		end = buf + fread (buf, 1, cur_buffer_size, in_file);
		if (end != buf + cur_buffer_size && retries++ < live_tv_retries) {
			Debug( 11,"%8i:Read only %d bytes, retries = %d \n", (int) filepos, end - buf, retries);
			fsetpos(in_file, &filepos);
dosleep:
			Sleep(1000L);
			goto again;
			end = buf + fread (buf, 1, cur_buffer_size, in_file);
		}
		headerpos = filepos;
//		filepos += cur_buffer_size;
		
		buf = buffer;
		for (; (nextbuf = buf + 188) <= end; buf = nextbuf) {
			if (csRestart) break;
			if (*buf != 0x47) {
//				if (!bad_sync)
//					fprintf (stderr, "bad sync byte\n");
				bad_sync=1;
				nextbuf = buf + 1;
				continue;
			}
			bad_sync=0;
			if (buf[1] & 0x80)	// Error bit
				continue;
			pid = ((buf[1] << 8) + buf[2]) & PID_MASK;
			if (pid == PID_MASK)
				continue;
			if (pid != prev_pid)
				Debug( 11,"%8i:pid = %4x\n",(int) filepos, pid);
			prev_pid = pid;

			
			if (pid == 0) {

				if (first_pat && !(buf[1] & 0x40))
					continue;
				// Skip if there is no payload.
				i = ((buf[3] & 0x30) >> 4);
				if (i == 0 || i == 2) 
					continue;

				GetPAT(&buf[4]);

				continue;
			}

			top_pid_count[pid]++;
			if (top_pid_count[top_pid_pid] < top_pid_count[pid])
				top_pid_pid = pid;
//			Debug( 11,"%8i:top_pid_count[%4x]= %d\n", (int) filepos, top_pid_pid[pid & 0xff], top_pid_count[pid & 0xff]);

			for (i=0; i<max_pid; i++) {
				if (pid == ppid[i])
					break;
			}

			if (pid == ppid[i]) {
				if (/*first_pmt && */!(buf[1] & 0x40))
					continue;
				i = ((buf[3] & 0x30) >> 4);
				if (i == 0 || i == 2) 
					continue;
				if (i == 3)
					i = 4+buf[4]+1;
				else
					i = 4;
				i += buf[i];

				GetPMT(&buf[i],pid);
				continue;
			}
			if (first_pmt && selected_video_pid == 0 && headerpos > 4000000)
				PMT_failed = 5;
			data = buf + 4;
			if (buf[3] & 0x20) {	/* buf contains an adaptation field */
				data = buf + 5 + buf[4];
				if (data > nextbuf)
					continue;
			}
			if (buf[3] & 0x10) {
				
				if (first_pmt && PMT_failed > 4) {
					for (i = 0; i < last_pid; i++) {
						if (pids[i] == pid)
							break;
					}
					if (i == last_pid) {
						pids[last_pid] = pid;
						if (data[0] == 0 && data[1] == 0 && data[2] == 1 ) {
							if (data[3] == 0xbd)
								pid_type[last_pid++] = 129;
							if (data[3] == 0xe0)
								pid_type[last_pid++] = 2;
							if (data[3] == 0xc0)
								pid_type[last_pid++] = 3;
							Debug( 11,"%8i:Found ES type = %3x, ES pid = %4x\n", (int) filepos, data[3], pid);
						}
					}
				}				
				
				Debug( 12,"%6i:Top pid [%4x]= %d\n", (int) filepos, top_pid_pid, top_pid_count[top_pid_pid]);
				
				if (demux_pid == 1 && first_pmt && PMT_failed > 4 && selected_video_pid == 0 && top_pid_pid == pid) {
					selected_video_pid = pid;
					demux_pid = pid;
					Debug( 1,"%8i:Auto selected video pid = %4x\n", (int) filepos, demux_pid);
					csRestart = 1;
				} else
				for (i = 0; i < last_pid; i++) {
					if (pid == pids[i] || selected_video_pid == pid) {
						if (selected_video_pid != pid && (pid_type[i] == 3 || pid_type[i] == 4 || pid_type[i] == 6|| pid_type[i] == 129)) {

							if (first_pmt && PMT_failed > 4 && selected_video_pid && selected_audio_pid == 0) {
								selected_audio_pid = pid;
								Debug( 1,"%8i:Auto selected audio pid = %4x\n", (int) filepos, pid);
								found_pids=1;
//								csRestart = 1;
							}

							if (found_pids && selected_audio_pid == pid) {
								Debug( 12,"%8i:Demux audio pid = %4x\n",(int)filepos,  pid);
								demux_audio (data, nextbuf, (buf[1] & 0x40) ? DEMUX_PAYLOAD_START : demuxparm);
								break;
							}
						} else
						if (pid_type[i] == 1 || pid_type[i] == 2 || selected_video_pid == pid) {
							if (demux_pid == 1 && selected_video_pid != pid) {
								selected_video_pid = pid;
								demux_pid = pid;
								Debug( 1,"%8i:Auto selected video pid = %4x\n", (int) filepos, demux_pid);
								csRestart = 1;
							}
							if (selected_video_pid == pid) {
								Debug( 12,"%8i:Demux video pid = %4x\n",(int)filepos, pid);
								demux (data, nextbuf, (buf[1] & 0x40) ? DEMUX_PAYLOAD_START : demuxparm);
								break;
							}
						}
					}
				}
			}
		}
		if (end != buffer + buffer_size) {
			Debug( 11,"%8i:Read only %d bytes, stopping\n", (int)filepos,end - buffer, retries);
			break;
		}
		memcpy (buffer, buf, end - buf);
		buf = buffer + (end - buf);
    } while (!sigint  && !csFound);
    free (buffer);
}
 

void DetermineType(FILE *in_file)
{
	char buf[1024];
	int test = 0;
	int i,m;
	if (demux_pid == 0) {
		
		FSEEK(in_file, (__int64)80000, SEEK_SET);
		m = fread (buf, 1, 1024, in_file);
		FSEEK(in_file, (__int64)0, SEEK_SET);
		i = 0;
again:
		while (buf[i] != 0x47 && i < m) i++;
		if (i >= m - 2 * 0xBC)
			goto not_a_ts;
		if ((buf[i+0xBC] != 0x47 || buf[i + 2 * 0xBC] != 0x47)) {
			if (test>8)
				goto not_a_ts;
			else {
				test++;
				i++;
				goto again;
			}
		}
		demux_pid = 1;
	}
	max_pid=0;				// Initialize TS decoder
	first_pat = 1;
	first_pmt = 1;
not_a_ts:
	if (dump_seek) printf("Determine Stream: %d\n",demux_pid);

}

void DecodeOnePicture(FILE * f, int fp)
{
	static int oldfp=0;
	__int64 oldfrompos;
	off_t off_eof;
	reviewing = 1;
//	if (demux_asf)
//		return;
#ifdef _DEBUG
	if (oldfp != fp) {
		dump_seek = 1;
		oldfp = fp;
	} else {
		dump_seek = 0;
	}
#endif
	live_tv_retries = 0;
	if (dump_seek) printf("Starting: Start seek to %d  ------------------------------------------\n",fp);

    if (in_file == 0) {
		in_file = f;
	    mpeg2dec = mpeg2_init ();
	    if (mpeg2dec == NULL)
		exit (2);
	    mpeg2_malloc_hooks (malloc_hook, NULL);

		FSEEK(in_file, (__int64)0, SEEK_END);
		fileendpos = FTELL(in_file);
		if (fileendpos == -1)
			return;
		FSEEK(in_file, (__int64)0, SEEK_SET);

//		 decode_init(); // Audio decoder
//		csStartJump = 0;
/* replaced by */
		csJumping = 0;
		csStepping = 1;
		csStartJump = 0;


		seekIter = 0;
		seekDirection = 0;
		frompos = 0;
		SeekPos = 3;
		framenum = 0;	
		framenum_infer = 0;
		expected_frame_count = 0;
		sound_frame_counter = 0;
		initial_pts = 0;
		initial_pts_set = 0;
		initial_apts_set = 0;
		if (demux_asf) {
#ifdef USE_ASF
			ASF_Init(in_file);
			ASF_Open();
			demux_pid = 0;
			is_AC3 = 0;
			asf_loop();
#endif
		} else {
			
			DetermineType(in_file);
			if (demux_pid)
				ts_loop();
			else
				ps_loop();
		}
//		bitrate = ASF_bitrate();
//		byterate = bitrate / 8 / get_fps();
//		byterate = (int)((bitrate) / 8 / get_fps() * 0.75);
		if (frame_count > 0)
			byterate = fileendpos / frame_count;
		else {
			frame_count = fileendpos / byterate;
			//			DecodeOnePicture(f, frame_count);
		}
	}
	frame_ptr = 0;
	
	fgetpos(in_file, &filepos);
	if (filepos == -1)
	{
		reviewing = 0;
		return;
//	fp = 100;
	}
	frompos = filepos + ((fp-37) - framenum_infer) * (__int64)byterate; 
	oldfrompos = frompos;
	//	frompos = (fp-30) * byterate;
	if (frompos > fileendpos)
		frompos = fileendpos - 100*byterate;
	if (frompos < 0) 
		frompos = 0;
	if (dump_seek) printf("[%6d] Jumping from filepos %d to %6i with bitrate %d \n",framenum_infer,(int)(headerpos/byterate), (int)(frompos/byterate),(int)(byterate*8*get_fps()));

	csStartJump = 1;
	seekIter = 0;
	seekDirection = 0;
	SeekPos = fp;
	if (demux_asf) {
#ifdef USE_ASF
//		ASF_Init(in_file);
//		ASF_Open();
		demux_pid = 0;
		is_AC3 = 0;
		ASF_Control(NULL, 1,  ((double) frompos)/fileendpos); 
		asf_loop();
#endif
	} else {
		DetermineType(in_file);
		if (demux_pid)
			ts_loop();
		else
			ps_loop();
	}
	reviewing = 0;
}

void raise_exception(void)
{
	*(int *)0 = 0;
}



int filter(void)
{
	printf("Exception raised, Comskip is terminating\n");
	exit(99);
}



int main (int argc, char ** argv)
{
	int result = 0;
	int i;
//	fpos_t		fileendpos;
	char temp[512];

	char *ptr;
	size_t len;
#ifndef _DEBUG
//	__try
	{
		//      raise_exception();
#endif

//		output_debugwindow = 1;

		if (strstr(argv[0],"GUI"))
			output_debugwindow = 1;
		else {		
#ifdef _WIN32
			//added windows specific
			SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
		}
		//get path to executable
		ptr = argv[0];
		if (*ptr == '\"') {
			ptr++; //strip off quotation marks
			len = (size_t)(strchr(ptr,'\"') - ptr);
		} else {
			len = strlen(ptr);
		}
		strncpy(HomeDir, ptr, len);
		
		len = (size_t)max(0,strrchr(HomeDir,'\\') - HomeDir);
		if (len==0) {
			HomeDir[0] = '.';
			HomeDir[1] = '\0';
		} else {
			HomeDir[len] = '\0';
		}
		
		fprintf (stderr, "Comskip %s.%s, made using:\n", COMSKIPVERSION,SUBVERSION);
		
#ifdef _WIN32
#ifdef HAVE_IO_H
		_setmode (_fileno (stdin), O_BINARY);
		_setmode (_fileno (stdout), O_BINARY);
#endif
#endif
		
		fprintf (stderr, PACKAGE"-"VERSION
			" - by Michel Lespinasse <walken@zoy.org> and Aaron Holtzman\n");
		
		in_file = LoadSettings(argc, argv); 
		
		FSEEK(in_file, (__int64)0, SEEK_END);
		fileendpos = FTELL(in_file);
		
		FSEEK(in_file, (__int64)0, SEEK_SET);
		
		mpeg2dec = mpeg2_init ();
		if (mpeg2dec == NULL)
			exit (2);
		mpeg2_malloc_hooks (malloc_hook, NULL);
		decode_init(); // Audio decoder
		InitialAC3();
		
			sample_file = fopen("samples.csv", "w");
		DUMP_OPEN

		csRestart = 0;
		framenum = 0;	
		framenum_infer = 0;
		expected_frame_count = 0;

		if (demux_asf) {
#ifdef USE_ASF
			ASF_Init(in_file);
			ASF_Open();
			demux_pid = 0;

			is_AC3 = 0;
			asf_loop();
#endif
		} else {
			
			DetermineType(in_file);
			if (demux_pid)
				ts_loop();
			else
				ps_loop();
		}
		mpeg2_close (mpeg2dec);
		fclose (in_file);

		frame_ptr = NULL;
		if (demux_pid && framenum_infer == 0) {
            result = 3;
			Debug(0,"Video PID not found, available video PID's ");
			for (i = 0; i < last_pid; i++) {
				if (pid_type[i] == 1 || pid_type[i] == 2 ) {
					Debug(0, "%x, ", pids[i]);
				}
			}
			Debug(0,"\n");
		}

		Debug( 10,"\nParsed %d video frames and %d audio frames of %4.2f fps\n", framenum_infer, sound_frame_counter, get_fps());
		Debug( 10,"\nMaximum Volume found is %d\n", max_volume_found);
		if (framenum > 0)
			byterate = fileendpos / framenum;
		
		print_fps (1);
#ifdef USE_ASF
		ASF_Close();
#endif
		in_file = 0;
		if (framenum>0)	{
			byterate = fileendpos / framenum;
			if(BuildMasterCommList()) {
				result = 1;
				printf("Commercials were found.\n");
			} else {
				result = 0;
				printf("Commercials were not found.\n");
			}
			if (output_debugwindow) {
				processCC = 0;
				printf("Close window when done\n");

				DUMP_CLOSE
				if (output_timing) {
					output_timing = 0;
				}

#ifndef GUI
				while(1) {
					ReviewResult();
					vo_refresh();
					Sleep((DWORD)100);
				}
#endif
				//		printf(" Press Enter to close debug window\n");
				//		gets(HomeDir);
			}
		}

#ifndef _DEBUG
	}
//	__except(filter()) /* Stage 3 */
//	{
//      printf("Exception raised, terminating\n");/* Stage 5 of terminating exception */
//		exit(result);
//	}
#endif
	exit (result);
}

