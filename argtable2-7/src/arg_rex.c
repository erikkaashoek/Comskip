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
#include <sys/types.h>
#include <regex.h>


/* local error codes (these must not conflict with reg_error codes) */
enum {EMINCOUNT=200,EMAXCOUNT=201};

struct privhdr
    {
    const char *pattern;
    int flags;
    regex_t regex;
    };


static void resetfn(struct arg_rex *parent)
    {
    struct privhdr *priv = (struct privhdr*)(parent->hdr.priv);

    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count=0;

    /* construct the regex representation of the given pattern string. */
    /* Dont bother checking for errors as we already did that earlier (in the constructor) */
    regcomp(&(priv->regex), priv->pattern, priv->flags);
    }

static int scanfn(struct arg_rex *parent, const char *argval)
    {
    int errorcode = 0;

    if (parent->count == parent->hdr.maxcount )
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
        struct privhdr *priv = (struct privhdr*)parent->hdr.priv;
    
       /* test the current argument value for a match with the regular expression */
        /* if a match is detected, record the argument value in the arg_rex struct */
        errorcode = regexec(&(priv->regex), argval, 0, NULL, 0);
        if (errorcode==0)
            parent->sval[parent->count++] = argval;
        }

    /*printf("%s:scanfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static int checkfn(struct arg_rex *parent)
    {
    int errorcode = (parent->count < parent->hdr.mincount) ? EMINCOUNT : 0;
    struct privhdr *priv = (struct privhdr*)parent->hdr.priv;
     
    /* free the regex "program" we constructed in resetfn */
    regfree(&(priv->regex));

    /*printf("%s:checkfn(%p) returns %d\n",__FILE__,parent,errorcode);*/
    return errorcode;
    }

static void errorfn(struct arg_rex *parent, FILE *fp, int errorcode, const char *argval, const char *progname)
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

        case REG_NOMATCH:
            fputs("illegal value  ",fp);
            arg_print_option(fp,shortopts,longopts,argval,"\n");
            break;
        
        default:
            {
            char errbuff[256];
            regerror(errorcode, NULL, errbuff, sizeof(errbuff));
            printf("%s\n", errbuff);
            }
            break;
        }
    }


struct arg_rex* arg_rex0(const char* shortopts,
                         const char* longopts,
                         const char* pattern,
                         const char *datatype,
                         int flags,
                         const char *glossary)
    {
    return arg_rexn(shortopts,longopts,pattern,datatype,0,1,flags,glossary);
    }

struct arg_rex* arg_rex1(const char* shortopts,
                         const char* longopts,
                         const char* pattern,
                         const char *datatype,
                         int flags,
                         const char *glossary)
    {
    return arg_rexn(shortopts,longopts,pattern,datatype,1,1,flags,glossary);
    }


struct arg_rex* arg_rexn(const char* shortopts,
                         const char* longopts,
                         const char* pattern,
                         const char *datatype,
                         int mincount,
                         int maxcount,
                         int flags,
                         const char *glossary)
    {
    size_t nbytes;
    struct arg_rex *result;
    struct privhdr *priv;

    if (!pattern)
        {
        printf("argtable: ERROR - illegal regular expression pattern \"(NULL)\"\n");
        printf("argtable: Bad argument table.\n");
        return NULL;
        }

	/* foolproof things by ensuring maxcount is not less than mincount */
	maxcount = (maxcount<mincount) ? mincount : maxcount;

    nbytes = sizeof(struct arg_rex)       /* storage for struct arg_rex */
           + sizeof(struct privhdr)       /* storage for private arg_rex data */
           + maxcount * sizeof(char*);    /* storage for sval[maxcount] array */

    result = (struct arg_rex*)malloc(nbytes);
    if (result)
        {
        int errorcode;

        /* init the arg_hdr struct */
        result->hdr.flag      = ARG_HASVALUE;
        result->hdr.shortopts = shortopts;
        result->hdr.longopts  = longopts;
        result->hdr.datatype  = datatype ? datatype : pattern;
        result->hdr.glossary  = glossary;
        result->hdr.mincount  = mincount;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = (arg_scanfn*)scanfn;
        result->hdr.checkfn   = (arg_checkfn*)checkfn;
        result->hdr.errorfn   = (arg_errorfn*)errorfn;

        /* store the arg_rex_priv struct immediately after the arg_rex struct */
        result->hdr.priv  = (const char**)(result+1);
        priv = (struct privhdr*)(result->hdr.priv);
        priv->pattern = pattern;
        priv->flags = flags | REG_NOSUB;

        /* store the sval[maxcount] array immediately after the arg_rex_priv struct */
        result->sval  = (const char**)(priv+1);
        result->count = 0;

        /* here we construct and destroy a regex representation of the regular expression
           for no other reason than to force any regex errors to be trapped now rather
           than later. If we dont, then errors may go undetected until an argument is
           actually parsed. */
        errorcode = regcomp(&(priv->regex), priv->pattern, priv->flags);
        if (errorcode)
            {
            char errbuff[256];
            regerror(errorcode, &(priv->regex), errbuff, sizeof(errbuff));
            printf("argtable: %s \"%s\"\n", errbuff, priv->pattern);
            printf("argtable: Bad argument table.\n");
            }
        else
            regfree(&(priv->regex)); 
        }

    /*printf("arg_rexn() returns %p\n",result);*/
    return result;
    }
