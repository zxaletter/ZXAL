#ifndef PARSER_H
#define PARSER_H

#include "Lexer/lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_NODES 1000

typedef enum {
	LOGIC_ERROR,
	SYNTAX_ERROR,
	SEMANTICS_ERROR,
	TYPE_ERROR,
} error_t;

typedef struct {
	error_t type;
	char* line;
	char* msg;
} Error;

typedef enum {
	NODE_INTEGER,
	NODE_IF,
	NODE_ELSE_IF,
	NODE_ELSE,
	NODE_FOR,
	NODE_WHILE,
	NODE_RETURN,
	NODE_CALL,
	NODE_SUBSCRIPT,
	NODE_ARG, 
	NODE_ASSIGNMENT,
	NODE_ADDR,
	NODE_NAME,

	NODE_ADD,
	NODE_SUB,
	NODE_DIV,
	NODE_MUL,
	NODE_ADD_EQUAL,
	NODE_SUB_EQUAL,
	NODE_DIV_EQUAL,
	NODE_MUL_EQUAL,

	NODE_LESS,
	NODE_GREATER,
	NODE_LESS_EQUAL,
	NODE_GREATER_EQUAL,
	NODE_NOT,
	NODE_EQUAL,
	NODE_NOT_EQUAL,
	NODE_INCREMENT,
	NODE_DECREMENT,
	NODE_LOGICAL_AND,
	NODE_LOGICAL_OR,

	NODE_UNKNOWN
} node_t;

typedef union {
	int val;
	char c;
	char* name;
} NodeValue;

typedef struct {
	node_t type;
	NodeValue value;
	struct Node* left;
	struct Node* right;
	struct Node* prev;
	struct Node* next;
	struct type* t;
	bool node_free;
} Node;

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
	struct type* subtype;
	bool type_free;	
};

typedef struct {
	int size;
	int capacity;
	Token* end;

} Parser;

bool match_token(Parser* parser, char expected);
token_t peek_token_type(Parser* parser);
Token peek_token(Parser* parser);
token_t peek_next_token_type(Parser* parser);
Token advance_parser(Parser* parser);
bool at_token_eof(Parser* parser);

node_t get_op_kind(Token* token);
data_t get_type(Token* token);
struct type* create_type(data_t main_type, struct type* subtype);

Error create_error(error_t type);
void emit_error_msg(Parser* parser, FILE* file, Error* err);

Node* create_node(node_t type, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_int_node(node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* parse_factor(Parser* parser, FILE* file);
Node* parse_multiplicative(Parser* parser, FILE* file);
Node* parse_additive(Parser* parser, FILE* file);
Node* parse_relational(Parser* parser, FILE* file);
Node* parse_logical_and(Parser* parser, FILE* file);
Node* parse_logical_or(Parser* parser, FILE* file);

Node* parse_statement(Parser* parser, FILE* file);
Node* parse_block(Parser* parser, FILE* file);
Node* parse_parameters(Parser* parser, FILE* file);
Node* parse_function(Parser* parser, FILE* file);
Node* parse_args(Parser* parser, FILE* file);
Node* parse_array_list(Parser* parser, FILE* file);
Node* parse_let(Parser* parser, FILE* file);
Parser* initialize_parser(Token* tokens);
Node* parse(Token* tokens, FILE* file);

void print_ast(Node* root);


void free_type(struct type* t);
void free_expression(Node* node);
void free_parameter_list(Node* head);
void free_statement_list(Node* head);
void free_node(Node* node);
void free_ast(Node* root);
#endif