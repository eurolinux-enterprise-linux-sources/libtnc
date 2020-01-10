// policy_tree.c
//
// Called by the policy YACC parser to generate a parse tree 
// representing the policy file
// Nodes in the tree are subclasses of pt_node.
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id:  $

#ifdef WIN32
#define TNC_IMC_EXPORTS
#include <windows.h>
#define snprintf _snprintf
#else
#include <config.h>
#endif

#include "policy_tree.h"
#include "policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// For access to lexer variables
extern FILE *yyin, *yyout;
extern int  yydebug, lexer_lineno;
extern char *lexer_filename, *yytext;

////////////////////////////////////////////////////////////////////////////////
// Lexer and parser errors will end up here
void
yyerror(pt_node* n)
{
    fprintf(stderr, "OSC_IMV syntax error at '%s' line %d: '%s'\n", lexer_filename, lexer_lineno, yytext);
}

////////////////////////////////////////////////////////////////////////////////
// pt_parse_policy
// Parse the text of a policy
// Return a pointer to the head of the parse tree, if successful
pt_node*
pt_parse_policy(char* filename)
{
    pt_node* policy = NULL;
    FILE* file;

//    yydebug = 1;
    if (file = fopen(filename, "r"))
    {
	yyin = file;
	lexer_lineno = 1;
	lexer_filename = filename;
	yyparse(&policy);
	yyin = NULL;
	lexer_filename = NULL;
	fclose(file);
    }
    else
    {
	fprintf(stderr, "OSC_IMV could not open policy file '%s': %s\n", filename, sys_errlist[errno]);
    }
    return policy;
}

////////////////////////////////////////////////////////////////////////////////
// pt_destroy
// Destroy an entire parse tree, starting at the root node.
void
pt_destroy(pt_node* n)
{
    n->destroy(n);
}

////////////////////////////////////////////////////////////////////////////////
// pt_evaluate
// Evaluate a parse tree, returning the value of evaluate of the root node
int
pt_evaluate(pt_node* n, osc_imv_connection* conn)
{
    return n->evaluate(n, conn);
}

////////////////////////////////////////////////////////////////////////////////
// statement
int
pt_statements_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_statements* p = (pt_statements*)this;
    int size = libtnc_array_size(&p->statements);
    int i;
    
    for (i = 0; i < size; i++)
    {
	pt_node* s = libtnc_array_index(&p->statements, i);
	pt_evaluate(s, conn);
    }
    return 1;
}

void 
pt_statements_destroy(pt_node* this)
{
    pt_statements* p = (pt_statements*)this;
    int size = libtnc_array_size(&p->statements);
    int i;

    for (i = 0; i < size; i++)
	pt_destroy(libtnc_array_index(&p->statements, i));
    libtnc_array_free(&p->statements);
    free(this);
}

pt_node*
pt_statements_new(pt_node* statement)
{
    pt_statements* p = calloc(1, sizeof(pt_statements));

    if (p)
    {
	p->node.evaluate = pt_statements_evaluate;
	p->node.destroy = pt_statements_destroy;
	libtnc_array_init(&p->statements);
	libtnc_array_push(&p->statements, (void*)statement);
    }
    return (pt_node*)p;
}

pt_node*
pt_statements_append(pt_node* this, pt_node* statement)
{
    pt_statements* p = (pt_statements*)this;

    libtnc_array_push(&p->statements, (void*)statement);
    return (pt_node*)this;
}

////////////////////////////////////////////////////////////////////////////////
// if
int
pt_if_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_if* p = (pt_if*)this;

    if (pt_evaluate(p->disjunction, conn))
    {
	pt_evaluate(p->statements, conn);
    }
    return 1;
}

void 
pt_if_destroy(pt_node* this)
{
    pt_if* p = (pt_if*)this;

    pt_destroy(p->disjunction);
    pt_destroy(p->statements);
    free(this);
}

