/*********************************************************************
Example source code for defining custom arg_xxx data types for the
argtable2 command line parser library. It shows how to make custom
arg_xxx data types with additional error checking capabilities.

The example code implements a custom arg_xxx data type which accepts
a <double> value that must lie within a given range of values.

    usage: argcustom [-v] <scalar> [-x <double>] [-y <double>]... [--help]
    
        <scalar>                  <double> value in range [0.0, 1.0]
        -x <double>               x coeff in range [-1.0, 1.0]
        -y <double>               y coeff in range [0.5, 0.9]
        --help                    print this help and exit

Copyright (C) 1998-2001,2003-2007 Stewart Heitmann
sheitmann@users.sourceforge.net

This is free software; you can redistribute it and/or
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
#include "argxxx.h"


int main(int argc, char **argv)
    {
    struct arg_xxx  *scalar  = arg_xxx1(NULL, NULL, "<scalar>",            0.0, 1.0, "<double> value in range [0.0, 1.0]");
    struct arg_xxx  *x       = arg_xxx0("x",  NULL, "<double>",           -1.0, 1.0, "x coeff in range [-1.0, 1.0]");
    struct arg_xxx  *y       = arg_xxxn("y",  NULL, "<double>", 0,argc+2,  0.5, 0.9, "y coeff in range [0.5, 0.9]");
    struct arg_lit  *help    = arg_lit0(NULL,"help",                                 "print this help and exit");
    struct arg_end  *end     = arg_end(20);
    void* argtable[] = {scalar,x,y,help,end};
    const char* progname = "argcustom";
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

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
        {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,progname);
        printf("Try '%s --help' for more information.\n",progname);
        exitcode=1;
        goto exit;
        }

    /* only get here is command line arguments were parsed sucessfully */
    printf("scalar = %f\n", scalar->data[0]);
    if (x->count > 0)
        printf("x = %f\n", x->data[0]);
    for (i=0; i<y->count; i++)
        printf("y[%d] = %f\n", i, y->data[i]);

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
