#ifndef AUXILIARIES_H
#define AUXILIARIES_H

#define ERROR_CAPACITY 100
#define CONTEXT_CAPACITY 10
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_NODES 1000

typedef enum {
	NODE_INTEGER,
	NODE_CHAR,
	NODE_BOOL,
	NODE_IF,
	NODE_ELSE_IF,
	NODE_ELSE,
	NODE_FOR, // 6
	NODE_WHILE, // 7
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_STRUCT_DEF, // todo
	NODE_ENUM, // todo
	NODE_RETURN,
	NODE_CALL,
	NODE_SUBSCRIPT, // 14
	NODE_PARAM, // 15
	NODE_ARG, 
	NODE_ASSIGNMENT, // 17
	NODE_ADDR, // todo when we have pointer implementation

	NODE_BLOCK, // 19
	NODE_NAME, //20
	NODE_DECL,
	NODE_DEF,
	NODE_AUG,

	NODE_ADD,
	NODE_SUB, // 25
	NODE_DIV,
	NODE_MUL,
	NODE_ADD_EQUAL,
	NODE_SUB_EQUAL,
	NODE_DIV_EQUAL, // 30
	NODE_MUL_EQUAL,
	NODE_MODULO,

	NODE_LESS,
	NODE_GREATER,
	NODE_LESS_EQUAL, // 35
	NODE_GREATER_EQUAL, // 36
	NODE_NOT,
	NODE_EQUAL,
	NODE_NOT_EQUAL,
	NODE_INCREMENT, // 40
	NODE_DECREMENT,
	NODE_LOGICAL_AND,
	NODE_LOGICAL_OR,
	NODE_ARRAY_LIST,

	NODE_UNKNOWN
} node_t;

typedef union {
	int val;
	char c;
	char* name;
} NodeValue;

typedef struct Node Node;
struct Node {
	node_t type;
	NodeValue value;
	Node* left;
	Node* right;
	Node* prev;
	Node* next;
	struct type* t;
	bool node_free;
	struct Symbol* symbol;
};

typedef enum {
	TYPE_INTEGER,
	TYPE_CHAR,
	TYPE_BOOL,
	TYPE_VOID,
	TYPE_ARRAY,
	TYPE_POINTER,
	TYPE_FUNCTION,
	TYPE_STRUCT,
	TYPE_ENUM,
	TYPE_UNKNOWN
} data_t;

struct type {
	data_t kind;
	bool type_free;	
	struct type* subtype;
	Node* params;
};

// everything for typechecker 
Node* params_copy(Node* params);
Node* create_param(Node* wrapped_param);

struct type* type_create(data_t main_type, struct type* subtype, Node* params);
struct type* type_copy(struct type* t);
bool type_equals(struct type* a, struct type* b);

void typecheck_params(Node* params);
struct type* typecheck_expression(Node* expr);
void typecheck_statement(Node* stmt);
void typecheck_globals(Node* globals);
void typecheck_tree(Node* root);
void free_type(struct type* t);
// 
#endif