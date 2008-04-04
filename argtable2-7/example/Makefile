#######################################################################
# This file is part of the argtable2 library.
# Copyright (C) 1998-2001,2003-2007 Stewart Heitmann
# sheitmann@users.sourceforge.net
#
# The argtable2 library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
# USA.
#######################################################################
#
#SHELL  = /bin/sh

#the default make rule for compiling C source code is
# xxx.o xxx.c:
#   $(CC) -c $(CFLAGS) -o xxx.o xxx.c


CC        = gcc
#add the -static switch to CFLAGS to force static linking if desired
CFLAGS    += -I/usr/local/include -g -Wall
LDFLAGS   += -L/usr/local/lib
#LDLIBS    += -largtable2 -Wl,-rpath=/usr/local/lib
LDLIBS    += -largtable2

TARGETS = uname rm mv echo ls myprog myprog_C89 multisyntax argcustom callbacks hasoptvalue

all: $(TARGETS)

argcustom: argcustom.o argxxx.o
	  $(CC) $(LDFLAGS) argcustom.o argxxx.o -o $@ $(LDLIBS)

clean:
	rm -f core core.* *.o *~ $(TARGETS)

distclean: clean
	rm -f Makefile



