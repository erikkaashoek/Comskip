/*********************************************************************
Example source code for using the argtable2 library to implement:

    uname [-asnrvmpio] [--help] [--version]

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

/* Here we simulate the uname functionality */
int mymain(int kname, int nname, int krel, int kver, int mach, int proc, int hard, int opsys)
    {
    if (kname) printf("Linux ");
    if (nname) printf("localhost.localdomain ");
    if (krel)  printf("2.4.19-16 ");
    if (kver)  printf("#1 Fri Sep 20 18:15:05 CEST 2002 ");
    if (mach)  printf("i686 ");
    if (proc)  printf("Intel ");
    if (hard)  printf("unknown ");
    if (opsys) printf("GNU/Linux ");
    printf("\n");
    return 0;
    }


int main(int argc, char **argv)
    {
    const char* progname = "uname";
    struct arg_lit *all   = arg_lit0("a", "all",              "print all information, in the following order:");
    struct arg_lit *kname = arg_lit0("s", "kernel-name",      "print the kernel name");
    struct arg_lit *nname = arg_lit0("n", "nodename",         "print the node name");
    struct arg_lit *krel  = arg_lit0("r", "kernel-release",   "print the kernel release");
    struct arg_lit *kver  = arg_lit0("v", "kernel-version",   "print the kernel version");
    struct arg_lit *mach  = arg_lit0("m", "machine",          "print the machine hardware name");
    struct arg_lit *proc  = arg_lit0("p", "processor",        "print the processor type");
    struct arg_lit *hard  = arg_lit0("i", "hardware-platform","print the hardware platform");
    struct arg_lit *opsys = arg_lit0("o", "operating-system", "print the operating system");
    struct arg_lit *help  = arg_lit0(NULL,"help",             "print this help and exit");
    struct arg_lit *vers  = arg_lit0(NULL,"version",          "print version information and exit");
    struct arg_end *end   = arg_end(20);
    void* argtable[] = {all,kname,nname,krel,kver,mach,proc,hard,opsys,help,vers,end};
    int nerrors;
    int exitcode=0;

    /* verify the argtable[] entries were allocated sucessfully */
    if (arg_nullcheck(argtable) != 0)
        {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exitcode=1;
        goto exit;
        }

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
        {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout,argtable,"\n");
        printf("Print certain system information.  With no options, same as -s.\n\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        printf("\nReport bugs to <foo@bar>.\n");
        exitcode=0;
        goto exit;
        }

    /* special case: '--version' takes precedence error reporting */
    if (vers->count > 0)
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

    /* special case: uname with no command line options is equivalent to "uname -s" */
    if (argc==1)
        {
        exitcode = mymain(0,1,0,0,0,0,0,0);
        goto exit;
        }

    /* special case: "uname -a" is equivalent to "uname -snrvmpi" */
    if (all->count>0)
        {
        exitcode = mymain(1,1,1,1,1,1,1,1);
        goto exit;
        }

    /* normal case: take the command line options at face value */
    exitcode = mymain(kname->count, nname->count, krel->count, kver->count, mach->count, proc->count, hard->count, opsys->count);

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
