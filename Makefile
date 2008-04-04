####### Compiler, tools and options

CC       = cc
CXX      = c++
LEX      = flex
YACC     = yacc
ARCHFLAGS= -force_cpusubtype_ALL -arch i386 -arch ppc7400
CFLAGS   = -g -pipe -Wall -W -O2 -fasm-blocks -Wno-unused  -DHAVE_CONFIG_H $(ARCHFLAGS)  
CXXFLAGS = -g -pipe -Wall -W -O2  -DHAVE_CONFIG_H $(ARCHFLAGS)
LEXFLAGS = 
YACCFLAGS= -d
INCPATH  = -I. -Ilibmpeg2 -IAC3Dec -Iargtable2-7/src
LINK     = c++
LFLAGS   = -headerpad_max_install_names -prebind $(ARCHFLAGS)
LIBS     = $(SUBLIBS)  
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

HEADERS = avcodec.h \
		bswap.h \
		commercial_skip.h \
		common.h \
		comskip.h \
		config.h \
		dsputil.h \
		gettimeofday.h \
		inttypes.h \
		mpegaudio.h \
		mpegaudiodectab.h \
		mpegaudiotab.h \
		mpegvideo.h \
		rational.h \
		resource.h \
		argtable2-7/src/argtable2.h \
		AC3Dec/ac3.h \
		AC3Dec/bitstream.h \
		AC3Dec/global.h \
		AC3Dec/mpalib.h \
		libmpeg2/alpha_asm.h \
		libmpeg2/attributes.h \
		libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mmx.h \
		libmpeg2/mpeg2.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/vlc.h
SOURCES = comskip.c \
		dump_state.c \
		getbits.c \
		gettimeofday.c \
		mem.c \
		mpeg2dec.c \
		mpegaudiodec.c \
		utils.c \
		video_out_dx.c \
		argtable2-7/src/arg_date.c \
		argtable2-7/src/arg_dbl.c \
		argtable2-7/src/arg_end.c \
		argtable2-7/src/arg_file.c \
		argtable2-7/src/arg_int.c \
		argtable2-7/src/arg_lit.c \
		argtable2-7/src/arg_rem.c \
		argtable2-7/src/arg_rex.c \
		argtable2-7/src/arg_str.c \
		argtable2-7/src/argtable2.c \
		AC3Dec/bit_allocate.c \
		AC3Dec/bitstream.c \
		AC3Dec/coeff.c \
		AC3Dec/crc.c \
		AC3Dec/decode.c \
		AC3Dec/downmix.c \
		AC3Dec/exponent.c \
		AC3Dec/imdct.c \
		AC3Dec/parse.c \
		AC3Dec/rematrix.c \
		AC3Dec/sanity_check.c \
		libmpeg2/alloc.c \
		libmpeg2/cpu_accel.c \
		libmpeg2/cpu_state.c \
		libmpeg2/decode2.c \
		libmpeg2/header.c \
		libmpeg2/idct.c \
		libmpeg2/idct_alpha.c \
		libmpeg2/idct_altivec.c \
		libmpeg2/idct_mmx.c \
		libmpeg2/motion_comp.c \
		libmpeg2/motion_comp_alpha.c \
		libmpeg2/motion_comp_altivec.c \
		libmpeg2/motion_comp_mmx.c \
		libmpeg2/motion_comp_vis.c \
		libmpeg2/slice.c
OBJECTS = comskip.o \
		dump_state.o \
		getbits.o \
		gettimeofday.o \
		mem.o \
		mpeg2dec.o \
		mpegaudiodec.o \
		utils.o \
		video_out_dx.o \
		arg_date.o \
		arg_dbl.o \
		arg_end.o \
		arg_file.o \
		arg_int.o \
		arg_lit.o \
		arg_rem.o \
		arg_rex.o \
		arg_str.o \
		argtable2.o \
		bit_allocate.o \
		bitstream.o \
		coeff.o \
		crc.o \
		decode.o \
		downmix.o \
		exponent.o \
		imdct.o \
		parse.o \
		rematrix.o \
		sanity_check.o \
		alloc.o \
		cpu_accel.o \
		cpu_state.o \
		decode2.o \
		header.o \
		idct.o \
		idct_alpha.o \
		idct_altivec.o \
		idct_mmx.o \
		motion_comp.o \
		motion_comp_alpha.o \
		motion_comp_altivec.o \
		motion_comp_mmx.o \
		motion_comp_vis.o \
		slice.o
FORMS = 
UICDECLS = 
UICIMPLS = 
SRCMOC   = 
OBJMOC = 
DIST	   = comskip.pro
QMAKE_TARGET = comskip
DESTDIR  = ../../
TARGET   = ../../comskip

first: all
####### Implicit rules

.SUFFIXES: .c .o .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)  
	test -d ../../ || mkdir -p ../../
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)

mocables: $(SRCMOC)
uicables: $(UICDECLS) $(UICIMPLS)



dist: 
	@mkdir -p .tmp/comskip && $(COPY_FILE) --parents $(SOURCES) $(HEADERS) $(FORMS) $(DIST) .tmp/comskip/ && ( cd `dirname .tmp/comskip` && $(TAR) comskip.tar comskip && $(GZIP) comskip.tar ) && $(MOVE) `dirname .tmp/comskip`/comskip.tar.gz . && $(DEL_FILE) -r .tmp/comskip

mocclean:
uiclean:

yaccclean:
lexclean:
clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) ../../$(TARGET) $(TARGET)


FORCE:

####### Compile

comskip.o: comskip.c inttypes.h \
		config.h \
		argtable2-7/src/argtable2.h \
		comskip.h

dump_state.o: dump_state.c config.h \
		inttypes.h

