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
enum {EMINCOUNT=1,EMAXCOUNT};

static void resetfn(struct arg_lit *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count = 0;
    }

static int scanfn(struct arg_lit *parent, const char *argval)
    {
    int errorcode = 0;
    if (parent->count < parent->hdr.maxcount )
        parent->count++;
    else
        errorcode = EMAXCOUNT;
    /*printf("%s:scanfn(%p,%s) returns %d\n",__FILE__,parent,argval,errorcode);*/
    return errorcode;
    }

static int checkfn(struct arg_lit *parent)
    {
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;
    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static void errorfn(struct arg_lit *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
    {
    const char *shortopts = parent->hdr.shortopts;
    const char *longopts  = parent->hdr.longopts;
    const char *datatype  = parent->hdr.datatype;

    switch(errorcode)
        {
        case EMINCOUNT:
            fprintf(fp,"%s: missing option ",progname);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            fprintf(fp,"\n");
            break;

        case EMAXCOUNT:
            fprintf(fp,"%s: extraneous option ",progname);
            arg_print_option(fp,shortopts,longopts,datatype,"\n");
            break;
        }
    }

struct arg_lit* arg_lit0(const char* shortopts,
                         const char* longopts,
                         const char* glossary)
    {return arg_litn(shortopts,longopts,0,1,glossary);}

struct arg_lit* arg_lit1(const char* shortopts,
                         const char* longopts,
                         const char* glossary)
    {return arg_litn(shortopts,longopts,1,1,glossary);}


struct arg_lit* arg_litn(const char* shortopts,
                         const char* longopts,
                         int mincount,
                         int maxcount,
                         const char *glossary)
    {
	struct arg_lit *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

    result = (struct arg_lit*)malloc(sizeof(struct arg_lit));
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = 0;
        result->hdr.shortopts = shortopts;
        result->hdr.longopts  = longopts;
        result->hdr.datatype  = NULL;
        result->hdr.glossary  = glossary;
        result->hdr.mincount  = mincount;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = (arg_scanfn*)scanfn;
        result->hdr.checkfn   = (arg_checkfn*)checkfn;
        result->hdr.errorfn   = (arg_errorfn*)errorfn;

        /* init local variables */
        result->count = 0;
        }
    /*printf("arg_litn() returns %p\n",result);*/
    return result;
    }
