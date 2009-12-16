/*********************************************************************
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

/* config.h must be included before anything else */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "argtable2.h"

/* local error codes */
enum {EMINCOUNT=1,EMAXCOUNT,EBADDOUBLE};

static void resetfn(struct arg_dbl *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count=0;
    }

static int scanfn(struct arg_dbl *parent, const char *argval)
    {
    int errorcode = 0;

    if (parent->count == parent->hdr.maxcount)
        {
        /* maximum number of arguments exceeded */
        errorcode = EMAXCOUNT;
        }
    else if (!argval)
        {
        /* a valid argument with no argument value was given. */
        /* This happens when an optional argument value was invoked. */
        /* leave parent arguiment value unaltered but still count the argument. */
        parent->count++;
        }
    else
        {
        double val;
        char *end;

        /* extract double from argval into val */
        val = strtod(argval,&end);

        /* if success then store result in parent->dval[] array otherwise return error*/
        if (*end==0)
            parent->dval[parent->count++] = val;
        else
            errorcode = EBADDOUBLE;
        }

    /*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static int checkfn(struct arg_dbl *parent)
    {
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;
    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static void errorfn(struct arg_dbl *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
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
            fputs("missing option ",fp);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;

        case EMAXCOUNT:
            fputs("excess option ",fp);
            arg_print_option(fp,shortopts,longopts,argval,"\n");
            break;

        case EBADDOUBLE:
            fprintf(fp,"invalid argument \"%s\" to option ",argval);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;
        }
    }


struct arg_dbl* arg_dbl0(const char* shortopts,
                               const char* longopts,
                               const char *datatype,
                               const char *glossary)
    {
    return arg_dbln(shortopts,longopts,datatype,0,1,glossary);
    }

struct arg_dbl* arg_dbl1(const char* shortopts,
                               const char* longopts,
                               const char *datatype,
                               const char *glossary)
    {
    return arg_dbln(shortopts,longopts,datatype,1,1,glossary);
    }


struct arg_dbl* arg_dbln(const char* shortopts,
                               const char* longopts,
                               const char *datatype,
                               int mincount,
                               int maxcount,
                               const char *glossary)
    {
    size_t nbytes;
    struct arg_dbl *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

    nbytes = sizeof(struct arg_dbl)     /* storage for struct arg_dbl */
           + (maxcount+1) * sizeof(double);  /* storage for dval[maxcount] array plus one extra for padding to memory boundary */

    result = (struct arg_dbl*)malloc(nbytes);
    if (result)
        {
        size_t addr;
        size_t rem;

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

        /* Store the dval[maxcount] array on the first double boundary that immediately follows the arg_dbl struct. */
        /* We do the memory alignment purely for SPARC and Motorola systems. They require floats and doubles to be  */
        /* aligned on natural boundaries */
        addr = (size_t)(result+1);
        rem  = addr % sizeof(double);
        result->dval  = (double*)(addr + sizeof(double) - rem);
        /* printf("addr=%p, dval=%p, sizeof(double)=%d rem=%d\n", addr, result->dval, (int)sizeof(double), (int)rem); */

        result->count = 0;
        }
    /*printf("arg_dbln() returns %p\n",result);*/
    return result;
    }
