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

/* #ifdef HAVE_STDLIB_H */
#include <stdlib.h>
/* #endif */

#include "argtable2.h"

static void resetfn(struct arg_end *parent)
    {
    /*printf("%s:resetfn(%p)\n",__FILE__,parent);*/
    parent->count = 0;
    }

static void errorfn(void *parent, FILE *fp, int error, const char *argval, const char *progname)
    {
    progname = progname ? progname : "";
    argval = argval ? argval : "";

    fprintf(fp,"%s: ",progname);
    switch(error)
        {
        case ARG_ELIMIT:
            fputs("too many errors to display",fp);
            break;
        case ARG_EMALLOC:
            fputs("insufficent memory",fp);
            break;
        case ARG_ENOMATCH:
            fprintf(fp,"unexpected argument \"%s\"",argval);
            break;
        case ARG_EMISSARG:
            fprintf(fp,"option \"%s\" requires an argument",argval);
            break;
        case ARG_ELONGOPT:
            fprintf(fp,"invalid option \"%s\"",argval);
            break;
        default:
            fprintf(fp,"invalid option \"-%c\"",error);
            break;
        }
    fputc('\n',fp);
    }


struct arg_end* arg_end(int maxcount)
    {
    size_t nbytes;
    struct arg_end *result;

    nbytes = sizeof(struct arg_end)
           + maxcount * sizeof(int)             /* storage for int error[maxcount] array*/
           + maxcount * sizeof(void*)           /* storage for void* parent[maxcount] array */
           + maxcount * sizeof(char*);          /* storage for char* argval[maxcount] array */

    result = (struct arg_end*)malloc(nbytes);
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = ARG_TERMINATOR;
        result->hdr.shortopts = NULL;
        result->hdr.longopts  = NULL;
        result->hdr.datatype  = NULL;
        result->hdr.glossary  = NULL;
        result->hdr.mincount  = 1;
        result->hdr.maxcount  = maxcount;
        result->hdr.parent    = result;
        result->hdr.resetfn   = (arg_resetfn*)resetfn;
        result->hdr.scanfn    = NULL;
        result->hdr.checkfn   = NULL;
        result->hdr.errorfn   = errorfn;

        /* store error[maxcount] array immediately after struct arg_end */
        result->error = (int*)(result+1);

        /* store parent[maxcount] array immediately after error[] array */
        result->parent = (void**)(result->error + maxcount );

        /* store argval[maxcount] array immediately after parent[] array */
        result->argval = (const char**)(result->parent + maxcount );
        }

    /*printf("arg_end(%d) returns %p\n",maxcount,result);*/
    return result;
    }


void arg_print_errors(FILE* fp, struct arg_end* end, const char* progname)
    {
    int i;
    /*printf("arg_errors()\n");*/
    for (i=0; i<end->count; i++)
        {
        struct arg_hdr *errorparent = (struct arg_hdr *)(end->parent[i]);
        if (errorparent->errorfn)
            errorparent->errorfn(end->parent[i],fp,end->error[i],end->argval[i],progname);
        }
    }


