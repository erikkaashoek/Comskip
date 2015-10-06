####### Compiler, tools and options

CC       = cc
LEX      = flex
YACC     = yacc
ARCHFLAGS=
DEFINES  = -DDONATOR -DPROCESS_CC
CFLAGS   = -g -pipe -Wall -W -Wno-address -Wno-unused-parameter -Wno-unused-label -Wno-unused-result -O2 $(DEFINES) $(ARCHFLAGS)
LEXFLAGS =
YACCFLAGS= -d
INCPATH  = -I. $(INCLUDES)
LINK     = $(CC) -g
LFLAGS   = $(ARCHFLAGS)
LIBS     =
SHLIBS = -lavutil -lavformat -lavcodec -largtable2
DESTDIR = /usr/local
AR       = ar cq
RANLIB   = ranlib -s
QMAKE    = qmake
TAR      = tar -cf
GZIP     = gzip -9f
COPY     = cp -f
COPY_FILE= cp -f
COPY_DIR = cp -f -r
INSTALL_FILE= $(COPY_FILE)
INSTALL_DIR = $(COPY_DIR)
DEL_FILE = rm -f
SYMLINK  = ln -sf
DEL_DIR  = rmdir
MOVE     = mv -f
CHK_DIR_EXISTS= test -d
MKDIR    = mkdir -p

####### Output directory

OBJECTS_DIR = ./

####### Files

OBJECTS = comskip.o \
		platform.o \
		mpeg2dec.o \
		video_out_dx.o \
		ccextratorwin/608.o \
		ccextratorwin/ccextractor.o \
		ccextratorwin/encoding.o \
		ccextratorwin/general_loop.o \
		ccextratorwin/myth.o
DIST	   = comskip.pro
TARGET   = comskip

####### Platform specific

ifneq (,$(findstring Windows,$(OS)))
	PLATFORMLIBS = -L./vendor -lgdi32 -lcomdlg32
	PLATFORMINCS = -I./vendor
	OBJECTS += win32_pthread.o
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		SHLIBS += -pthread -lm
	endif
endif

ifneq (,$(DEBUG))
	CFLAGS += -ggdb -O0
endif

####### Implicit rules

first: all

.SUFFIXES: .c .o .cpp .cc .cxx .C

.c.o:
	$(CC) -c $(CFLAGS) $(PLATFORMINCS) $(INCPATH) -o $@ $<

####### Build rules

ifneq (,$(shell sdl-config --cflags))
comskip-sdl.o: comskip.o
	$(CC) -c $(CFLAGS) $(PLATFORMINCS) $(INCPATH) $(shell sdl-config --cflags) -DHAVE_SDL -o $@ comskip.c

mpeg2dec-sdl.o: mpeg2dec.o
	$(CC) -c $(CFLAGS) $(PLATFORMINCS) $(INCPATH) $(shell sdl-config --cflags) -DHAVE_SDL -o $@ mpeg2dec.c

comskip-gui: $(TARGET) comskip-sdl.o mpeg2dec-sdl.o video_out_sdl.o
	$(LINK) $(LFLAGS) -o $@ $(filter-out comskip.o mpeg2dec.o,$(OBJECTS)) comskip-sdl.o mpeg2dec-sdl.o video_out_sdl.o $(PLATFORMLIBS) $(LIBS) $(SHLIBS) $(shell sdl-config --libs)

all: comskip-gui
endif

all: Makefile $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) $(LFLAGS) -o $@ $(OBJECTS) $(PLATFORMLIBS) $(LIBS) $(SHLIBS)

votest: video_out_dx.c video_out_sdl.c
	$(LINK) $(CFLAGS) $(PLATFORMINCS) $(INCPATH) -DTEST $(LFLAGS) -o votest video_out_dx.c video_out_sdl.c $(PLATFORMLIBS) $(LIBS) $(SHLIBS)

clean:
	-$(DEL_FILE) $(OBJECTS)

####### Compile

comskip.o: comskip.c platform.h \
		config.h \
		comskip.h

mpeg2dec.o: mpeg2dec.c platform.h \
		config.h \
		comskip.h

platform.o: platform.c platform.h

video_out_dx.o: video_out_dx.c resource.h

video_out_sdl.o: video_out_sdl.c

####### Install

install: $(TARGET)
	$(COPY_FILE) $(TARGET) $(DESTDIR)/bin/$(TARGET)

uninstall:
