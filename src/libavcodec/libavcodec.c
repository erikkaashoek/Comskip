#include <windows.h>

#include "tlibavcodec.h"
#include <stdio.h>

//// #de fine DLLLoad(P, N) ((long)P) = (long) N

// # d e f ine DLLLoad(P) ((long)pt_##P) = (long)GetProcAddress(mymodule, "##P##")

AVCodecContext  *pCodecCtx;
static AVCodec         *pCodec;
AVFrame         *pFrame; 
static int             frameFinished;
//  int             numBytes;
//  uint8_t         *buffer; 



#define DLLLoad(P, N) ((long)P) = (long)GetProcAddress(mymodule, N)

// # d e f ine DLLLoad(P) ((long)pt_##P) = (long)GetProcAddress(mymodule, "##P##")




extern void av_log_set_level(int level);
int Load_libavcodec()
{
#undef RELEASE
#ifndef DONATORS
#ifndef _DEBUG
#define RELEASE
#endif
#endif
#ifndef RELEASE
/*
	HMODULE mymodule;
	mymodule = LoadLibrary("libavcodec.dll");
	if (mymodule == NULL)
		return(0);

	DLLLoad( pt_avcodec_init, "avcodec_init");
	DLLLoad( pt_avcodec_register_all, "avcodec_register_all");

	DLLLoad( pt_avcodec_find_decoder_by_name, "avcodec_find_decoder_by_name" );
	DLLLoad( pt_avcodec_alloc_context, "avcodec_alloc_context" );
	DLLLoad( pt_avcodec_open, "avcodec_open" );
	DLLLoad( pt_avcodec_alloc_frame, "avcodec_alloc_frame" );
	DLLLoad( pt_avcodec_decode_video, "avcodec_decode_video" );
	DLLLoad( pt_avcodec_thread_init, "avcodec_thread_init" );
*/


	avcodec_init();

    /* register all the codecs */
    avcodec_register_all();





  // Find the decoder for the video stream
	pCodec = avcodec_find_decoder_by_name("h264");
	if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }

	pCodecCtx = avcodec_alloc_context((TlibavcodecExt *) 0 );

	// Open codec
	if(avcodec_open(pCodecCtx, pCodec)<0)
		return -1; // Could not open codec

// 
//	av_log_set_level(50); 


//	avcodec_thread_init(pCodecCtx, 4);
	// Allocate video frame
	pFrame=avcodec_alloc_frame(); 	
	pCodecCtx->flags |= CODEC_FLAG_GRAY;
	pCodecCtx->lowres = 2;
	pCodecCtx->skip_loop_filter = AVDISCARD_ALL;
/*
    AVDISCARD_NONE   =-16, ///< discard nothing
    AVDISCARD_DEFAULT=  0, ///< discard useless packets like 0 size packets in avi
    AVDISCARD_NONREF =  8, ///< discard all non reference
    AVDISCARD_BIDIR  = 16, ///< discard all bidirectional frames
    AVDISCARD_NONKEY = 32, ///< discard all frames except keyframes
    AVDISCARD_ALL    = 48, ///< discard all
*/
//        avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size); 
#endif
	return(1);

}


int h264_decode_frame(char *data, int size)
{
	int done;
/*
#define BUFSIZE	1000000
static char buffer[BUFSIZE];
static int chars = 0;
	int i=0;
	int frame_found = 0;
	while (size > 0) {
		if (chars > 100 && size > 3 && data[0] == 0 && data[1] == 0 && data[2] == 1)
		{
			frame_found = 1;
*/
//			pCodecCtx->lowres=1;
#ifndef RELEASE

	__try {
#ifdef DONATORS
			done = avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, data, size); 
			if (done < 0) {
				frameFinished = 0;
			}
#else
#ifdef _DEBUG
			done = avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, data, size); 
			if (done < 0) {
				frameFinished = 0;
			}
#endif
#endif
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
	{ 
		Debug(0, "\nh.264 decoder raised an exception\n");
		avcodec_close(pCodecCtx); 
		Load_libavcodec(); 
		return(0);
	}
#endif
/*			
			chars = 0;
		}
		buffer[chars++] = *data++;
		size--;
	}
*/
	if (frameFinished)
	{
		return(1);
	}
	return(0);
}

