/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     STRING = 258,
     NAME = 259,
     OR = 260,
     AND = 261,
     IF = 262,
     RECOMMEND = 263,
     USERMESSAGE = 264,
     LOG = 265,
     OP_EQUALS = 266,
     OP_CONTAINS = 267,
     OP_LIKE = 268,
     OP_GT = 269,
     OP_LT = 270,
     OP_EQ = 271,
     LOG_ERR = 272,
     LOG_WARNING = 273,
     LOG_NOTICE = 274,
     LOG_INFO = 275,
     LOG_DEBUG = 276,
     REC_ALLOW = 277,
     REC_NO_ACCESS = 278,
     REC_ISOLATE = 279,
     REC_NO_RECOMMENDATION = 280
   };
#endif
#define STRING 258
#define NAME 259
#define OR 260
#define AND 261
#define IF 262
#define RECOMMEND 263
#define USERMESSAGE 264
#define LOG 265
#define OP_EQUALS 266
#define OP_CONTAINS 267
#define OP_LIKE 268
#define OP_GT 269
#define OP_LT 270
#define OP_EQ 271
#define LOG_ERR 272
#define LOG_WARNING 273
#define LOG_NOTICE 274
#define LOG_INFO 275
#define LOG_DEBUG 276
#define REC_ALLOW 277
#define REC_NO_ACCESS 278
#define REC_ISOLATE 279
#define REC_NO_RECOMMENDATION 280




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 44 "policy.y"
typedef union YYSTYPE {
    pt_node* pt_node;
    char*    string;
    int      token;
} YYSTYPE;
/* Line 1249 of yacc.c.  */
#line 92 "policy.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



