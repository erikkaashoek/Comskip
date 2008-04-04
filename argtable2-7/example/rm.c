/*********************************************************************
Example source code for using the argtable2 library to implement:

   rm [-dfirv] [--help] [--version] <file> [<file>]...

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

/* Here we only simulate the rm functionality */
void mymain(int d, int f, int i, int r, int v, const char** files, int nfiles)
    {
    int j;

    /* if verbose option was given then display all option settings */
    if (v)
        {
        printf("remove directories  = %s\n", ((d)?"YES":"NO"));
        printf("force removal       = %s\n", ((f)?"YES":"NO"));
        printf("interactive mode    = %s\n", ((i)?"YES":"NO"));
        printf("recurse directories = %s\n", ((r)?"YES":"NO"));
        printf("verbose messages    = %s\n", ((v)?"YES":"NO"));
        }
    /* print the filenames that would be deleted if this really was the "rm" function */
    for (j=0; j<nfiles; j++)
        printf("unlinking \"%s\"\n", files[j]);
    printf("%d files total\n",nfiles);
    }


int main(int argc, char **argv)
    {
    const char* progname = "rm";
    struct arg_lit  *dir   = arg_lit0("d", "directory",   "unlink file(s), even if it is a non-empty directory");
    struct arg_rem  *dir2  = arg_rem( NULL,               "(super-user only)");
    struct arg_lit  *force = arg_lit0("f", "force",       "ignore nonexistant files, never prompt");
    struct arg_lit  *inter = arg_lit0("i", "interactive", "prompt before any removal");
    struct arg_lit  *recur = arg_lit0("rR","recursive",   "remove the contents of directories recursively");
    struct arg_lit  *verb  = arg_lit0("v", "verbose",     "explain what is being done");
    struct arg_lit  *help  = arg_lit0(NULL,"help",        "print this help and exit");
    struct arg_lit  *vers  = arg_lit0(NULL,"version",     "print version information and exit");
    struct arg_file *files = arg_filen(NULL,NULL,NULL,1,argc+2,NULL);
    struct arg_end  *end   = arg_end(20);
    void* argtable[] = {dir,dir2,force,inter,recur,verb,help,vers,files,end};
    int exitcode=0;
    int nerrors;

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
        printf("Remove (unlink) the specified file(s).\n\n");
        arg_print_glossary(stdout,argtable,"  %-20s %s\n");
        printf("\nReport bugs to <no-one> as this is just an example program.\n");
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

    /* command line options are all ok, now perform the "rm" functionality */
    mymain(dir->count, force->count, inter->count, recur->count, verb->count, files->filename, files->count);

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