/*
#include "stdafx.h"
#include "Tlibavcodec.h"
#include "Tdll.h"
#include "ffdebug.h"

const char_t* Tlibavcodec::idctNames[]=
{
 _l("auto"),
 _l("libmpeg2"),
 _l("simple MMX"),
 _l("Xvid"),
 _l("simple"),
 _l("integer"),
 _l("FAAN"),
 NULL
};
const char_t* Tlibavcodec::errorResiliences[]=
{
 _l("none"),
 _l("careful"),
 _l("compliant"),
 _l("aggressive"),
 _l("very aggressive"),
 NULL
};
const char_t* Tlibavcodec::errorConcealments[]=
{
 _l("none"),
 _l("guess MVS"),
 _l("deblock"),
 _l("guess MVS + deblock"),
 NULL
};
const Tlibavcodec::Tdia_size Tlibavcodec::dia_sizes[]=
{
 -3,_l("adaptive with size 3"),
 -2,_l("adaptive with size 2"),
 -1,_l("experimental"),
 0,_l("default"),
 1,_l("size 1 diamond"),
 2,_l("size 2 diamond"),
 3,_l("size 3 diamond"),
 4,_l("size 4 diamond"),
 5,_l("size 5 diamond"),
 6,_l("size 6 diamond"),
 0,NULL
};

//===================================== Tlibavcodec ====================================
Tlibavcodec::Tlibavcodec(const Tconfig *config):refcount(0)
{
 dll=new Tdll(_l("libavcodec.dll"),config);
 if (!dll->ok)
  {
   delete dll;
   dll=new Tdll(_l("libavcodec_dec.dll"),config);
   dec_only=true;
  }
 else
  dec_only=false;
 dll->loadFunction(avcodec_init,"avcodec_init");
 dll->loadFunction(dsputil_init,"dsputil_init");
 dll->loadFunction(avcodec_register_all,"avcodec_register_all");
 dll->loadFunction(avcodec_find_decoder,"avcodec_find_decoder");
 dll->loadFunction(avcodec_open0,"avcodec_open");
 dll->loadFunction(avcodec_alloc_context0,"avcodec_alloc_context");
 dll->loadFunction(avcodec_alloc_frame,"avcodec_alloc_frame");
 dll->loadFunction(avcodec_decode_video,"avcodec_decode_video");
 dll->loadFunction(avcodec_decode_audio,"avcodec_decode_audio");
 dll->loadFunction(avcodec_flush_buffers,"avcodec_flush_buffers");
 dll->loadFunction(avcodec_close0,"avcodec_close");
 //dll->loadFunction(av_free_static,"av_free_static");
 dll->loadFunction(av_log_set_callback,"av_log_set_callback");
 dll->loadFunction(av_log_get_callback,"av_log_get_callback");
 dll->loadFunction(avcodec_thread_init,"avcodec_thread_init");
 dll->loadFunction(avcodec_thread_free,"avcodec_thread_free");
 dll->loadFunction(av_free,"av_free");
 dll->loadFunction(avcodec_default_get_buffer,"avcodec_default_get_buffer");
 dll->loadFunction(avcodec_default_release_buffer,"avcodec_default_release_buffer");
 dll->loadFunction(avcodec_default_reget_buffer,"avcodec_default_reget_buffer");
 dll->loadFunction(avcodec_get_current_idct,"avcodec_get_current_idct");
 dll->loadFunction(avcodec_get_encoder_info,"avcodec_get_encoder_info");

 if (!dec_only)
  {
   dll->loadFunction(avcodec_find_encoder,"avcodec_find_encoder");
   dll->loadFunction(avcodec_encode_video,"avcodec_encode_video");
   dll->loadFunction(avcodec_encode_audio,"avcodec_encode_audio");
  }
 else
  {
   avcodec_find_encoder=NULL;
   avcodec_encode_video=NULL;
   avcodec_encode_audio=NULL;
  }

 ok=dll->ok;

 if (ok)
  {
   avcodec_init();
   avcodec_register_all();
   av_log_set_callback(avlog);
  }
}
Tlibavcodec::~Tlibavcodec()
{
 //if (dll->ok) av_free_static();
 delete dll;
}

int Tlibavcodec::lavcCpuFlags(void)
{
 int lavc_cpu_flags=FF_MM_FORCE;
 if (Tconfig::cpu_flags&FF_CPU_MMX)    lavc_cpu_flags|=FF_MM_MMX;
 if (Tconfig::cpu_flags&FF_CPU_MMXEXT) lavc_cpu_flags|=FF_MM_MMXEXT;
 if (Tconfig::cpu_flags&FF_CPU_SSE)    lavc_cpu_flags|=FF_MM_SSE;
 if (Tconfig::cpu_flags&FF_CPU_SSE2)   lavc_cpu_flags|=FF_MM_SSE2;
 if (Tconfig::cpu_flags&FF_CPU_3DNOW)  lavc_cpu_flags|=FF_MM_3DNOW;
 if (Tconfig::cpu_flags&FF_CPU_SSE3)   lavc_cpu_flags|=FF_MM_SSE3;
 if (Tconfig::cpu_flags&FF_CPU_SSSE3)  lavc_cpu_flags|=FF_MM_SSSE3;
 return lavc_cpu_flags;
}

AVCodecContext* Tlibavcodec::avcodec_alloc_context(TlibavcodecExt *ext)
{
 AVCodecContext *ctx=avcodec_alloc_context0();
 ctx->dsp_mask=Tconfig::lavc_cpu_flags;
 if (ext)
  ext->connectTo(ctx,this);
 ctx->postgain=1.0f;
 ctx->scenechange_factor=1;
 return ctx;
}
int Tlibavcodec::avcodec_open(AVCodecContext *avctx, AVCodec *codec)
{
 CAutoLock l(&csOpenClose);
 return avcodec_open0(avctx,codec);
}
int Tlibavcodec::avcodec_close(AVCodecContext *avctx)
{
 CAutoLock l(&csOpenClose);
 return avcodec_close0(avctx);
}

bool Tlibavcodec::getVersion(const Tconfig *config,ffstring &vers,ffstring &license)
{
 const char *x=text<char>("aaa");
 Tdll *dl=new Tdll(_l("libavcodec.dll"),config);
 if (!dl->ok)
  {
   delete dl;
   dl=new Tdll(_l("libavcodec_dec.dll"),config);
  }
 void (*av_getVersion)(char **version,char **build,char **datetime,const char* *license);
 dl->loadFunction(av_getVersion,"getVersion");
 bool res;
 if (av_getVersion)
  {
   res=true;
   char *version,*build,*datetime;const char *lic;
   av_getVersion(&version,&build,&datetime,&lic);
   vers=(const char_t*)text<char_t>(version)+
   // ffstring(", build ")+build+
   ffstring(_l(" ("))+(const char_t*)text<char_t>(datetime)+_l(")");
   license=text<char_t>(lic);
  }
 else
  {
   res=false;
   vers.clear();
   license.clear();
  }
 delete dl;
 return res;
}
bool Tlibavcodec::check(const Tconfig *config)
{
 return Tdll::check(_l("libavcodec.dll"),config) || Tdll::check(_l("libavcodec_dec.dll"),config);
}

void Tlibavcodec::avlog(AVCodecContext *avctx,int level,const char *fmt,va_list valist)
{
 DPRINTFvaA(fmt,valist);
}

void Tlibavcodec::avlogMsgBox(AVCodecContext *avctx,int level,const char *fmt,va_list valist)
{
 if (level > AV_LOG_ERROR)
  {
   DPRINTFvaA(fmt,valist);
  }
 else
  {
   char buf[1024];
   int len=_vsnprintf(buf,1023,fmt,valist);
   if (len>0)
    {
     buf[len]='\0';
     MessageBoxA(NULL,buf,"ffdshow libavcodec encoder error",MB_ICONERROR|MB_OK);
    }
  }
}

//=================================== TlibavcodecExt ===================================
void TlibavcodecExt::connectTo(AVCodecContext *ctx,Tlibavcodec *libavcodec)
{
 ctx->self=this;
 ctx->get_buffer=get_buffer;default_get_buffer=libavcodec->avcodec_default_get_buffer;
 ctx->reget_buffer=reget_buffer;default_reget_buffer=libavcodec->avcodec_default_reget_buffer;
 ctx->release_buffer=release_buffer;default_release_buffer=libavcodec->avcodec_default_release_buffer;
 ctx->handle_user_data=handle_user_data0;
}
int TlibavcodecExt::get_buffer(AVCodecContext *c, AVFrame *pic)
{
 int ret=c->self->default_get_buffer(c,pic);
 if (ret==0)
  c->self->onGetBuffer(pic);
 return ret;
}
int TlibavcodecExt::reget_buffer(AVCodecContext *c, AVFrame *pic)
{
 int ret=c->self->default_reget_buffer(c,pic);
 if (ret==0)
  c->self->onRegetBuffer(pic);
 return ret;
}
void TlibavcodecExt::release_buffer(AVCodecContext *c, AVFrame *pic)
{
 c->self->default_release_buffer(c,pic);
 c->self->onReleaseBuffer(pic);
}
void TlibavcodecExt::handle_user_data0(AVCodecContext *c, const uint8_t *buf,int buf_len)
{
 c->self->handle_user_data(buf,buf_len);
}
*/