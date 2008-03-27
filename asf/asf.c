/*****************************************************************************
 * asf.c : ASF demux module
 *****************************************************************************
 * Copyright (C) 2002-2003 the VideoLAN team
 * $Id: asf.c 12087 2005-08-09 14:43:04Z jpsaman $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdlib.h>                                      /* malloc(), free() */

#define __VLC__
//#define WORDS_BIGENDIAN 1

#include "vlc/vlc.h"
#include "vlc/input.h"

#include "vlc_meta.h"


#include "codecs.h"                        /* BITMAPINFOHEADER, WAVEFORMATEX */
#include "libasf.h"

/* TODO
 *  - add support for the newly added object: language, bitrate,
 *                                            extended stream properties.
 */

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
int  ASF_Open  ( vlc_object_t * );
static void Close ( vlc_object_t * );
/*
vlc_module_begin();
    set_category( CAT_INPUT );
    set_subcategory( SUBCAT_INPUT_DEMUX );
    set_description( _("ASF v1.0 demuxer") );
    set_capability( "demux2", 200 );
    set_callbacks( Open, Close );
    add_shortcut( "asf" );
vlc_module_end();
*/


static demux_t asf_demux;
static char ASF_Buffer[70000];
static __int64 ASF_start;
static __int64 ASF_end;
static __int64 ASF_cur;
extern __int64 headerpos;


void ASF_Init(void *s)
{
	memset(&asf_demux, 0, sizeof(asf_demux));
	asf_demux.s = s;
	own_stream_Seek(s, (__int64)0);
	ASF_cur = 0;
	ASF_start = 0;
	ASF_end = 0;
}

void ASF_Close(void)
{
	asf_demux.s = NULL;
}

void ASF_stop(){
	asf_demux.b_die = 1;
}

	 
extern int is_AC3;

#define msg_Dbg  own_Dbg
#define msg_Warn own_Dbg
#define msg_Info own_Dbg
#define msg_Err own_Dbg

char debugText[1000];

static void own_Dbg(void *p, char* fmt, ...)
{
	va_list	ap;
	debugText[0] = '\n';
	va_start(ap, fmt);
	vsprintf(&debugText[1], fmt, ap);
	va_end(ap);

	Debug(10, debugText);
}

#define stream_Peek own_stream_Peak
#define stream_Tell own_stream_Tell
#define stream_Seek  own_stream_Seek
#define stream_Read own_stream_Read

extern __int64 own_stream_Tell( stream_t *s);
extern __int64 own_stream_Size( stream_t *s);


extern __int64 _ftelli64(FILE *);
extern int  _fseeki64(FILE *, __int64, int);
extern int dvrms_live_tv_retries;
extern int dvrmsstandoff;


static int max_packet_size;
static int max_packet_size_stream;

int own_stream_Peak( stream_t *s, uint8_t **pp_peek, int i_peek )
{
	int ins;
	int retries = 0;
	*pp_peek = ASF_Buffer;
	if (ASF_cur + i_peek >= ASF_end || ASF_cur < ASF_start)
	{
//		if (ASF_cur == 0)
//			rewind(s);
again:

	
		if (dvrmsstandoff > 0 && own_stream_Size(s) < ASF_cur + dvrmsstandoff * (__int64)1000) {
			if (retries <= dvrms_live_tv_retries-2) {
				Debug( 11,"Sleep due to dvrmsstandoff, retries = %d \n", retries);
			dosleep:
				retries++;
				Sleep(1000L);
				goto again;
			} else {
				dvrmsstandoff = 0;
			}
		}
	
	
		if (_fseeki64(s, ASF_cur, 0) == 0)
			ins = fread(*pp_peek, 1, 65535+1, (FILE *) s);
		else
			ins = 0;
		if (ins < 65535+1) {
/*
			if (retries <= dvrms_live_tv_retries) {
				Debug( 11,"Sleep on live TV in own_stream_Peak, retries = %i \n", retries);
				goto dosleep;
			} else {
				Debug( 11,"Exceeded retries in own_stream_Peak, retries = %i \n", retries);

			}
*/
		}
		ASF_start = ASF_cur;
		ASF_end = ASF_cur + ins;
		return(min(ins,i_peek));
	}
	*pp_peek = &ASF_Buffer[(int)(ASF_cur - ASF_start)];
	return i_peek;

}

int own_stream_Read( stream_t *s, void *p_read, int i_read )
{
	void *p_input;

	if (p_read) {
		if (own_stream_Peak(s, &p_input, i_read) < i_read)
			return(0);
		memcpy(p_read, p_input, i_read);
	}
	ASF_cur += i_read;
	return i_read;
}

__int64 own_stream_Size( stream_t *s)
{
	__int64 old_pos, end_pos;
	old_pos = _ftelli64(s);
	_fseeki64(s, (__int64)0, SEEK_END);
	end_pos = _ftelli64(s);
	_fseeki64(s, old_pos, SEEK_END);
	return(end_pos);
}	


__int64 own_stream_Tell( stream_t *s)
{
	return ASF_cur;
}

int own_stream_Seek(stream_t *s,__int64 p)
{
	ASF_cur = p;
	return 0;
}

#define PTS_STEP 90 //1000


//#define MPEGbufferSize 70000
//static char MPEGbuffer[MPEGbufferSize];
//int MPEGbufferIndex=0;


