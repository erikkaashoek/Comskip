#!/bin/sh

make distclean && ./configure --enable-donator CPPFLAGS=-DDEBUG CFLAGS="-ggdb3 -O0"
