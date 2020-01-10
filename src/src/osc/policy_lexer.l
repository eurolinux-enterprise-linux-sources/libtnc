/* policy.l
 * 
 * Flex file for OSC-IMC policy files.
 * Specifies the lexical analyser for OSC IMV policy files.
 * Generates lex.yy.c
 * 
 * Any strings allocated here must be freed later by the parser or the application
 * 
 * 
 * 
 */

%{
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id:  $

#include "policy_tree.h"
#include "policy.h"
#include <string.h>

// These are set by the parser caller so we can print some useful logging errors
int   lexer_lineno = 1;
char* lexer_filename = NULL;

// Shortcut for defining simple tokens
// Sets the yylval to the token and also returns the token
#define TOK(n) yylval.token = n; return n;

%}

comment			#.*
string			\'[^\'\n]*\'
word			[a-zA-Z]+
ws			[ \t\r]
%%

{ws}			;
{comment}		;
{string}		{ yylval.string = strdup(yytext+1);
			  yylval.string[yyleng-2] = '\0';
			  return STRING; }

if			{ TOK(IF) }
or			{ TOK(OR) }
and			{ TOK(AND) }
recommend		{ TOK(RECOMMEND) }
log			{ TOK(LOG) }
usermessage		{ TOK(USERMESSAGE) }

"=="			{ TOK(OP_EQUALS) }
contains		{ TOK(OP_CONTAINS) }
like			{ TOK(OP_LIKE) }
">"			{ TOK(OP_GT) }
"<"			{ TOK(OP_LT) }
eq			{ TOK(OP_EQ) }

ERR			{ TOK(LOG_ERR) }
WARNING			{ TOK(LOG_WARNING) }
NOTICE			{ TOK(LOG_NOTICE) }
INFO			{ TOK(LOG_INFO) }
DEBUG			{ TOK(LOG_DEBUG) }

ALLOW			{ TOK(REC_ALLOW) }
NO_ACCESS		{ TOK(REC_NO_ACCESS) }
ISOLATE			{ TOK(REC_ISOLATE) }
NO_RECOMMENDATION	{ TOK(REC_NO_RECOMMENDATION) }

{word}			{ yylval.string = strdup(yytext);
			  return NAME; }	
\n			{ lexer_lineno++; }
.			{ return yytext[0];}

%%

// yyerror is in policy_tree.c
int
yywrap()
{
    return 1;
}
