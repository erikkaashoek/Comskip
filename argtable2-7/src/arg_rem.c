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

/*#ifdef HAVE_STDLIB_H*/
#include <stdlib.h>
/*#endif*/

#include "argtable2.h"

struct arg_rem* arg_rem(const char *datatype,
                        const char *glossary)
    {
    struct arg_rem *result = (struct arg_rem*)malloc(sizeof(struct arg_rem));
    if (result)
        {
        /* init the arg_hdr struct */
        result->hdr.flag      = 0;
        result->hdr.shortopts = NULL;
        result->hdr.longopts  = NULL;
        result->hdr.datatype  = datatype;
        result->hdr.glossary  = glossary;
        result->hdr.mincount  = 1;
        result->hdr.maxcount  = 1;
        result->hdr.parent    = result;
        result->hdr.resetfn   = NULL;
        result->hdr.scanfn    = NULL;
        result->hdr.checkfn   = NULL;
        result->hdr.errorfn   = NULL;
        }
    /*printf("arg_rem() returns %p\n",result);*/
    return result;
    }
