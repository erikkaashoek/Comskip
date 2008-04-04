SUBDIRS = src tests doc

exampledir=@exampledir@
dist_example_DATA  = example/Makefile example/Makefile.nmake example/README.txt
dist_example_DATA += example/argcustom.c
dist_example_DATA += example/argxxx.c
dist_example_DATA += example/argxxx.h
dist_example_DATA += example/callbacks.c
dist_example_DATA += example/echo.c
dist_example_DATA += example/hasoptvalue.c
dist_example_DATA += example/ls.c
dist_example_DATA += example/multisyntax.c
dist_example_DATA += example/mv.c
dist_example_DATA += example/myprog.c
dist_example_DATA += example/myprog_C89.c
dist_example_DATA += example/rm.c
dist_example_DATA += example/uname.c

# Extra rule to ensure the Windows-specific files are included in the distribution tarball.
# Unfortunately, "make distcheck" does not seem to honour this hook although "make dist" does.
dist-hook:
	cp -p src/Makefile.nmake $(distdir)/src
	cp -p src/argtable2.def $(distdir)/src