getbits.o: getbits.c avcodec.h \
		common.h \
		rational.h \
		config.h \
		inttypes.h \
		bswap.h

gettimeofday.o: gettimeofday.c config.h \
		gettimeofday.h

mem.o: mem.c avcodec.h \
		common.h \
		rational.h \
		config.h \
		inttypes.h \
		bswap.h

mpeg2dec.o: mpeg2dec.c inttypes.h \
		config.h \
		AC3Dec/ac3.h \
		comskip.h

mpegaudiodec.o: mpegaudiodec.c common.h \
		avcodec.h \
		mpegaudio.h \
		mpegaudiodectab.h \
		config.h \
		inttypes.h \
		bswap.h \
		rational.h

utils.o: utils.c avcodec.h \
		dsputil.h \
		mpegvideo.h \
		common.h \
		rational.h \
		config.h \
		inttypes.h \
		bswap.h

video_out_dx.o: video_out_dx.c resource.h

arg_date.o: argtable2-7/src/arg_date.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_date.o argtable2-7/src/arg_date.c

arg_dbl.o: argtable2-7/src/arg_dbl.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_dbl.o argtable2-7/src/arg_dbl.c

arg_end.o: argtable2-7/src/arg_end.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_end.o argtable2-7/src/arg_end.c

arg_file.o: argtable2-7/src/arg_file.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_file.o argtable2-7/src/arg_file.c

arg_int.o: argtable2-7/src/arg_int.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_int.o argtable2-7/src/arg_int.c

arg_lit.o: argtable2-7/src/arg_lit.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_lit.o argtable2-7/src/arg_lit.c

arg_rem.o: argtable2-7/src/arg_rem.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_rem.o argtable2-7/src/arg_rem.c

arg_rex.o: argtable2-7/src/arg_rex.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_rex.o argtable2-7/src/arg_rex.c

arg_str.o: argtable2-7/src/arg_str.c config.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o arg_str.o argtable2-7/src/arg_str.c

argtable2.o: argtable2-7/src/argtable2.c config.h \
		argtable2-7/src/getopt.h \
		argtable2-7/src/argtable2.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o argtable2.o argtable2-7/src/argtable2.c

bit_allocate.o: AC3Dec/bit_allocate.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o bit_allocate.o AC3Dec/bit_allocate.c

bitstream.o: AC3Dec/bitstream.c AC3Dec/ac3.h \
		AC3Dec/bitstream.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o bitstream.o AC3Dec/bitstream.c

coeff.o: AC3Dec/coeff.c AC3Dec/ac3.h \
		AC3Dec/bitstream.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o coeff.o AC3Dec/coeff.c

crc.o: AC3Dec/crc.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o crc.o AC3Dec/crc.c

decode.o: AC3Dec/decode.c AC3Dec/ac3.h \
		AC3Dec/bitstream.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o decode.o AC3Dec/decode.c

downmix.o: AC3Dec/downmix.c AC3Dec/global.h \
		AC3Dec/ac3.h \
		resource.h \
		AC3Dec/mpalib.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o downmix.o AC3Dec/downmix.c

exponent.o: AC3Dec/exponent.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o exponent.o AC3Dec/exponent.c

imdct.o: AC3Dec/imdct.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o imdct.o AC3Dec/imdct.c

parse.o: AC3Dec/parse.c AC3Dec/ac3.h \
		AC3Dec/bitstream.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o parse.o AC3Dec/parse.c

rematrix.o: AC3Dec/rematrix.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o rematrix.o AC3Dec/rematrix.c

sanity_check.o: AC3Dec/sanity_check.c AC3Dec/ac3.h \
		config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o sanity_check.o AC3Dec/sanity_check.c

alloc.o: libmpeg2/alloc.c libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o alloc.o libmpeg2/alloc.c

cpu_accel.o: libmpeg2/cpu_accel.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o cpu_accel.o libmpeg2/cpu_accel.c

cpu_state.o: libmpeg2/cpu_state.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/mmx.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o cpu_state.o libmpeg2/cpu_state.c

decode2.o: libmpeg2/decode2.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o decode2.o libmpeg2/decode2.c

header.o: libmpeg2/header.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o header.o libmpeg2/header.c

idct.o: libmpeg2/idct.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o idct.o libmpeg2/idct.c

idct_alpha.o: libmpeg2/idct_alpha.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/alpha_asm.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o idct_alpha.o libmpeg2/idct_alpha.c

idct_altivec.o: libmpeg2/idct_altivec.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o idct_altivec.o libmpeg2/idct_altivec.c

idct_mmx.o: libmpeg2/idct_mmx.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/mmx.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o idct_mmx.o libmpeg2/idct_mmx.c

motion_comp.o: libmpeg2/motion_comp.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o motion_comp.o libmpeg2/motion_comp.c

motion_comp_alpha.o: libmpeg2/motion_comp_alpha.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/alpha_asm.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o motion_comp_alpha.o libmpeg2/motion_comp_alpha.c

motion_comp_altivec.o: libmpeg2/motion_comp_altivec.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o motion_comp_altivec.o libmpeg2/motion_comp_altivec.c

motion_comp_mmx.o: libmpeg2/motion_comp_mmx.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/mmx.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o motion_comp_mmx.o libmpeg2/motion_comp_mmx.c

motion_comp_vis.o: libmpeg2/motion_comp_vis.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o motion_comp_vis.o libmpeg2/motion_comp_vis.c

slice.o: libmpeg2/slice.c libmpeg2/config.h \
		libmpeg2/inttypes.h \
		libmpeg2/mpeg2.h \
		libmpeg2/attributes.h \
		libmpeg2/mpeg2_internal.h \
		libmpeg2/vlc.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o slice.o libmpeg2/slice.c

####### Install

install:  

uninstall:  

