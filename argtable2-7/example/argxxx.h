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

#ifndef ARG_XXX
#define ARG_XXX

#include <argtable2.h>

struct arg_xxx
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of matching command line arguments found */
   double *data;            /* Array of matching command line argument data  */
   double minval, maxval;   /* Custom range of allowable data values */
   };

struct arg_xxx* arg_xxx0(const char* shortopts, const char* longopts, const char *datatype,
                         double minvalue, double maxvalue, const char *glossary);

struct arg_xxx* arg_xxx1(const char* shortopts, const char* longopts, const char *datatype,
                         double minvalue, double maxvalue, const char *glossary);

struct arg_xxx* arg_xxxn(const char* shortopts, const char* longopts, const char *datatype,
                         int mincount, int maxcount,
                         double minvalue, double maxvalue, const char *glossary);

#endif
