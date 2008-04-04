A note on ANSI C89 versus C99 compilers.
----------------------------------------
These example programs use a style of array initialization that was
introduced in the ANSI C99 standard. Older (C89 compliant) compilers
will not compile them.
The Open Watcom compiler, for instance, is known to have this problem.

If this affects you, then an easy workaround is to initialize your
arrays according to the C89 standard as demonstrated in the
"myprog_C89.c" program.

Thanks to Justin Dearing for pointing this out.

Stewart Heitmann, Jan 2004.
