DEFINES  = -DDONATOR -DPROCESS_CC

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = comskip

comskip_SOURCES = comskip.c comskip.h mpeg2dec.c platform.c platform.h video_out_dx.c \
  ccextratorwin/608.c ccextratorwin/608.h \
  ccextratorwin/ccextractor.c ccextratorwin/ccextractor.h \
  ccextratorwin/encoding.c ccextratorwin/encoding.h \
  ccextratorwin/general_loop.c ccextratorwin/general_loop.h \
  ccextratorwin/myth.c ccextratorwin/myth.h

comskip_CPPFLAGS =  $(argtable2_CFLAGS) $(ffmpeg_CFLAGS) $(DEFINES)
comskip_LDFLAGS = $(argtable2_LIBS) $(ffmpeg_LIBS)
