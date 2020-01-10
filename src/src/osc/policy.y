/* policy.y
 * 
 * Bison grammar for OSC-IMC policy files.
 * This grammar describes the permitted format of OSC IMV policy files.
 * The C file policy.tab.c and policy.tab.h are generated from this file, and
 * they actually implement the parser.
 * The result of parsing an OSC-IMC policy file is a parse tree (a tree of pt_node
 * structures) 
 * The parse tree is executed by the OSC IMV for each OSC IMC that it is asked
 * to evaluate. The  parse tree is executed in the context of a given OSC IMC-IMV connection
 * in order to evaluate the security posture of the client hosting the OSC IMC at
 * the other end of the IMC-IMV connection.
 * 
 */

%{
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id:  $

#include "policy_tree.h"

%}

%token <string> STRING NAME

%type <pt_node> policy statements statement usermessage if condition
%type <pt_node> predicate function recommend log

%type <token> loglevel op recommendation

%left AND OR
%token IF RECOMMEND USERMESSAGE LOG
%token <token> OP_EQUALS OP_CONTAINS OP_LIKE OP_GT OP_LT OP_EQ 
%token <token> LOG_ERR LOG_WARNING LOG_NOTICE LOG_INFO LOG_DEBUG
%token <token> REC_ALLOW REC_NO_ACCESS REC_ISOLATE REC_NO_RECOMMENDATION

// This lets us get the resulting policy parse tree out of yyparse()
%parse-param {pt_node** policy}

%error-verbose

%union {
    pt_node* pt_node;
    char*    string;
    int      token;
}

%%

policy:		statements
                { *policy = $1; } // Get policy back out of yyparse()
		;

statements:	statement
                {$$ = pt_statements_new($1);}	
		| statements statement
                {$$ = pt_statements_append($1, $2);}	
		;

statement:	if | recommend | log | usermessage
		;

if:		IF '(' condition ')' '{' statements '}'
                { $$ = pt_if_new($3, $6);}
		;

condition:	condition OR condition
                {$$ = pt_disjunction_new($1, $3);}
                | condition AND condition
                {$$ = pt_conjunction_new($1, $3);}
                | '(' condition ')'
		{$$ = $2;}
		| predicate
                {$$ = $1;}	
		;

predicate:	function op STRING
                {$$ = pt_predicate_new($1, $2, $3);}
		; 

function:	NAME '.' NAME '(' STRING ')'
                {$$ = pt_function_new($1, $3, $5);}
		;

op:		OP_EQUALS | OP_CONTAINS | OP_LIKE | OP_GT | OP_LT | OP_EQ
		;

recommend:	RECOMMEND recommendation
                {$$ = pt_recommend_new($2);}
		;

recommendation: REC_ALLOW | REC_NO_ACCESS | REC_ISOLATE | REC_NO_RECOMMENDATION
		;

log:		LOG loglevel STRING
                {$$ = pt_log_new($2, $3);}
		;

loglevel:	LOG_ERR | LOG_WARNING | LOG_NOTICE | LOG_INFO | LOG_DEBUG
{ $$ = $1;}
		;

usermessage:	USERMESSAGE STRING
		{$$ = pt_usermessage_new($2);}
		;

