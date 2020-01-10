// policy_tree.h
//
// Copyright (C) 2008 Mike McCauley
// Author: Mike McCauley (mikem@open.com.au)
// $Id:  $
#include <libtncarray.h>
#include "osc_imv.h"

#ifndef POLICY_TREE_H
#define POLICY_TREE_H

// A node in the parse tree (subclassed for each type of node)
typedef struct pt_node
{
    int  (*evaluate)();         // Function to call to execute the node
    void (*destroy)(struct pt_node*);  // Function to call to destroy the node
} pt_node;

typedef struct
{
    pt_node	node;
    pt_node*    function;
    char*       system;
    char*       subsystem;
    char*       arg;
} pt_function;

typedef struct
{
    pt_node	node;
    pt_node*    function;
    int         op;
    char*       string;
} pt_predicate;

typedef struct
{
    pt_node	node;
    pt_node*    pred1;
    pt_node*    pred2;
} pt_conjunction;

typedef struct
{
    pt_node	node;
    pt_node*    conj1;
    pt_node*    conj2;
} pt_disjunction;

typedef struct
{
    pt_node	node;
    libtnc_array statements;
} pt_statements;

typedef struct
{
    pt_node	node;
    pt_node* disjunction;
    pt_node* statements;
} pt_if;

typedef struct
{
    pt_node	node;
    char*	string;
} pt_usermessage;

typedef struct
{
    pt_node	node;
    int         loglevel;
    char*	string;
} pt_log;

typedef struct
{
    pt_node	node;
    int         recommendation;
} pt_recommend;

// Generate a policy parse tree from the policy text in a file
extern pt_node* pt_parse_policy(char* filename);

// Destroy a parse tree or subtree
extern void     pt_destroy(pt_node* n);

// Evaluate a parse tree or subtree
extern int      pt_evaluate(pt_node* n, osc_imv_connection* conn);

// Create new nodes of various types
extern pt_node* pt_statements_new(pt_node* statement);
extern pt_node* pt_statements_append(pt_node* this, pt_node* statement);
extern pt_node* pt_if_new(pt_node* disjunction, pt_node* statements);
extern pt_node* pt_disjunction_new(pt_node* conj1, pt_node* conj2);
extern pt_node* pt_conjunction_new(pt_node* pred1, pt_node* pred2);
extern pt_node* pt_predicate_new(pt_node* function, int op, char* string);
extern pt_node* pt_function_new(char* system, char* subsystem, char* arg);
extern pt_node* pt_recommend_new(int recommendation);
extern pt_node* pt_log_new(int loglevel, char* string);
extern pt_node* pt_usermessage_new(char* s);


#endif

