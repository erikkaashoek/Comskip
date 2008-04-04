/*********************************************************************
Example source code for using the argtable2 library to implement:

Usage: hasoptvalue -f <int> [-f <int>] [-f <int>] -b [<int>] [-b [<int>]] [-b [<int>]] [--help] [--version]
This program demonstrates the use of the argtable2 library
  -f, --foo=<int>           takes an integer value (defaults to 9)
  -b, --bar=[<int>]         takes an optional integer value (defaults to 5)
  --help                    print this help and exit
  --version                 print version information and exit

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
#include <argtable2.h>


int main(int argc, char **argv)
    {
    struct arg_int  *foo     = arg_intn("f","foo", NULL,1,3,       "takes an integer value (defaults to 9)");
    struct arg_int  *bar     = arg_intn("b","bar", NULL,1,3,       "takes an optional integer value (defaults to 5)");
    struct arg_lit  *help    = arg_lit0(NULL,"help",                "print this help and exit");
    struct arg_lit  *version = arg_lit0(NULL,"version",             "print version information and exit");
    struct arg_end  *end     = arg_end(20);
    void* argtable[] = {foo,bar,help,version,end};
    const char* progname = "hasoptvalue";
    int nerrors;
    int exitcode=0;
    int i;

    /* verify the argtable[] entries were allocated sucessfully */
    if (arg_nullcheck(argtable) != 0)
        {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exitcode=1;
        goto exit;
        }

    /* set foo default values to 9 */
    for (i=0; i<foo->hdr.maxcount; i++)
        foo->ival[i]=9;

    /* allow bar to have optional values */
    /* and set bar default values to 5 */
    bar->hdr.flag |= ARG_HASOPTVALUE;
    for (i=0; i<bar->hdr.maxcount; i++)
        bar->ival[i]=5;

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
        {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout,argtable,"\n");
        printf("This program demonstrates the use of the argtable2 library\n");
        arg_print_glossary(stdout,argtable,"  %-25s %s\n");
        exitcode=0;
        goto exit;
        }

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0)
        {
        printf("'%s' example program for the \"argtable\" command line argument parser.\n",progname);
        printf("September 2005, Stewart Heitmann\n");
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

    /* command line arguments are successfully parsed at this point. */
    /* print what we have parsed */
    printf("%d instances of --foo detected on command line\n", foo->count);
    for (i=0; i<foo->hdr.maxcount; i++)
        printf("foo[%d] = %d\n", i, foo->ival[i]);         
    printf("%d instances of --bar detected on command line\n", bar->count);
    for (i=0; i<bar->hdr.maxcount; i++)
        printf("bar[%d] = %d\n", i, bar->ival[i]);         

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