struct block_sys_t
{
    uint8_t     *p_allocated_buffer;
    int         i_allocated_buffer;
};



 
#define BLOCK_PADDING_SIZE 32 
block_t *__block_New( vlc_object_t *p_obj, int i_size )
{
    /* We do only one malloc
     * TODO bench if doing 2 malloc but keeping a pool of buffer is better
     * 16 -> align on 16
     * 2 * BLOCK_PADDING_SIZE -> pre + post padding
     */
    block_sys_t *p_sys;
    const int i_alloc = i_size + 2 * BLOCK_PADDING_SIZE + 16;
    block_t *p_block =
        malloc( sizeof( block_t ) + sizeof( block_sys_t ) + i_alloc );

    if( p_block == NULL ) return NULL;

    /* Fill opaque data */
    p_sys = (block_sys_t*)( (uint8_t*)p_block + sizeof( block_t ) );
    p_sys->i_allocated_buffer = i_alloc;
    p_sys->p_allocated_buffer = (uint8_t*)p_block + sizeof( block_t ) +
        sizeof( block_sys_t );

    /* Fill all fields */
    p_block->p_next         = NULL;
    p_block->p_prev         = NULL;
    p_block->i_flags        = 0;
    p_block->i_pts          = 0;
    p_block->i_dts          = 0;
    p_block->i_length       = 0;
    p_block->i_rate         = 0;
    p_block->i_buffer       = i_size;
    p_block->p_buffer       =
        &p_sys->p_allocated_buffer[BLOCK_PADDING_SIZE +
            16 - ((uintptr_t)p_sys->p_allocated_buffer % 16 )];
    p_block->pf_release     = free;

    /* Is ok, as no comunication between p_vlc */
    p_block->p_manager      = VLC_OBJECT( p_obj->p_vlc );
    p_block->p_sys          = p_sys;

    return p_block;
}
 

/*****************************************************************************
 * demux2_vaControlHelper:
 *****************************************************************************/
