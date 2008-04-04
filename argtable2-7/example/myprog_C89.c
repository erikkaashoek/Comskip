/*********************************************************************
This example source code is an alternate version of myprog.c
that adheres to ansi C89 standards rather than ansi C99.
The only difference being that C89 does not permit the argtable array
to be statically initialized with the contents of variables set at
runtime whereas C99 does.
Hence we cannot declare and initialize the argtable array in one declaration as
    void* argtable[] = {list,recurse,repeat,defines,outfile,verbose,help,version,infiles,end};
Instead, we must declare 
    void* argtable[10];
and initialize the contents of the array separately.


This file is part of the argtable2 library.
Copyright (C) 1998-2001,2003-2007 Stewart Heitmann
sheitmann@users.sourceforge.net

The argtable2 library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
**********************************************************************/
#include "argtable2.h"

int mymain(int l, int R, int k,
           const char **defines, int ndefines,
           const char *outfile,
           int v,
           const char **infiles, int ninfiles)
    {
    int i;

    if (l>0) printf("list files (-l)\n");
    if (R>0) printf("recurse through directories (-R)\n");
    if (v>0) printf("verbose is enabled (-v)\n");
    printf("scalar k=%d\n",k);
    printf("output is \"%s\"\n", outfile);

    for (i=0; i<ndefines; i++)
        printf("user defined macro \"%s\"\n",defines[i]);

    for (i=0; i<ninfiles; i++)
        printf("infile[%d]=\"%s\"\n",i,infiles[i]);

    return 0;
    }


int main(int argc, char **argv)
    {
    struct arg_lit  *list    = arg_lit0("lL",NULL,                      "list files");
    struct arg_lit  *recurse = arg_lit0("R",NULL,                       "recurse through subdirectories");
    struct arg_int  *repeat  = arg_int0("k","scalar",NULL,              "define scalar value k (default is 3)");
    struct arg_str  *defines = arg_strn("D","define","MACRO",0,argc+2,  "macro definitions");
    struct arg_file *outfile = arg_file0("o",NULL,"<output>",           "output file (default is \"-\")");
    struct arg_lit  *verbose = arg_lit0("v","verbose,debug",            "verbose messages");
    struct arg_lit  *help    = arg_lit0(NULL,"help",                    "print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version",                 "print version information and exit");
    struct arg_file *infiles = arg_filen(NULL,NULL,NULL,1,argc+2,       "input file(s)");
    struct arg_end  *end     = arg_end(20);
    void* argtable[10];
    const char* progname = "myprog_C89";
    int nerrors;
    int exitcode=0;

    /* initialize the argtable array with ptrs to the arg_xxx structures constructed above */
    argtable[0] = list;
    argtable[1] = recurse;
    argtable[2] = repeat;
    argtable[3] = defines;
    argtable[4] = outfile;
    argtable[5] = verbose;
    argtable[6] = help;
    argtable[7] = version;
    argtable[8] = infiles;
    argtable[9] = end;
        
    /* verify the argtable[] entries were allocated sucessfully */
    if (arg_nullcheck(argtable) != 0)
        {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exitcode=1;
        goto exit;
        }

    /* set any command line default values prior to parsing */
    repeat->ival[0]=3;
    outfile->filename[0]="-";

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
        {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout,argtable,"\n");
        printf("This program demonstrates the use of the argtable2 library\n");
        printf("for parsing command line arguments.\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exitcode=0;
        goto exit;
        }

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0)
        {
        printf("'%s' example program for the \"argtable\" command line argument parser.\n",progname);
        printf("September 2003, Stewart Heitmann\n");
        exitcode=0;
        goto exit;
        }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
        {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,progname);
        printf("Try '%s --help' for more information.\n",progname);
        exitcode=1;
        goto exit;
        }

    /* special case: uname with no command line options induces brief help */
    if (argc==1)
        {
        printf("Try '%s --help' for more information.\n",progname);
        exitcode=0;
        goto exit;
        }

    /* normal case: take the command line options at face value */
    exitcode = mymain(list->count, recurse->count, repeat->ival[0],
                      defines->sval, defines->count,
                      outfile->filename[0], verbose->count,
                      infiles->filename, infiles->count);

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
