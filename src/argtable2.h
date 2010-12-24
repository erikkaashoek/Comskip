/*********************************************************************
This file is part of the argtable2 library.
Copyright (C) 1998,1999,2000,2001,2003,2004 Stewart Heitmann
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
#ifndef ARGTABLE2
#define ARGTABLE2

#include <stdio.h>    /* typedef FILE */

#ifdef __cplusplus
extern "C" {
#endif


/* bit masks for arg_hdr.flag */
enum
    {
    ARG_TERMINATOR=0x1,
    ARG_HASVALUE=0x2
    };

typedef void (arg_resetfn)(void *parent);
typedef int  (arg_scanfn)(void *parent, const char *argval);
typedef int  (arg_checkfn)(void *parent);
typedef void (arg_errorfn)(void *parent, FILE *fp, int error, const char *argval, const char *progname);


/*
 * The arg_hdr struct defines properties that are common to all arg_xxx structs.
 * The argtable library requires each arg_xxx struct to have an arg_hdr
 * struct as its first data member.
 * The argtable library functions then use this data to identify the
 * properties of the command line option, such as its option tags,
 * datatype string, and glossary strings, and so on.
 * Moreover, the arg_hdr struct contains pointers to custom functions that
 * are provided by each arg_xxx struct which perform the tasks of parsing
 * that particular arg_xxx arguments, performing post-parse checks, and
 * reporting errors.
 * These functions are private to the individual arg_xxx source code
 * and are the pointer to them are initiliased by that arg_xxx struct's
 * constructor function. The user could alter them after construction
 * if desired, but the original intention is for them to be set by the
 * constructor and left unaltered.
 */
struct arg_hdr
   {
   char         flag;        /* Modifier flags: ARG_TERMINATOR, ARG_HASVALUE. */
   const char  *shortopts;   /* String defining the short options */
   const char  *longopts;    /* String defiing the long options */
   const char  *datatype;    /* Description of the argument data type */
   const char  *glossary;    /* Description of the option as shown by arg_print_glossary function */
   int          mincount;    /* Minimum number of occurences of this option accepted */
   int          maxcount;    /* Maximum number of occurences if this option accepted */
   void        *parent;      /* Pointer to parent arg_xxx struct */
   arg_resetfn *resetfn;     /* Pointer to parent arg_xxx reset function */
   arg_scanfn  *scanfn;      /* Pointer to parent arg_xxx scan function */
   arg_checkfn *checkfn;     /* Pointer to parent arg_xxx check function */
   arg_errorfn *errorfn;     /* Pointer to parent arg_xxx error function */
   };

struct arg_rem
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   };

struct arg_lit
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   };

struct arg_int
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   int *ival;               /* Storage for integer argument values */
   };

struct arg_dbl
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   double *dval;            /* Storage for double argument values */
   };

struct arg_str
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   const char **sval;       /* Storage for string argument pointers */
   };

struct arg_file
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   const char **filename;   /* Storage for filename string pointers */
   const char **basename;   /* Storage for basename string pointers */
   const char **extension;  /* Storage for extension string pointers */
   };


enum {ARG_ELIMIT=1, ARG_EMALLOC, ARG_ENOMATCH, ARG_ELONGOPT, ARG_EMISSARG};
struct arg_end
   {
   struct arg_hdr hdr;      /* The mandatory argtable header struct */
   int count;               /* Number of occurences of this parsed */
   int *error;              /* Storage for error codes */
   void **parent;           /* Storage for pointers to parent arg_xxx */
   const char **argval;     /* Storage for pointers to offending command line string */
   };


/**** arg_xxx constructor functions *********************************/

struct arg_rem* arg_rem(const char* datatype, const char* glossary);

struct arg_lit* arg_lit0(const char* shortopts,
                         const char* longopts,
                         const char* glossary);
struct arg_lit* arg_lit1(const char* shortopts,
                         const char* longopts,
                         const char *glossary);
struct arg_lit* arg_litn(const char* shortopts,
                         const char* longopts,
                         int mincount,
                         int maxcount,
                         const char *glossary);

struct arg_int* arg_int0(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char* glossary);
struct arg_int* arg_int1(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char *glossary);
struct arg_int* arg_intn(const char* shortopts,
                         const char* longopts,
                         const char *datatype,
                         int mincount,
                         int maxcount,
                         const char *glossary);

struct arg_dbl* arg_dbl0(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char* glossary);
struct arg_dbl* arg_dbl1(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char *glossary);
struct arg_dbl* arg_dbln(const char* shortopts,
                         const char* longopts,
                         const char *datatype,
                         int mincount,
                         int maxcount,
                         const char *glossary);

struct arg_str* arg_str0(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char* glossary);
struct arg_str* arg_str1(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         const char *glossary);
struct arg_str* arg_strn(const char* shortopts,
                         const char* longopts,
                         const char* datatype,
                         int mincount,
                         int maxcount,
                         const char *glossary);

struct arg_file* arg_file0(const char* shortopts,
                           const char* longopts,
                           const char* datatype,
                           const char* glossary);
struct arg_file* arg_file1(const char* shortopts,
                           const char* longopts,
                           const char* datatype,
                           const char *glossary);
struct arg_file* arg_filen(const char* shortopts,
                           const char* longopts,
                           const char* datatype,
                           int mincount,
                           int maxcount,
                           const char *glossary);

struct arg_end* arg_end(int maxerrors);


/**** other functions *******************************************/
int arg_nullcheck(void **argtable);
int arg_parse(int argc, char **argv, void **argtable);
void arg_print_option(FILE *fp, const char *shortopts, const char *longopts, const char *datatype, const char *suffix);
void arg_print_syntax(FILE *fp, void **argtable, const char *suffix);
void arg_print_syntaxv(FILE *fp, void **argtable, const char *suffix);
void arg_print_glossary(FILE *fp, void **argtable, const char *format);
void arg_print_errors(FILE* fp, struct arg_end* end, const char* progname);
void arg_freetable(void **argtable, size_t n);

/**** deprecated functions, for back-compatibility only ********/
void arg_free(void **argtable);

#ifdef __cplusplus
}
#endif
#endif