int demux2_vaControlHelper( stream_t *s,
                            int64_t i_start, int64_t i_end,
                            int i_bitrate, int i_align,
                            int i_query, va_list args )
{
    int64_t i_tell;
    double  f, *pf;
    int64_t i64, *pi64;

//    if( i_end < 0 )    
		i_end   = own_stream_Size( s );
    if( i_start < 0 )  i_start = 0;
    if( i_align <= 0 ) i_align = 1;
    i_tell = stream_Tell( s );

    switch( i_query )
    {
        case DEMUX_GET_LENGTH:
            pi64 = (int64_t*)va_arg( args, int64_t * );
            if( i_bitrate > 0 && i_end > i_start )
            {
                *pi64 = I64C(8000000) * (i_end - i_start) / i_bitrate;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;

        case DEMUX_GET_TIME:
            pi64 = (int64_t*)va_arg( args, int64_t * );
            if( i_bitrate > 0 && i_end > i_start )
            {
                *pi64 = I64C(8000000) * (i_tell - i_start) / i_bitrate;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;

        case DEMUX_GET_POSITION:
            pf = (double*)va_arg( args, double * );
            if( i_start < i_end )
            {
                *pf = (double)( i_tell - i_start ) /
                      (double)( i_end  - i_start );
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;


        case DEMUX_SET_POSITION:
            f = (double)va_arg( args, double );
            if( i_start < i_end && f >= 0.0 && f <= 1.0 )
            {
                int64_t i_block = (f * ( i_end - i_start )) / i_align;

                if( stream_Seek( s, i_start + i_block * i_align ) )
                {
                    return VLC_EGENERIC;
                }
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;

        case DEMUX_SET_TIME:
            i64 = (int64_t)va_arg( args, int64_t );
            if( i_bitrate > 0 && i64 >= 0 )
            {
                int64_t i_block = i64 * i_bitrate / I64C(8000000) / i_align;
                if( stream_Seek( s, i_start + i_block * i_align ) )
                {
                    return VLC_EGENERIC;
                }
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;

        case DEMUX_GET_FPS:
        case DEMUX_GET_META:
        case DEMUX_SET_NEXT_DEMUX_TIME:
        case DEMUX_GET_TITLE_INFO:
        case DEMUX_SET_GROUP:
            return VLC_EGENERIC;

        default:
            msg_Err( s, "unknown query in demux_vaControlDefault" );
            return VLC_EGENERIC;
    }
}
 
char *vlc_strndup( const char *string, size_t n )
{
    char *psz;
    size_t len = strlen( string );

    len = __MIN( len, n );
    psz = (char*)malloc( len + 1 );

    if( psz != NULL )
    {
        memcpy( (void*)psz, (const void*)string, len );
        psz[ len ] = 0;
    }

    return psz;
} 

char *vlc_strdup( const char *string)
{
    char *psz;
    size_t len = strlen( string );

    psz = (char*)malloc( len + 1 );

    if( psz != NULL )
    {
        memcpy( (void*)psz, (const void*)string, len );
        psz[ len ] = 0;
    }

    return psz;
} 

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
int ASF_Demux  ( demux_t * );
static int Control( demux_t *, int i_query, va_list args );

typedef struct
{
    int i_cat;

    es_out_id_t     *p_es;

    asf_object_stream_properties_t *p_sp;

    mtime_t i_time;

    block_t         *p_frame; /* use to gather complete frame */

} asf_track_t;

struct demux_sys_t
{
    mtime_t             i_time;     /* s */
    mtime_t             i_length;   /* length of file file */
    int64_t             i_bitrate;  /* global file bitrate */

    asf_object_root_t            *p_root;
    asf_object_file_properties_t *p_fp;

    unsigned int        i_track;
    asf_track_t         *track[128];

    int64_t             i_data_begin;
    int64_t             i_data_end;

    vlc_meta_t          *meta;
};

mtime_t ASF_PTS()
{
	return asf_demux.p_sys->i_time;
}

int ASF_bitrate()
{
    demux_sys_t *p_sys = asf_demux.p_sys;
	return(p_sys->i_bitrate);
//	return(1200000);
}


int ASF_Control( stream_t *s, int i_query, ... )
{
    va_list args;
    int     i_result;

    va_start( args, i_query );
    i_result = Control( s, i_query, args );
    va_end( args );
    return i_result;
}
 

static mtime_t  GetMoviePTS( demux_sys_t * );
static int      DemuxInit( demux_t * );
static void     DemuxEnd( demux_t * );
static int      DemuxPacket( demux_t * );

/*****************************************************************************
 * Open: check file and initializes ASF structures
 *****************************************************************************/
int ASF_Open( vlc_object_t * p_this )
{
    demux_t     *p_demux = &asf_demux;
    demux_sys_t *p_sys;
    guid_t      guid;
    uint8_t     *p_peek;
	int old_dvrmsstandoff = dvrmsstandoff;

    /* A little test to see if it could be a asf stream */
    if( stream_Peek( p_demux->s, &p_peek, 16 ) < 16 ) return VLC_EGENERIC;

    ASF_GetGUID( &guid, p_peek );
    if( !ASF_CmpGUID( &guid, &asf_object_header_guid ) ) return VLC_EGENERIC;

    /* Set p_demux fields */
    p_demux->pf_demux = ASF_Demux;
    p_demux->pf_control = ASF_Control;
    p_demux->p_sys = p_sys = malloc( sizeof( demux_sys_t ) );
    memset( p_sys, 0, sizeof( demux_sys_t ) );

    /* Load the headers */
    if( DemuxInit( p_demux ) )
    {
        return VLC_EGENERIC;
    }
	dvrmsstandoff = old_dvrmsstandoff;

	max_packet_size = 0;
	max_packet_size_stream = 0;


    return VLC_SUCCESS;
}

extern mtime_t pts;
extern mtime_t apts; 
/*****************************************************************************
 * Demux: read packet and send them to decoders
 *****************************************************************************/
int ASF_Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys;
	p_demux = &asf_demux;
    p_sys = p_demux->p_sys;

//    for( ;; )
    {
        uint8_t *p_peek;
        mtime_t i_length;
        mtime_t i_time_begin = GetMoviePTS( p_sys );
        int i_result;

        if( p_demux->b_die )
		{
//            break;
        }

        /* Check if we have concatenated files */
        if( stream_Peek( p_demux->s, &p_peek, 16 ) == 16 )
        {
            guid_t guid;

            ASF_GetGUID( &guid, p_peek );
            if( ASF_CmpGUID( &guid, &asf_object_header_guid ) )
            {
                msg_Warn( p_demux, "Found a new ASF header" );
                /* We end this stream */
                DemuxEnd( p_demux );

                /* And we prepare to read the next one */
                if( DemuxInit( p_demux ) )
                {
                    msg_Err( p_demux, "failed to load the new header" );
                    return 0;
                }
//                continue;
            }
        }

        /* Read and demux a packet */
        if( ( i_result = DemuxPacket( p_demux ) ) <= 0 )
        {
            return i_result;
        }
        if( i_time_begin == -1 )
        {
            i_time_begin = GetMoviePTS( p_sys );
//			pts = i_time_begin;
        }
        else
        {
            i_length = GetMoviePTS( p_sys ) - i_time_begin;
            if( i_length < 0 || i_length >= 40 * PTS_STEP )
            {
//                break;
            }
        }
    }

    /* Set the PCR */
    p_sys->i_time = GetMoviePTS( p_sys );
    if( p_sys->i_time >= 0 )
    {
        // es_out_Control( p_demux->out, ES_OUT_SET_PCR, p_sys->i_time );
    }

    return 1;
}

/*****************************************************************************
 * Close: frees unused data
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    demux_t     *p_demux = (demux_t *)&asf_demux;

    DemuxEnd( p_demux );

    free( p_demux->p_sys );
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    demux_sys_t *p_sys;// = p_demux->p_sys;
    int64_t     *pi64;
    int         i;
    vlc_meta_t **pp_meta;
	p_demux = &asf_demux;
	p_sys = p_demux->p_sys;

    switch( i_query )
    {
        case DEMUX_SET_TIME:
            return VLC_EGENERIC;

        case DEMUX_GET_LENGTH:
            pi64 = (int64_t*)va_arg( args, int64_t * );
            *pi64 = p_sys->i_length;
            return VLC_SUCCESS;

        case DEMUX_GET_META:
            pp_meta = (vlc_meta_t**)va_arg( args, vlc_meta_t** );
            *pp_meta = vlc_meta_Duplicate( p_sys->meta );
            return VLC_SUCCESS;

        case DEMUX_SET_POSITION:
            p_sys->i_time = -1;
            for( i = 0; i < 128 ; i++ )
            {
                asf_track_t *tk = p_sys->track[i];
                if( tk ) tk->i_time = -1;
            }

        default:
            return demux2_vaControlHelper( p_demux->s,
                                           p_sys->i_data_begin, p_sys->i_data_end,
                                           p_sys->i_bitrate, p_sys->p_fp->i_min_data_packet_size,
                                           i_query, args );
    }
}

/*****************************************************************************
 *
 *****************************************************************************/
static mtime_t GetMoviePTS( demux_sys_t *p_sys )
{
    mtime_t i_time;
    int     i;

    i_time = -1;
    for( i = 0; i < 128 ; i++ )
    {
        asf_track_t *tk = p_sys->track[i];

        if( tk && tk->p_es && tk->i_time > 0)
        {
            if( i_time < 0 )
            {
                i_time = tk->i_time;
            }
            else
            {
                i_time = __MIN( i_time, tk->i_time );
            }
        }
    }

    return i_time;
}

#define GETVALUE2b( bits, var, def ) \
    switch( (bits)&0x03 ) \
    { \
        case 1: var = p_peek[i_skip]; i_skip++; break; \
        case 2: var = GetWLE( p_peek + i_skip );  i_skip+= 2; break; \
        case 3: var = GetDWLE( p_peek + i_skip ); i_skip+= 4; break; \
        case 0: \
        default: var = def; break;\
    }

extern int csFound;

static int DemuxPacket( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    int         i_data_packet_min = p_sys->p_fp->i_min_data_packet_size;
    uint8_t     *p_peek;
    int         i_skip;

    int         i_packet_size_left;
    int         i_packet_flags;
    int         i_packet_property;

    int         b_packet_multiple_payload;
    int         i_packet_length;
    int         i_packet_sequence;
    int         i_packet_padding_length;

    uint32_t    i_packet_send_time;
    uint16_t    i_packet_duration;
    int         i_payload;
    int         i_payload_count;
    int         i_payload_length_type;
	int retries = 0;

again:
    if( stream_Peek( p_demux->s, &p_peek,i_data_packet_min)<i_data_packet_min )
    {
/*
		if (retries <= dvrms_live_tv_retries) {
			Debug( 11,"Sleep on live TV in DemuxPacket peek, retries = %i \n", retries);
			retries++;
			Sleep(1000L);
			goto again;
		} else {
			Debug( 11,"Exceeded retries in DemuxPacket peek, retries = %i \n", retries);
		}
*/
		msg_Warn( p_demux, "\ncannot peek while getting new packet, EOF ?" );
        return 0;
    }
    i_skip = 0;

    /* *** parse error correction if present *** */
    if( p_peek[0]&0x80 )
    {
        unsigned int i_error_correction_length_type;
        unsigned int i_error_correction_data_length;
        unsigned int i_opaque_data_present;

        i_error_correction_data_length = p_peek[0] & 0x0f;  // 4bits
        i_opaque_data_present = ( p_peek[0] >> 4 )& 0x01;    // 1bit
        i_error_correction_length_type = ( p_peek[0] >> 5 ) & 0x03; // 2bits
        i_skip += 1; // skip error correction flags

        if( i_error_correction_length_type != 0x00 ||
            i_opaque_data_present != 0 ||
            i_error_correction_data_length != 0x02 )
        {
            goto loop_error_recovery;
        }

        i_skip += i_error_correction_data_length;
    }
    else
    {
/*
		if (retries <= dvrms_live_tv_retries) {
			Debug( 11,"Sleep on live TV in DemuxPacket error correction, retries = %i \n", retries);
			retries++;
			Sleep(1000L);
			goto again;
		} else {
			Debug( 11,"Exceeded retries in DemuxPacket error correction, retries = %i \n", retries);
		}
*/
        msg_Warn( p_demux, "p_peek[0]&0x80 != 0x80" );
    }

    /* sanity check */
    if( i_skip + 2 >= i_data_packet_min )
    {
        goto loop_error_recovery;
    }

    i_packet_flags = p_peek[i_skip]; i_skip++;
    i_packet_property = p_peek[i_skip]; i_skip++;

    b_packet_multiple_payload = i_packet_flags&0x01;

    /* read some value */
    GETVALUE2b( i_packet_flags >> 5, i_packet_length, i_data_packet_min );
    GETVALUE2b( i_packet_flags >> 1, i_packet_sequence, 0 );
    GETVALUE2b( i_packet_flags >> 3, i_packet_padding_length, 0 );

    i_packet_send_time = GetDWLE( p_peek + i_skip ); i_skip += 4;
    i_packet_duration  = GetWLE( p_peek + i_skip ); i_skip += 2;

	asf_tag_picture((mtime_t)i_packet_send_time * PTS_STEP);


//        i_packet_size_left = i_packet_length;   // XXX donn�s reellement lu
    /* FIXME I have to do that for some file, I don't known why */
    i_packet_size_left = i_data_packet_min;

    if( b_packet_multiple_payload )
    {
        i_payload_count = p_peek[i_skip] & 0x3f;
        i_payload_length_type = ( p_peek[i_skip] >> 6 )&0x03;
        i_skip++;
    }
    else
    {
        i_payload_count = 1;
        i_payload_length_type = 0x02; // unused
    }

    for( i_payload = 0; i_payload < i_payload_count ; i_payload++ )
    {
        asf_track_t   *tk;

        int i_stream_number;
        int i_media_object_number;
        int i_media_object_offset;
        int i_replicated_data_length;
        int i_payload_data_length;
        int i_payload_data_pos;
        int i_sub_payload_data_length;
        int i_tmp;

		int first_packet = 1;

        mtime_t i_pts;
        mtime_t i_pts_delta;

        if( i_skip >= i_packet_size_left )
        {
            /* prevent some segfault with invalid file */
            break;
        }

        i_stream_number = p_peek[i_skip] & 0x7f;
        i_skip++;

        GETVALUE2b( i_packet_property >> 4, i_media_object_number, 0 );
        GETVALUE2b( i_packet_property >> 2, i_tmp, 0 );
        GETVALUE2b( i_packet_property, i_replicated_data_length, 0 );

        if( i_replicated_data_length > 1 ) // should be at least 8 bytes
        {
//			i_pts = (mtime_t)GetDWLE( p_peek + i_skip + 4 );
            i_pts = (mtime_t)GetDWLE( p_peek + i_skip + 4 ) * PTS_STEP;
            i_skip += i_replicated_data_length;
            i_pts_delta = 0;

            i_media_object_offset = i_tmp;

            if( i_skip >= i_packet_size_left )
            {
                break;
            }
        }
        else if( i_replicated_data_length == 1 )
        {
            /* msg_Dbg( p_demux, "found compressed payload" ); */

            i_pts = (mtime_t)i_tmp * PTS_STEP;
            i_pts_delta = (mtime_t)p_peek[i_skip] * PTS_STEP; i_skip++;

            i_media_object_offset = 0;
        }
        else
        {
            i_pts = (mtime_t)i_packet_send_time * PTS_STEP;
            i_pts_delta = 0;

            i_media_object_offset = i_tmp;
        }

        i_pts = __MAX( i_pts - p_sys->p_fp->i_preroll * PTS_STEP, 0 );
        if( b_packet_multiple_payload )
        {
            GETVALUE2b( i_payload_length_type, i_payload_data_length, 0 );
        }
        else
        {
            i_payload_data_length = i_packet_length -
                                        i_packet_padding_length - i_skip;
        }

        if( i_payload_data_length < 0 || i_skip + i_payload_data_length > i_packet_size_left )
        {
            break;
        }
#if 0
         msg_Dbg( p_demux,
                  "payload(%d/%d) stream_number:%d media_object_number:%d media_object_offset:%d replicated_data_length:%d payload_data_length %d",
                  i_payload + 1, i_payload_count, i_stream_number, i_media_object_number,
                  i_media_object_offset, i_replicated_data_length, i_payload_data_length );
#endif

        if( ( tk = p_sys->track[i_stream_number] ) == NULL )
        {
            msg_Warn( p_demux,
                      "undeclared stream[Id 0x%x]", i_stream_number );
            i_skip += i_payload_data_length;
            continue;   // over payload
        }

		if (tk->i_cat == VIDEO_ES) {
//			asf_tag_picture((mtime_t)i_pts + i_payload * (mtime_t)i_pts_delta);
			headerpos = ASF_cur;

		}
		if (tk->i_cat == AUDIO_ES || tk->i_cat == UNKNOWN_ES) {
			set_apts((mtime_t)i_pts + i_payload * (mtime_t)i_pts_delta);
		}

//        if( !tk->p_es )
//        {
//            i_skip += i_payload_data_length;
//            continue;
//        }

//		MPEGbufferIndex=0;

        for( i_payload_data_pos = 0;
             i_payload_data_pos < i_payload_data_length &&
                    i_packet_size_left > 0;
             i_payload_data_pos += i_sub_payload_data_length )
        {
            block_t *p_frag;
            int i_read;

            // read sub payload length
            if( i_replicated_data_length == 1 )
            {
                i_sub_payload_data_length = p_peek[i_skip]; i_skip++;
                i_payload_data_pos++;
            }
            else
            {
                i_sub_payload_data_length = i_payload_data_length;
            }

            /* FIXME I don't use i_media_object_number, sould I ? */
//            if( tk->p_frame && i_media_object_offset == 0 )
if (0)
            {
                /* send complete packet to decoder */
                block_t *p_gather = block_ChainGather( tk->p_frame );

                es_out_Send( p_demux->out, tk->p_es, p_gather );

                tk->p_frame = NULL;
            }

            i_read = i_sub_payload_data_length + i_skip;
            if( stream_Peek( p_demux->s, &p_peek, i_read ) < i_read )
            {
                msg_Warn( p_demux, "\ncannot read data" );
                return 0;
            } else {
//                msg_Warn( p_demux, "\nread %d data of stream %d", i_read, i_stream_number );
			}

			if (tk->i_cat == VIDEO_ES) {
//				dump_video(&p_peek[i_skip], &p_peek[i_read]);
				dump_video_start();
				decode_mpeg2 (&p_peek[i_skip], &p_peek[i_read]);
if (csFound)
	return(0);
		        i_packet_size_left -= i_read;
/*
				if (MPEGbufferIndex + i_read > MPEGbufferSize) {
					msg_Warn( p_demux, "\npacket too large" );
				} else {
					memcpy(&MPEGbuffer[MPEGbufferIndex], p_peek, i_read);
					MPEGbufferIndex += i_read;
					if (i_packet_size_left <= 1 && MPEGbufferIndex > 0 ) {
						decode_mpeg2 (MPEGbuffer, &MPEGbuffer[MPEGbufferIndex]);
						MPEGbufferIndex=0;
					}
				}
*/
			}
			if (tk->i_cat == AUDIO_ES || tk->i_cat == UNKNOWN_ES) {
//					dump_audio(&p_peek[i_skip], &p_peek[i_read]);

				if (i_read - i_skip > max_packet_size) {
					max_packet_size = i_read - i_skip;
					max_packet_size_stream = i_stream_number;
				}
				if ( max_packet_size_stream == i_stream_number) {
					dump_audio_start();
					decode_mpeg2_audio(&p_peek[i_skip], &p_peek[i_read]);
				}
				i_packet_size_left -= i_read;
			}



			stream_Read( p_demux->s, NULL, i_read );
/*
            p_frag->p_buffer += i_skip;
            p_frag->i_buffer -= i_skip;
*/
            if( tk->p_frame == NULL )
            {
                tk->i_time =
                    ( (mtime_t)i_pts + i_payload * (mtime_t)i_pts_delta );
/*
                p_frag->i_pts = tk->i_time;

                if( tk->i_cat != VIDEO_ES )
                    p_frag->i_dts = p_frag->i_pts;
                else
                {
                    p_frag->i_dts = p_frag->i_pts;
                    p_frag->i_pts = 0;
                }
*/
			}

//            block_ChainAppend( &tk->p_frame, p_frag );

            i_skip = 0;
            if( i_packet_size_left > 0 )
            {
                if( stream_Peek( p_demux->s, &p_peek, i_packet_size_left )
                                                         < i_packet_size_left )
                {
                    msg_Warn( p_demux, "\ncannot peek, EOF ?" );
                    return 0;
                }
            }
        }
//        msg_Warn( p_demux, "\nDone %d data of stream %d", MPEGbufferIndex, i_stream_number );

    }

    if( i_packet_size_left > 0 )
    {
        if( stream_Read( p_demux->s, NULL, i_packet_size_left )
                                                         < i_packet_size_left )
        {
            msg_Warn( p_demux, "cannot skip data, EOF ?" );
            return 0;
        }
    }

    return 1;

loop_error_recovery:
/*
	if (retries <= dvrms_live_tv_retries) {
		Debug( 11,"Sleep on live TV in DemuxPacket loop_error_recovery, retries = %i \n", retries);
		retries++;
		Sleep(1000L);
		goto again;
	}
*/
    msg_Warn( p_demux, "unsupported packet header" );
    if( p_sys->p_fp->i_min_data_packet_size != p_sys->p_fp->i_max_data_packet_size )
    {
        msg_Err( p_demux, "unsupported packet header, fatal error" );
        return -1;
    }
    stream_Read( p_demux->s, NULL, i_data_packet_min );

    return 1;
}

/*****************************************************************************
 *
 *****************************************************************************/
static int DemuxInit( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    vlc_bool_t  b_seekable = 1;
    int         i;


    unsigned int    i_stream;
    asf_object_content_description_t *p_cd;

    /* init context */
    p_sys->i_time   = -1;
    p_sys->i_length = 0;
    p_sys->i_bitrate = 0;
    p_sys->p_root   = NULL;
    p_sys->p_fp     = NULL;
    p_sys->i_track  = 0;
    for( i = 0; i < 128; i++ )
    {
        p_sys->track[i] = NULL;
    }
    p_sys->i_data_begin = -1;
    p_sys->i_data_end   = -1;
    p_sys->meta         = NULL;

    /* Now load all object ( except raw data ) */
//    stream_Control( p_demux->s, STREAM_CAN_FASTSEEK, &b_seekable );
    if( (p_sys->p_root = ASF_ReadObjectRoot( p_demux->s, b_seekable )) == NULL )
    {
        msg_Warn( p_demux, "ASF plugin discarded (not a valid file)" );
        return VLC_EGENERIC;
    }
    p_sys->p_fp = p_sys->p_root->p_fp;

    if( p_sys->p_fp->i_min_data_packet_size != p_sys->p_fp->i_max_data_packet_size )
    {
        msg_Warn( p_demux, "ASF plugin discarded (invalid file_properties object)" );
        goto error;
    }

    p_sys->i_track = ASF_CountObject( p_sys->p_root->p_hdr,
                                      &asf_object_stream_properties_guid );
    if( p_sys->i_track <= 0 )
    {
        msg_Warn( p_demux, "ASF plugin discarded (cannot find any stream!)" );
        goto error;
    }

    msg_Dbg( p_demux, "found %d streams", p_sys->i_track );

    for( i_stream = 0; i_stream < p_sys->i_track; i_stream ++ )
    {
        asf_track_t    *tk;
        asf_object_stream_properties_t *p_sp;
        vlc_bool_t b_access_selected;

        p_sp = ASF_FindObject( p_sys->p_root->p_hdr,
                               &asf_object_stream_properties_guid,
                               i_stream );

        tk = p_sys->track[p_sp->i_stream_number] = malloc( sizeof( asf_track_t ) );
        memset( tk, 0, sizeof( asf_track_t ) );

        tk->i_time = -1;
        tk->p_sp = p_sp;
        tk->p_es = NULL;
        tk->p_frame = NULL;

        /* Check (in case of mms) if this track is selected (ie will receive data) */
/*
        if( !stream_Control( p_demux->s, STREAM_CONTROL_ACCESS, ACCESS_GET_PRIVATE_ID_STATE,
                             p_sp->i_stream_number, &b_access_selected ) &&
            !b_access_selected )
        {
            tk->i_cat = UNKNOWN_ES;
            msg_Dbg( p_demux, "ignoring not selected stream(ID:%d) (by access)",
                     p_sp->i_stream_number );
            continue;
        }
*/
		if( ASF_CmpGUID( &p_sp->i_stream_type,
                              &asf_object_stream_type_video ) &&
                 p_sp->i_type_specific_data_length >= 11 +
                 sizeof( BITMAPINFOHEADER ) )
        {
            es_format_t  fmt;
            uint8_t      *p_data = &p_sp->p_type_specific_data[11];

            es_format_Init( &fmt, VIDEO_ES,
                            VLC_FOURCC( p_data[16], p_data[17],
                                        p_data[18], p_data[19] ) );
            fmt.video.i_width = GetDWLE( p_data + 4 );
            fmt.video.i_height= GetDWLE( p_data + 8 );

            if( p_sp->i_type_specific_data_length > 11 +
                sizeof( BITMAPINFOHEADER ) )
            {
                fmt.i_extra = __MIN( GetDWLE( p_data ),
                                     p_sp->i_type_specific_data_length - 11 -
                                     sizeof( BITMAPINFOHEADER ) );
                fmt.p_extra = malloc( fmt.i_extra );
                memcpy( fmt.p_extra, &p_data[sizeof( BITMAPINFOHEADER )],
                        fmt.i_extra );
            }

            /* Look for an aspect ratio */
            if( p_sys->p_root->p_metadata )
            {
                asf_object_metadata_t *p_meta = p_sys->p_root->p_metadata;
                int i_aspect_x = 0, i_aspect_y = 0;
                unsigned int i;

                for( i = 0; i < p_meta->i_record_entries_count; i++ )
                {
                    if( !strcmp( p_meta->record[i].psz_name, "AspectRatioX" ) )
                    {
                        if( (!i_aspect_x && !p_meta->record[i].i_stream) ||
                            p_meta->record[i].i_stream ==
                            p_sp->i_stream_number )
                            i_aspect_x = p_meta->record[i].i_val;
                    }
                    if( !strcmp( p_meta->record[i].psz_name, "AspectRatioY" ) )
                    {
                        if( (!i_aspect_y && !p_meta->record[i].i_stream) ||
                            p_meta->record[i].i_stream ==
                            p_sp->i_stream_number )
                            i_aspect_y = p_meta->record[i].i_val;
                    }
                }

                if( i_aspect_x && i_aspect_y )
                {
                    fmt.video.i_aspect = i_aspect_x *
                        (int64_t)fmt.video.i_width * VOUT_ASPECT_FACTOR /
                        fmt.video.i_height / i_aspect_y;
                }
	    }

            tk->i_cat = VIDEO_ES;
            tk->p_es = 1; // es_out_Add( p_demux->out, &fmt );
//            es_format_Clean( &fmt );

            msg_Dbg( p_demux, "added new video stream(ID:%d)",
                     p_sp->i_stream_number );
        }
		else
        if( ASF_CmpGUID( &p_sp->i_stream_type, &asf_object_stream_type_audio ) &&
            p_sp->i_type_specific_data_length >= sizeof( WAVEFORMATEX ) - 2 )
        {
            es_format_t fmt;
            uint8_t *p_data = p_sp->p_type_specific_data;
            int i_format;

#define ASF_DEBUG 0

#define GUID_FMT "0x%x-0x%x-0x%x-0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
#define GUID_PRINT( guid )  \
    (guid).v1,              \
    (guid).v2,              \
    (guid).v3,              \
    (guid).v4[0],(guid).v4[1],(guid).v4[2],(guid).v4[3],    \
    (guid).v4[4],(guid).v4[5],(guid).v4[6],(guid).v4[7]

#ifdef ASF_DEBUG
   own_Dbg( 0,
             "\nfound audio object guid: " GUID_FMT " size:"I64Fd,
             GUID_PRINT( p_sp->i_stream_type ),
             p_sp->i_type_specific_data_length );
#endif


            es_format_Init( &fmt, AUDIO_ES, 0 );
            i_format = GetWLE( &p_data[0] );
            wf_tag_to_fourcc( i_format, &fmt.i_codec, NULL );
            fmt.audio.i_channels        = GetWLE(  &p_data[2] );
            fmt.audio.i_rate      = GetDWLE( &p_data[4] );
            fmt.i_bitrate         = GetDWLE( &p_data[8] ) * 8;
            fmt.audio.i_blockalign      = GetWLE(  &p_data[12] );
            fmt.audio.i_bitspersample   = GetWLE(  &p_data[14] );

            if( p_sp->i_type_specific_data_length > sizeof( WAVEFORMATEX ) &&
                i_format != WAVE_FORMAT_MPEGLAYER3 &&
                i_format != WAVE_FORMAT_MPEG )
            {
                fmt.i_extra = __MIN( GetWLE( &p_data[16] ),
                                     p_sp->i_type_specific_data_length -
                                     sizeof( WAVEFORMATEX ) );
                fmt.p_extra = malloc( fmt.i_extra );
                memcpy( fmt.p_extra, &p_data[sizeof( WAVEFORMATEX )],
                        fmt.i_extra );
            }

            tk->i_cat = AUDIO_ES;
            tk->p_es = 1; // es_out_Add( p_demux->out, &fmt );
            // es_format_Clean( &fmt );

            msg_Dbg( p_demux, "added new audio stream(codec:0x%x,ID:%d)",
                    GetWLE( p_data ), p_sp->i_stream_number );
        }
        else
        {

#ifdef ASF_DEBUG
    own_Dbg( 0,
             "\nfound audio object guid: " GUID_FMT " size:"I64Fd,
             GUID_PRINT( p_sp->i_stream_type ),
             p_sp->i_type_specific_data_length );
#endif


			tk->p_es = 1;
//			tk->i_cat = UNKNOWN_ES;
            tk->i_cat = AUDIO_ES;
            msg_Dbg( p_demux, "unknown stream(ID:%d), assumed to be Audio",
                     p_sp->i_stream_number );
        }
    }

    p_sys->i_data_begin = p_sys->p_root->p_data->i_object_pos + 50;
    if( p_sys->p_root->p_data->i_object_size != 0 )
    { /* local file */
        p_sys->i_data_end = p_sys->p_root->p_data->i_object_pos +
                                    p_sys->p_root->p_data->i_object_size;
    }
    else
    { /* live/broacast */
        p_sys->i_data_end = -1;
    }


    /* go to first packet */
    stream_Seek( p_demux->s, p_sys->i_data_begin );

    /* try to calculate movie time */
    if( p_sys->p_fp->i_data_packets_count > 0 )
    {
        int64_t i_count;
        int64_t i_size = own_stream_Size( p_demux->s );

 //       if( p_sys->i_data_end > 0 && i_size > p_sys->i_data_end )
 //       {
            i_size = p_sys->i_data_end;
 //       }

        /* real number of packets */
        i_count = ( i_size - p_sys->i_data_begin ) /
                  p_sys->p_fp->i_min_data_packet_size;

        /* calculate the time duration in micro-s */
        p_sys->i_length = (mtime_t)p_sys->p_fp->i_play_duration / 10 *
                   (mtime_t)i_count /
                   (mtime_t)p_sys->p_fp->i_data_packets_count;

        if( p_sys->i_length > 0 )
        {
            p_sys->i_bitrate = 8 * i_size * (int64_t)1000000 / p_sys->i_length;
        }
    }

    /* Create meta information */
    p_sys->meta = vlc_meta_New();

    if( ( p_cd = ASF_FindObject( p_sys->p_root->p_hdr,
                                 &asf_object_content_description_guid, 0 ) ) )
    {
        if( p_cd->psz_title && *p_cd->psz_title )
        {
            vlc_meta_Add( p_sys->meta, VLC_META_TITLE, p_cd->psz_title );
        }
        if( p_cd->psz_author && *p_cd->psz_author )
        {
             vlc_meta_Add( p_sys->meta, VLC_META_AUTHOR, p_cd->psz_author );
        }
        if( p_cd->psz_copyright && *p_cd->psz_copyright )
        {
            vlc_meta_Add( p_sys->meta, VLC_META_COPYRIGHT, p_cd->psz_copyright );
        }
        if( p_cd->psz_description && *p_cd->psz_description )
        {
            vlc_meta_Add( p_sys->meta, VLC_META_DESCRIPTION, p_cd->psz_description );
        }
        if( p_cd->psz_rating && *p_cd->psz_rating )
        {
            vlc_meta_Add( p_sys->meta, VLC_META_RATING, p_cd->psz_rating );
        }
    }
    for( i_stream = 0, i = 0; i < 128; i++ )
    {
        asf_object_codec_list_t *p_cl = ASF_FindObject( p_sys->p_root->p_hdr,
                                                        &asf_object_codec_list_guid, 0 );

        if( p_sys->track[i] )
        {
            vlc_meta_t *tk = vlc_meta_New();
            TAB_APPEND( p_sys->meta->i_track, p_sys->meta->track, tk );

            if( p_cl && i_stream < p_cl->i_codec_entries_count )
            {
                if( p_cl->codec[i_stream].psz_name &&
                    *p_cl->codec[i_stream].psz_name )
                {
                    vlc_meta_Add( tk, VLC_META_CODEC_NAME,
                                  p_cl->codec[i_stream].psz_name );
                }
                if( p_cl->codec[i_stream].psz_description &&
                    *p_cl->codec[i_stream].psz_description )
                {
                    vlc_meta_Add( tk, VLC_META_CODEC_DESCRIPTION,
                                  p_cl->codec[i_stream].psz_description );
                }
            }
            i_stream++;
        }
    }
	


    //es_out_Control( p_demux->out, ES_OUT_RESET_PCR );
    return VLC_SUCCESS;

error:
    ASF_FreeObjectRoot( p_demux->s, p_sys->p_root );
    return VLC_EGENERIC;
}
/*****************************************************************************
 *
 *****************************************************************************/
static void DemuxEnd( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    int         i;

    if( p_sys->p_root )
    {
        ASF_FreeObjectRoot( p_demux->s, p_sys->p_root );
        p_sys->p_root = NULL;
    }
    if( p_sys->meta )
    {
        vlc_meta_Delete( p_sys->meta );
        p_sys->meta = NULL;
    }

    for( i = 0; i < 128; i++ )
    {
        asf_track_t *tk = p_sys->track[i];

        if( tk )
        {
            if( tk->p_frame )
            {
                block_ChainRelease( tk->p_frame );
            }
            if( tk->p_es )
            {
				tk->p_es = 0;
 //               es_out_Del( p_demux->out, tk->p_es );
            }
            free( tk );
        }
        p_sys->track[i] = 0;
    }
}

