/*********************************************************************
Example source code for using the argtable2 library to implement:

    myprog [-lRv] [-k <int>] [-D MACRO]... [-o <output>] [--help]
       [--version] <file> [<file>]...

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
#include <stdlib.h>


/* error codes returned by myscanfn and mycheckfn, and received by myerrorfn. */
/* The exact values are unimportant provided they are non-zero and consistent */
/* between those three functions.                                             */                            
enum {EMINCOUNT=1, EMAXCOUNT, EBADINT, EZEROVAL, EODDCOUNT, EBADSUM};

/* 
 * Argtable calls an arg_xxx structure's hdr.scanfn routine once for each matching
 * command line argument.
 * The scanfn routine must extract the argument data value from the argv[]
 * string passed in argval and write the result back into the arg_xxx data
 * array.
 * Error codes returned by this function are saved by argtable and later
 * passed to the errorfn callback.
 *
 * This particular myscanfn is for arg_int structs. For demonstration purposes
 * it performs additional error checking to verify that the integer value
 * extracted is non-zero. Other custom rules could also be implemented at
 * that point in the code.
 */ 
int myscanfn(struct arg_int *parent,  /* ptr to relevant arg_int structure in argument table */
             const char *argval)      /* ptr to current command line argv[] string */ 
    {
    int val;
    char *end;

    /* return EMAXCOUNT error if we have reached the upper limit of allowed arguments for parent arg_int struct. */
    /* This is a mandatory check that scanfn must alwasy perform */
    if (parent->count == parent->hdr.maxcount )
        return EMAXCOUNT;

    /* extract base10 integer from argval string into val, return EBADINT if conversion failed */
    val = (int)strtol(argval,&end,10);
    if (*end!=0)
        return EBADINT;

    /* check the value is non-zero (for example purposes) and return EZEROVAL if it is not the case*/
    if (val==0)
        return EZEROVAL;

    /* all checks are passed, store the value in parent's ival[] array and increment the count */
    parent->ival[parent->count++] = val;

    /* return zero to indicate success */
    return 0;
    }


/* 
 * Argtable calls an arg_xxx structure's hdr.checkfn routine upon completion
 * of parsing the entire command line. It allows post-parse checks to be performed,
 * such as verifying the minimum number of exepected arguments have been received.
 * Like scanfn, error codes returned by this function are saved by argtable and later
 * passed to the errorfn callback.
 * 
 * This particular mycheckfn is for arg_int structs. For demonstration purposes
 * it performs additional checking to ensure that the number of arguments is even,
 * and that all values given sum to exactly 100.
 */
int mycheckfn(struct arg_int *parent)
    {
    int i;
    long sum = 0;

    /* return EMINCOUNT if the minimum number of arguments is not present. */
    /* This is a mandatory check that checkfn must alwasy perform. */
    if (parent->count < parent->hdr.mincount)
        return EMINCOUNT;

    /* return EODDCOUNT if the number of argument occurences is not even (for example purposes) */
    if (parent->count % 2 != 0)
        return EODDCOUNT;

    /* return EBADSUM if the sum of the arguments is not 100 (for example purposes) */
    for (i=0; i<parent->count; i++)
        sum += parent->ival[i];
    if (sum!=100)
        return EBADSUM;

    /* all checks passed */
    return 0;
    }


/* 
 * Argtable calls an arg_xxx structure's hdr.errorfn routine when arg_print_errors is executed.
 * The errorfn routine must print a meaningful error messages for the given error code.
 * The error codes are the same values returned by scanfn and checkfn.
 */
void myerrorfn(struct arg_int *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
    {
    const char *shortopts = parent->hdr.shortopts;
    const char *longopts  = parent->hdr.longopts;
    const char *datatype  = parent->hdr.datatype;

    fprintf(fp,"%s: ",progname);
    switch(errorcode)
        {
        case EMINCOUNT:
            fputs("missing option ",fp);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;

        case EMAXCOUNT:
            fputs("excess option ",fp);
            arg_print_option(fp,shortopts,longopts,argval,"\n");
            break;

        case EBADINT:
            arg_print_option(fp,shortopts,longopts,argval," is not a valid <int>\n");
            break;

        case EZEROVAL:
            arg_print_option(fp,shortopts,longopts,datatype," values cannot be zero\n");
            break;

        case EODDCOUNT:
            arg_print_option(fp,shortopts,longopts,datatype," values must occur in even numbers\n");
            break;

        case EBADSUM:
            arg_print_option(fp,shortopts,longopts,datatype," values must sum to 100\n");
            break;
        }
    }


int main(int argc, char **argv)
    {
    struct arg_int  *val = arg_intn(NULL,NULL,NULL,2,100,"must be an even number of non-zero integer values that sum to 100");
    struct arg_end  *end = arg_end(20);
    void* argtable[] = {val,end};
    const char* progname = "callbacks";
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

    /* replace the default arg_int parsing and error validation routines with our own custom routines */
    val->hdr.scanfn  = (arg_scanfn*)myscanfn;
    val->hdr.checkfn = (arg_checkfn*)mycheckfn;
    val->hdr.errorfn = (arg_errorfn*)myerrorfn;

    /* special case: no command line options induces brief help */
    if (argc==1)
        {
        printf("Usage: %s ", progname);
        arg_print_syntax(stdout,argtable,"\n");
        arg_print_glossary(stdout,argtable,"where: %s %s\n");
        exitcode=0;
        goto exit;
        }

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
        {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout,end,progname);
        exitcode=1;
        goto exit;
        }

    /* parsing was succesful, print the values obtained */
    for (i=0; i<val->count; i++)
        printf("val->ival[%d] = %d\n",i, val->ival[i]);

    exit:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));

    return exitcode;
    }
