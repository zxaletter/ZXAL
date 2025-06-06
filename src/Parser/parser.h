#ifndef PARSER_H
#define PARSER_H

#include "Lexer/lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_NODES 1000
#define ERROR_CAPACITY 100
typedef enum {
	EXPECTED_LEFT_PARENTHESES,
	EXPECTED_RIGHT_PARENTHESES,
	EXPECTED_LEFT_BRACE,
	EXPECTED_RIGHT_BRACE,
	EXPECTED_LEFT_BRACKET,
	EXPECTED_RIGHT_BRACKET,

	EXPECTED_ARROW,
	EXPECTED_COMMA,
	EXPECTED_COLON,
	EXPECTED_SEMICOLON,
	EXPECTED_SINGLE_QUOTE,
	EXPECTED_DOUBLE_QUOTE,

	EXPECTED_ELSE_KEYWORD,
	EXPECTED_RETURN_KEYWORD,
	EXPECTED_LET_KEYWORD,
	EXPECTED_INT_KEYWORD,
	EXPECTED_CHAR_KEYWORD,
	EXPECTED_BOOL_KEYWORD,
	EXPECTED_VOID_KEYWORD,
	EXPECTED_STRUCT_KEYWORD,
	EXPECTED_ENUM_KEYWORD,
	EXPECTED_IDENTIFIER,
	EXPECTED_ASSIGNMENT,
	EXPECTED_DATATYPE,

} error_t;

typedef struct Error {
	error_t type;
	char* message;
	Token* token;
	FileInfo* info;
} Error;

typedef struct ErrorList {
	Error** list;
	int size;
	int capacity;
	int current_error;
} ErrorList;

typedef enum {
	NODE_INTEGER,
	NODE_IF,
	NODE_ELSE_IF,
	NODE_ELSE,
	NODE_FOR,
	NODE_WHILE,
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_STRUCT,
	NODE_ENUM,
	NODE_RETURN,
	NODE_CALL,
	NODE_SUBSCRIPT,
	NODE_PARAM,
	NODE_ARG, 
	NODE_ASSIGNMENT,
	NODE_ADDR,
	NODE_NAME,
	NODE_DECL,
	NODE_DEF,

	NODE_ADD,
	NODE_SUB,
	NODE_DIV,
	NODE_MUL,
	NODE_ADD_EQUAL,
	NODE_SUB_EQUAL,
	NODE_DIV_EQUAL,
	NODE_MUL_EQUAL,
	NODE_MODULO,

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

char* get_token_string(token_t type);
void repor_error(Token* tok, FileInfo* info, error_t error);
void init_error_list();
ErrorList* create_error_list();
bool add_error(Error* err);
void create_error(error_t type, char* message, Token* token, FileInfo* info);
void display_error(Error* error);
void emit_errors();
void free_error(Error* error);
void free_error_list();

Node* create_node(node_t type, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_int_node(node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* parse_factor(Parser* parser, FileInfo* info);
Node* parse_unary(Parser* parser, FileInfo* info);
Node* parse_multiplicative(Parser* parser, FileInfo* info);
Node* parse_additive(Parser* parser, FileInfo* info);
Node* parse_relational(Parser* parser, FileInfo* info);
Node* parse_logical_and(Parser* parser, FileInfo* info);
Node* parse_logical_or(Parser* parser, FileInfo* info);

Node* parse_statement(Parser* parser, FileInfo* info);
Node* parse_block(Parser* parser, FileInfo* info);
Node* parse_parameters(Parser* parser, FileInfo* info);
Node* parse_function(Parser* parser, FileInfo* info);
Node* parse_enum_body(Parser* parser, FileInfo* info);
Node* parse_enum(Parser* parser, FileInfo* info);
Node* parse_struct(Parser* parser, FileInfo* info);
Node* parse_switch(Parser* parser, FileInfo* info);
Node* parse_args(Parser* parser, FileInfo* info);
Node* parse_array_list(Parser* parser, FileInfo* info);
Node* parse_let(Parser* parser, FileInfo* info);
Parser* initialize_parser(Token* tokens);
Node* parse(Token* tokens, FileInfo* info);

char* get_type_name(data_t type);
void print_statement_recursive(Node* stmt, char* child_prefix, char* stmt_connector, bool* is_last_stmt);
void print_ast(Node* root);

void synchronize(Parser* parser, token_t* synchronizations);
Token* copy_token(Token* original_token);

void free_type(struct type* t);
void free_expression(Node* node);
void free_parameter_list(Node* head);
void free_statement_list(Node* head);
void free_node(Node* node);
void free_ast(Node* root);
#endif