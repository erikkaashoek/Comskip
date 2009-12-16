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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "argtable2.h"

#ifdef WIN32
# define FILESEPARATOR '\\'
#else
# define FILESEPARATOR '/'
#endif

/* local error codes */
enum {EMINCOUNT=1,EMAXCOUNT};


static void resetfn(struct arg_file *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count=0;
    }


/* Returns ptr to the base filename within *filename */
static const char* arg_basename(const char *filename)
    {
    const char *result = (filename ? strrchr(filename,FILESEPARATOR) : NULL);
    if (result)
        result++;
    else
        result = filename;
    return result;
    }


/* Returns ptr to the file extension within *filename */
static const char* arg_extension(const char *filename)
    {
    const char *result = (filename ? strrchr(filename,'.') : NULL);
    if (filename && !result)
        result = filename+strlen(filename);
    return result;
    }


static int scanfn(struct arg_file *parent, const char *argval)
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
        parent->filename[parent->count]  = argval;
        parent->basename[parent->count]  = arg_basename(argval);
        parent->extension[parent->count] = arg_extension(argval);
        parent->count++;
        }

    /*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }


static int checkfn(struct arg_file *parent)
    {
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;
    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }


static void errorfn(struct arg_file *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
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

        default:
            fprintf(fp,"unknown error at \"%s\"\n",argval);
        }
    }


struct arg_file* arg_file0(const char* shortopts,
                           const char* longopts,
                           const char *datatype,
                           const char *glossary)
    {
    return arg_filen(shortopts,longopts,datatype,0,1,glossary);
    }


struct arg_file* arg_file1(const char* shortopts,
                           const char* longopts,
                           const char *datatype,
                           const char *glossary)
    {
    return arg_filen(shortopts,longopts,datatype,1,1,glossary);
    }


struct arg_file* arg_filen(const char* shortopts,
                           const char* longopts,
                           const char *datatype,
                           int mincount,
                           int maxcount,
                           const char *glossary)
    {
    size_t nbytes;
    struct arg_file *result;

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

    nbytes = sizeof(struct arg_file)     /* storage for struct arg_file */
           + sizeof(char*) * maxcount    /* storage for filename[maxcount] array */
           + sizeof(char*) * maxcount    /* storage for basename[maxcount] array */
           + sizeof(char*) * maxcount;   /* storage for extension[maxcount] array */

    result = (struct arg_file*)malloc(nbytes);
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = ARG_HASVALUE;
        result->hdr.shortopts = shortopts;
        result->hdr.longopts  = longopts;
        result->hdr.glossary  = glossary;
        result->hdr.datatype  = datatype ? datatype : "<file>";
        result->hdr.mincount  = mincount;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = (arg_scanfn*)scanfn;
        result->hdr.checkfn   = (arg_checkfn*)checkfn;
        result->hdr.errorfn   = (arg_errorfn*)errorfn;

        /* store the filename,basename,extension arrays immediately after the arg_file struct */
        result->filename  = (const char**)(result+1);
        result->basename  = result->filename + maxcount;
        result->extension = result->basename + maxcount;
        result->count = 0;
        }
    /*printf("arg_filen() returns %p\n",result);*/
    return result;
    }
