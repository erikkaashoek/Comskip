/*
 * Faad decoder
 * Copyright (c) 2003 Zdenek Kabelac.
 * Copyright (c) 2004 Thomas Raivio.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file faad.c
 * AAC decoder.
 *
 * still a bit unfinished - but it plays something
 */

//#define FAAD2_VERSION

//#include "avcodec.h"
#include "faad.h"

#ifndef FAADAPI
#define FAADAPI
#endif

/*
 * when CONFIG_LIBFAADBIN is defined the libfaad will be opened at runtime
 */
//#undef CONFIG_LIBFAADBIN
//#define CONFIG_LIBFAADBIN


typedef struct {
    void* handle;               /* dlopen handle */
    void* faac_handle;          /* FAAD library handle */
    int sample_size;
    int init;
} FAACContext;

static FAACContext sc;

static FAACContext *s = &sc;
int faad_sample_rate;
int faad_channels;

static const unsigned long faac_srates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

static void channel_setup()
{
#ifdef FAAD2_VERSION
//    FAACContext *s = avctx->priv_data;
        faacDecConfigurationPtr faac_cfg;

        faac_cfg = faacDecGetCurrentConfiguration(s->faac_handle);
        faac_cfg->downMatrix = 1;
        faacDecSetConfiguration(s->faac_handle, faac_cfg);

#endif
}

/*
static int faac_init_mp4(AVCodecContext *avctx)
{
    FAACContext *s = avctx->priv_data;
    unsigned long samplerate;
#ifndef FAAD2_VERSION
    unsigned long channels;
#else
    unsigned char channels;
#endif
    int r = 0;

    if (avctx->extradata){
        r = s->faacDecInit2(s->faac_handle, (uint8_t*) avctx->extradata,
                            avctx->extradata_size,
                            &samplerate, &channels);
        if (r < 0){
            av_log(avctx, AV_LOG_ERROR,
                   "faacDecInit2 failed r:%d   sr:%ld  ch:%ld  s:%d\n",
                   r, samplerate, (long)channels, avctx->extradata_size);
        } else {
            avctx->sample_rate = samplerate;
            avctx->channels = channels;
            channel_setup();
            s->init = 1;
        }
    }

    return r;
}
*/

int faac_test_frame(unsigned char *buf, int buf_size)
{
 //   FAACContext *s = avctx->priv_data;
    faacDecFrameInfo frame_info;
    void *out;

	if (s->faac_handle ==0) {
		faac_decode_init();
	}
    if(!s->init){
        unsigned long srate;
        unsigned char channels;
        int r = faacDecInit(s->faac_handle, buf, buf_size, &srate, &channels);
        if(r < 0){
            return -0;
        }
        faad_sample_rate = srate;
        faad_channels = channels;
        channel_setup();
        s->init = 1;
    }
    out = faacDecDecode(s->faac_handle, &frame_info, (unsigned char*)buf, (unsigned long)buf_size);

    if (frame_info.error > 0) {
        Debug(2, "faac: frame decoding failed: %s\n",
               faacDecGetErrorMessage(frame_info.error));
        return 0;
    }
	return 1;
}




int faac_decode_frame(       void *data, int *data_size,
                             unsigned char *buf, int buf_size)
{
 //   FAACContext *s = avctx->priv_data;
#ifndef FAAD2_VERSION
    unsigned long bytesconsumed;
    short *sample_buffer = NULL;
    unsigned long samples;
    int out;
#else
    faacDecFrameInfo frame_info;
    void *out;
#endif
    if(buf_size == 0)
        return 0;
#ifndef FAAD2_VERSION
    out = s->faacDecDecode(s->faac_handle,
                           (unsigned char*)buf,
                           &bytesconsumed,
                           data,
                           &samples);
    samples *= s->sample_size;
    if (data_size)
        *data_size = samples;
    return (buf_size < (int)bytesconsumed)
        ? buf_size : (int)bytesconsumed;
#else
	if (s->faac_handle ==0) {
		faac_decode_init();
	}
    if(!s->init){
        unsigned long srate;
        unsigned char channels;
        int r = faacDecInit(s->faac_handle, buf, buf_size, &srate, &channels);
        if(r < 0){
            Debug(1, "faac: codec init failed.\n");
            return -2;
        }
        faad_sample_rate = srate;
        faad_channels = channels;
        channel_setup();
        s->init = 1;
    }

    out = faacDecDecode(s->faac_handle, &frame_info, (unsigned char*)buf, (unsigned long)buf_size);

    if (frame_info.error > 0) {
        Debug(2, "faac: frame decoding failed: %s\n",
               faacDecGetErrorMessage(frame_info.error));
        return -1;
    }
/*
    if (!avctx->frame_size)
        avctx->frame_size = frame_info.samples/avctx->channels;
 */
    frame_info.samples *= s->sample_size;
    memcpy(data, out, frame_info.samples); // CHECKME - can we cheat this one

    if (data_size)
        *data_size = frame_info.samples;

    return (buf_size < (int)frame_info.bytesconsumed)
        ? buf_size : (int)frame_info.bytesconsumed;
#endif
}

int faac_decode_end()
{
//    FAACContext *s = avctx->priv_data;

    faacDecClose(s->faac_handle);

    return 0;
}

int faac_decode_init()
{
//    FAACContext *s = avctx->priv_data;
    faacDecConfigurationPtr faac_cfg;

    s->faac_handle = faacDecOpen();
    if (!s->faac_handle) {
        Debug(1,  "FAAD library: cannot create handler!\n");
        faac_decode_end();
        return -1;
    }


    faac_cfg = faacDecGetCurrentConfiguration(s->faac_handle);

    if (faac_cfg) {
#ifdef FAAD2_VERSION
            faac_cfg->outputFormat = FAAD_FMT_16BIT;
#endif
            s->sample_size = 2;

        faac_cfg->defSampleRate = 44100 ;
        faac_cfg->defObjectType = LC;
    }

    faacDecSetConfiguration(s->faac_handle, faac_cfg);

//    faac_init_mp4();

//    if(!s->init && avctx->channels > 0)
//      channel_setup(avctx);

 //   avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}
#undef AAC_CODEC
