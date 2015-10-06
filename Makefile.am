AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = comskip
if BUILD_GUI
    bin_PROGRAMS += comskip-gui
endif

comskip_SOURCES = comskip.c comskip.h mpeg2dec.c platform.c platform.h video_out_dx.c \
    ccextratorwin/608.c ccextratorwin/608.h \
    ccextratorwin/ccextractor.c ccextratorwin/ccextractor.h \
    ccextratorwin/encoding.c ccextratorwin/encoding.h \
    ccextratorwin/general_loop.c ccextratorwin/general_loop.h \
    ccextratorwin/myth.c ccextratorwin/myth.h

comskip_gui_SOURCES = comskip.c comskip.h mpeg2dec.c platform.c platform.h video_out_dx.c \
    video_out_sdl.c \
    ccextratorwin/608.c ccextratorwin/608.h \
    ccextratorwin/ccextractor.c ccextratorwin/ccextractor.h \
    ccextratorwin/encoding.c ccextratorwin/encoding.h \
    ccextratorwin/general_loop.c ccextratorwin/general_loop.h \
    ccextratorwin/myth.c ccextratorwin/myth.h

DEFINES = -DPROCESS_CC
if ENABLE_DONATOR
    DEFINES += -DDONATOR
endif

if PLATFORM_WIN32
    PLATFORM_LIBS = -lcomdlg32
endif

comskip_CPPFLAGS =  $(argtable2_CFLAGS) $(ffmpeg_CFLAGS) $(DEFINES)
comskip_LDADD = $(argtable2_LIBS) $(ffmpeg_LIBS) $(PLATFORM_LIBS)
comskip_gui_CPPFLAGS = $(argtable2_CFLAGS) $(ffmpeg_CFLAGS) $(sdl_CFLAGS) $(DEFINES) -DHAVE_SDL
comskip_gui_LDADD = $(argtable2_LIBS) $(ffmpeg_LIBS) $(sdl_LIBS) $(PLATFORM_LIBS)

if ENABLE_STATIC
    comskip_LDFLAGS = -static
    comskip_gui_LDFLAGS = -static
endif
