/*********************************************************************
Example source code for defining custom arg_xxx data types for the
argtable2 command line parser library. It shows how to make custom
arg_xxx data types with additional error checking capabilities.

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
#include <stdlib.h>
#include "argxxx.h"

/* local error codes */
enum {EMINCOUNT=1,EMAXCOUNT,EBADINT,EBADRANGE};

/* 
 * resetfn is called once by arg_parse() for each of our arg_xxx
 * structs in the argument table prior to parsing the command line.
 * It should reset any internal counts in the arg_custom struct
 * as a safety precaution in case the struct has been used previously.
 * Parameters:
 *   struct arg_xxx *parent = the ptr to the arg_xxx struct being reset. 
 */
static void resetfn(struct arg_xxx *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count=0;
    }


/* scanfn is called by arg_parse() for each command line argument
 * that arg_parse attempts to match to our arg_xxx argument table entry.
 * The matching is done according to the arguments tag and/or position
 * on the command line. However the argument value is not guaranteed to
 * be the correct format.
 * The scanfn should thus attempt to parse the data, and if successful
 * store the parsed value in the arg_xxx structure's data array.
 * Otherwise it should return a non-zero error code (of your choosing).
 * That error code will eventually be passed back to the errorfn
 * routine (defined below) for printing an appropriate error message.
 * No error messages should be printed directly from within this function
 * as they will be always seen during parsing and that is undesirable if
 * multiple argument tables are employed.
 * Parameters:
 *   struct arg_xxx *parent = ptr to the arg_xxx struct in the argtable.
 *   const char *argval = ptr to the appropriate command line argv[] string   
 */ 
static int scanfn(struct arg_xxx *parent, const char *argval)
    {
    int errorcode = 0;
    /*printf("%s:scanfn(%p,\"%s\")\n",__FILE__,parent,argval);*/

    if (parent->count == parent->hdr.maxcount)
        {
        /* maximum number of arguments exceeded */
        errorcode = EMAXCOUNT;
        }
    else if (!argval)
        {
        /* an argument with no argument value was given. */
        /* This happens when an optional argument value was invoked. */
        /* leave parent argument value unaltered but still count the argument. */
        parent->count++;
        } 
    else 
        {
        double val;
        char *pend;

        /* Attempt to extract a data value from the command line argument string (argval). */
        /* In this example we wish to extract a double value, furthermore, we validate     */ 
        /* it to ensure it is bewteen our desired [minval,maxval] range.                   */ 
        val = strtod(argval,&pend);
        
        if (*pend==0 && parent->minval<=val && val<=parent->maxval)
            /* success; double value was scanned ok, and it is within our desired range.  */
            /* store the result in parent data array and increment its argument counter. */
            parent->data[parent->count++] = val;
        else if (*pend==0)
            /* failure; double value was scanned ok, but it fell outside of desired range */
            errorcode = EBADRANGE;
        else
            /* failure; command line string was not a valid double */
            errorcode = EBADINT;
        }

    /*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }


/* checkfn is called once by arg_parse() for each of our arg_xxx
 * structs in the argument table when it has completed parsing the entire command line.
 * This where we have the chance to perform any post-parsing checks.
 * At very least, we should check the minimum number of required arguments has been satisfied.  
 * Other checks may also be performed as required.
 * Parameters:
 *   struct arg_xxx *parent = ptr to the arg_xxx struct in the argtable.
 */
static int checkfn(struct arg_xxx *parent)
    {
    /* return EMINCOUNT if the minimum argment count has not been satisfied */
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;

    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }


/* errorfn is called by arg_print_errors() for each error that was returned
 * by the scanfn defined above.
 * Parameters:
 *   struct arg_xxx *parent = ptr to the arg_xxx struct in the argtable.
 *   FILE *fp = output stream
 *   int errorcode = the error code returned by the scanfn routine
 *   const char *argval = ptr to the offending command line argv[] string (may be NULL)   
 *   const char *progname = the same progname string passed to arg_print_errors()
 */
static void errorfn(struct arg_xxx *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
    {
    const char *shortopts = parent->hdr.shortopts;
    const char *longopts  = parent->hdr.longopts;
    const char *datatype  = parent->hdr.datatype;

    /* make argval NULL safe */
    argval = argval ? argval : "";

    fprintf(fp,"%s: ",progname);
    switch(errorcode)
        {
        case EMINCOUNT:
            /* We expected more arg_xxx arguments than we received. */
            fputs("missing option \"",fp);
            arg_print_option(fp,shortopts,longopts,datatype,"\"\n");
            break;

        case EMAXCOUNT:
            /* We received more arg_xxx arguments than we expected. */
            fputs("excess option \"",fp);
            arg_print_option(fp,shortopts,longopts,argval,"\"\n");
            break;

        case EBADRANGE:
            /* An arg_xxx option was given a double value that   */ 
            /* exceeded our imposed [minval,maxval] range limit. */
            fprintf(fp,"value \"%s\" out of range for option ",argval);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;

        case EBADINT:
            /* An arg_xxx option was given with an invalid double value */
            fprintf(fp,"invalid argument \"%s\" to option ",argval);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;
        }
    }


/* The arg_xxx0(), arg_xxx1(), and arg_xxxn() functions each construct a
 * and initialise an arg_xxx struct and return a pointer to it.
 * The functions must allocate a single block of memory for storing both
 * arg_xxx struct at its argument data[] array contiguously.
 */
struct arg_xxx* arg_xxx0(const char* shortopts,
                         const char* longopts,
                         const char *datatype,
                         double      minvalue,
                         double      maxvalue,
                         const char *glossary)
    {
    return arg_xxxn(shortopts,longopts,datatype,0,1,minvalue,maxvalue,glossary);
    }

struct arg_xxx* arg_xxx1(const char* shortopts,
                         const char* longopts,
                         const char *datatype,
                         double      minvalue,
                         double      maxvalue,
                         const char *glossary)
    {
    return arg_xxxn(shortopts,longopts,datatype,1,1,minvalue,maxvalue,glossary);
    }


struct arg_xxx* arg_xxxn(const char* shortopts,
                         const char* longopts,
                         const char *datatype,
                         int mincount,
                         int maxcount,
                         double      minvalue,
                         double      maxvalue,
                         const char *glossary)
    {
    size_t nbytes;
    struct arg_xxx *result;

    /* allocate a single block of memory for storing both the arg_xxx */
    /* struct and the double data[maxcount] array contiguously.       */
    nbytes = sizeof(struct arg_xxx) + maxcount*sizeof(double);

    result = (struct arg_xxx*)malloc(nbytes);
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = ARG_HASVALUE;
        result->hdr.shortopts = shortopts;
        result->hdr.longopts  = longopts;
        result->hdr.datatype  = datatype ? datatype : "<double>";
        result->hdr.glossary  = glossary;
        result->hdr.mincount  = mincount;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = (arg_scanfn*)scanfn;
        result->hdr.checkfn   = (arg_checkfn*)checkfn;
        result->hdr.errorfn   = (arg_errorfn*)errorfn;

        /* locate the data[maxcount] array immediately after the arg_xxx struct */
        result->data  = (double*)(result+1);

        /* init the remaining of the arg_xxx struct variables */
        result->count = 0;
        result->minval = minvalue;
        result->maxval = maxvalue;
        }

    /*printf("arg_intn() returns %p\n",result);*/
    return result;
    }
