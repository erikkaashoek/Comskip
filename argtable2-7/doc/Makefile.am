dist_man_MANS=argtable2.3 argtable.3

docdir=@docdir@
dist_doc_DATA  = argtable2-advanced.html argtable2-advanced.pdf argtable2-advanced.ps 
dist_doc_DATA += argtable2-intro.html argtable2-intro.pdf argtable2-intro.ps
dist_doc_DATA += argtable2.html argtable2.pdf argtable2.ps argxxx.gif arg_int.gif


all:

#create a copy of the man page called "argtable" for those who forget its really called "argtable2"
argtable.3: argtable2.3
	cp argtable2.3 argtable.3	

#install-data-hook:
#	cd @prefix@/man/man3 ; ln -fs argtable2.3 argtable.3

#uninstall-hook:
#	-rm -f @prefix@/man/man3/argtable.3

maintainer-doc:
	groff -man -Thtml argtable2.3.in > argtable2.html.in
	groff -man -Tps argtable2.3 > argtable2.ps
	ps2pdf argtable2.ps argtable2.pdf

distclean: distclean-am
	rm -f Makefile argtable.3

maintainer-clean:
	rm -f argtable.3 argtable2.3 argtable2.html argtable2.html.in argtable2.ps argtable2.pdf
