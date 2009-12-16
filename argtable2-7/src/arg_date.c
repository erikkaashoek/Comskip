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
/* glibc2 needs this for strptime */
#define _XOPEN_SOURCE 

/* SunOS also requires this for strptime */
#define _XOPEN_VERSION 4 

/* config.h must be included before anything else */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "argtable2.h"

/* local error codes  */
enum {EMINCOUNT=1,EMAXCOUNT,EBADDATE};

static void resetfn(struct arg_date *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count=0;
    }

static int scanfn(struct arg_date *parent, const char *argval)
    {
    int errorcode = 0;

    if (parent->count == parent->hdr.maxcount )
        errorcode = EMAXCOUNT;
    else if (!argval)
        {
        /* no argument value was given, leave parent->tmval[] unaltered but still count it */
        parent->count++;
        }
    else 
        {
        const char *pend;
        struct tm tm = parent->tmval[parent->count];

        /* parse the given argument value, store result in parent->tmval[] */
        pend = strptime(argval, parent->format, &tm);
        if (pend && pend[0]=='\0')
            parent->tmval[parent->count++] = tm;
        else
            errorcode = EBADDATE;
        }

    /*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static int checkfn(struct arg_date *parent)
    {
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;
     
    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static void errorfn(struct arg_date *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
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

        case EBADDATE:
            {
            struct tm tm;
            char buff[200];           

            fprintf(fp,"illegal timestamp format \"%s\"\n",argval);
            bzero(&tm,sizeof(tm));
            strptime("1999-12-31 23:59:59","%F %H:%M:%S",&tm);
            strftime(buff, sizeof(buff), parent->format, &tm);
            printf("correct format is \"%s\"\n", buff);                        
            break;
            }
        }
    }
 

struct arg_date* arg_date0(const char* shortopts,
                           const char* longopts,
                           const char* format,
                           const char *datatype,
                           const char *glossary)
    {
    return arg_daten(shortopts,longopts,format,datatype,0,1,glossary);
    }

struct arg_date* arg_date1(const char* shortopts,
                           const char* longopts,
                           const char* format,
                           const char *datatype,
                           const char *glossary)
    {
    return arg_daten(shortopts,longopts,format,datatype,1,1,glossary);
    }


struct arg_date* arg_daten(const char* shortopts,
                           const char* longopts,
                           const char* format,
                           const char *datatype,
                           int mincount,
                           int maxcount,
                           const char *glossary)
    {
    size_t nbytes;
    struct arg_date *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

    /* default time format is the national date format for the locale */
    if (!format)
        format = "%x";

    nbytes = sizeof(struct arg_date)         /* storage for struct arg_date */
           + maxcount*sizeof(struct tm);     /* storage for tmval[maxcount] array */

    /* allocate storage for the arg_date struct + tmval[] array.    */
    /* we use calloc because we want the tmval[] array zero filled. */
    result = (struct arg_date*)calloc(1,nbytes);
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = ARG_HASVALUE;
        result->hdr.shortopts = shortopts;
        result->hdr.longopts  = longopts;
        result->hdr.datatype  = datatype ? datatype : format;
        result->hdr.glossary  = glossary;
        result->hdr.mincount  = mincount;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = (arg_scanfn*)scanfn;
        result->hdr.checkfn   = (arg_checkfn*)checkfn;
        result->hdr.errorfn   = (arg_errorfn*)errorfn;

        /* store the tmval[maxcount] array immediately after the arg_date struct */
        result->tmval  = (struct tm*)(result+1);

        /* init the remaining arg_date member variables */
        result->count = 0;
        result->format = format;
        }

    /*printf("arg_daten() returns %p\n",result);*/
    return result;
    }