pt_node*
pt_if_new(pt_node* disjunction, pt_node* statements)
{
    pt_if* p = calloc(1, sizeof(pt_if));

    if (p)
    {
	p->node.evaluate = pt_if_evaluate;
	p->node.destroy = pt_if_destroy;
	p->disjunction = disjunction;
	p->statements = statements;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// disjunction
int
pt_disjunction_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_disjunction* p = (pt_disjunction*)this;
    int result = pt_evaluate(p->conj1, conn);

    if (p->conj2)
    {
	// Some gymnastics to prevent the compiler doing shortcircuits
	int thisresult = pt_evaluate(p->conj2, conn);
	result = result || thisresult;
    }
    return result;
}

void 
pt_disjunction_destroy(pt_node* this)
{
    pt_disjunction* p = (pt_disjunction*)this;

    pt_destroy(p->conj1);
    if (p->conj2)
	pt_destroy(p->conj2);
    free(this);
}

pt_node*
pt_disjunction_new(pt_node* conj1, pt_node* conj2)
{
    pt_disjunction* p = calloc(1, sizeof(pt_disjunction));

    if (p)
    {
	p->node.evaluate = pt_disjunction_evaluate;
	p->node.destroy = pt_disjunction_destroy;
	p->conj1 = conj1;
	p->conj2 = conj2;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// conjunction
int
pt_conjunction_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_conjunction* p = (pt_conjunction*)this;
    int result = pt_evaluate(p->pred1, conn);

    if (p->pred2)
    {
	// Some gymnastics to prevent the compiler doing shortcircuits
	int thisresult = pt_evaluate(p->pred2, conn);
	result = result && thisresult;
    }

    return result;
}

void 
pt_conjunction_destroy(pt_node* this)
{
    pt_conjunction* p = (pt_conjunction*)this;

    pt_destroy(p->pred1);
    if (p->pred2)
	pt_destroy(p->pred2);
    free(this);
}

pt_node*
pt_conjunction_new(pt_node* pred1, pt_node* pred2)
{
    pt_conjunction* p = calloc(1, sizeof(pt_conjunction));

    if (p)
    {
	p->node.evaluate = pt_conjunction_evaluate;
	p->node.destroy = pt_conjunction_destroy;
	p->pred1 = pred1;
	p->pred2 = pred2;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// function
int
pt_function_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_function* p = (pt_function*)this;
    char*        value;

    // See if there is a value available for the requested function
    // If not, arrange for it to be got from the client IMC and return 0
    if (value = connGetClientData(conn, p->system, p->arg, p->subsystem))
    {
	// Available
	return 1;
    }
    else
    {
	// Not available, arrange to get it
	connRequestClientData(conn, p->system, p->arg);
	return 0;
    }
}

// Get the value of a specific client data item.
// It is assumed that it is available
char*
pt_function_get_value(pt_node* this, osc_imv_connection* conn)
{
    pt_function* p = (pt_function*)this;
    char*        value;
    
    return connGetClientData(conn, p->system, p->arg, p->subsystem);
}

void 
pt_function_destroy(pt_node* this)
{
    pt_function* p = (pt_function*)this;
    
    free(p->system);
    free(p->subsystem);
    free(p->arg);
    free(this);
}

pt_node*
pt_function_new(char* system, char* subsystem, char* arg)
{
    pt_function* p = calloc(1, sizeof(pt_function));

    if (p)
    {
	p->node.evaluate = pt_function_evaluate;
	p->node.destroy = pt_function_destroy;
	p->system = system;
	p->subsystem = subsystem;
	p->arg = arg;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// predicate
int
pt_predicate_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_predicate* p = (pt_predicate*)this;
    int fnresult = pt_evaluate((pt_node*)p->function, conn);
    int result = 0;

    if (fnresult)
    {
	// Function has the value available
	// Get the value of the function which we can compare
	// and compare it against string according to the operand
	char* fnstring = pt_function_get_value((pt_node*)p->function, conn);
	switch (p->op)
	{
	    case OP_EQUALS:
		result = atoi(fnstring) == atoi(p->string);
		break;

	    case OP_CONTAINS:
		result = (strstr(fnstring, p->string) != NULL);
		break;

	    case OP_LIKE:
		break;

	    case OP_GT:
		result = atoi(fnstring) > atoi(p->string);
		break;

	    case OP_LT:
		result = atoi(fnstring) < atoi(p->string);
		break;

	    case OP_EQ:
		result = (strcmp(fnstring, p->string) == 0);
		break;

	    default:
		// Should never happen
		break;
	}
    }
    return result;
}

void 
pt_predicate_destroy(pt_node* this)
{
    pt_predicate* p = (pt_predicate*)this;

    pt_destroy(p->function);
    free(p->string);
    free(this);
}

pt_node*
pt_predicate_new(pt_node* function, int op, char* string)
{
    pt_predicate* p = calloc(1, sizeof(pt_predicate));

    if (p)
    {
	p->node.evaluate = pt_predicate_evaluate;
	p->node.destroy = pt_predicate_destroy;
	p->function = function;
	p->op = op;
	p->string = string;
	    
    }
    return (pt_node*)p;
}


////////////////////////////////////////////////////////////////////////////////
// usermessage
int
pt_usermessage_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_usermessage* p = (pt_usermessage*)this;

    connUserMessage(conn, p->string);
    return 1;
}

void 
pt_usermessage_destroy(pt_node* this)
{
    pt_usermessage* p = (pt_usermessage*)this;

    free(p->string);
    free(this);
}

pt_node*
pt_usermessage_new(char* s)
{
    pt_usermessage* p = calloc(1, sizeof(pt_usermessage));

    if (p)
    {
	p->node.evaluate = pt_usermessage_evaluate;
	p->node.destroy = pt_usermessage_destroy;
	p->string = s;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// log
int
pt_log_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_log* p = (pt_log*)this;

    connLogMessage(conn, p->loglevel, p->string);
    return 1;
}

void 
pt_log_destroy(pt_node* this)
{
    pt_log* p = (pt_log*)this;
    free(p->string);
    free(this);
}

pt_node*
pt_log_new(int loglevel, char* string)
{
    pt_log* p = calloc(1, sizeof(pt_log));

    if (p)
    {
	p->node.evaluate = pt_log_evaluate;
	p->node.destroy = pt_log_destroy;
	p->loglevel = loglevel;
	p->string = string;
    }
    return (pt_node*)p;
}

////////////////////////////////////////////////////////////////////////////////
// recommend
int
pt_recommend_evaluate(pt_node* this, osc_imv_connection* conn)
{
    pt_recommend* p = (pt_recommend*)this;
    TNC_IMV_Action_Recommendation recommendation 
	= TNC_IMV_ACTION_RECOMMENDATION_NO_RECOMMENDATION;

    // Convert tokens into TNC Recommendation codes
    switch (p->recommendation)
    {
	case REC_ALLOW:
	    recommendation = TNC_IMV_ACTION_RECOMMENDATION_ALLOW;
	    break;

	case REC_NO_ACCESS:
	    recommendation = TNC_IMV_ACTION_RECOMMENDATION_NO_ACCESS;
	    break;

	case REC_ISOLATE:
	    recommendation = TNC_IMV_ACTION_RECOMMENDATION_ISOLATE;
	    break;

    }
    connSetRecommendation(conn, recommendation);
    return 1;
}

void 
pt_recommend_destroy(pt_node* this)
{
    pt_recommend* p = (pt_recommend*)this;

    free(this);
}

pt_node*
pt_recommend_new(int recommendation)
{
    pt_recommend* p = calloc(1, sizeof(pt_recommend));

    if (p)
    {
	p->node.evaluate = pt_recommend_evaluate;
	p->node.destroy = pt_recommend_destroy;
	p->recommendation = recommendation;
    }
    return (pt_node*)p;
}

