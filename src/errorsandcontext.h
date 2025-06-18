#ifndef ERRORS_AND_CONTEXT_H
#define ERRORS_AND_CONTEXT_H

#define ERROR_CAPACITY 100
#define CONTEXT_CAPACITY 10

#include "Lexer/lexer.h"

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
	CONTEXT_OP,
	CONTEXT_IF,
	CONTEXT_CALL,
	CONTEXT_RETURN,
	CONTEXT_AUG,
	CONTEXT_LOOP,
	CONTEXT_SWITCH,
	CONTEXT_VOID_FUNCTION,
	CONTEXT_NONVOID_FUNCTION
} context_t;

typedef struct ContextStack {
	int top;
	int size;
	int capacity;
	context_t* contexts;
} ContextStack;

bool is_context_stack_empty();
bool context_lookup(context_t context);
void push_context(context_t context);
void pop_context();
void init_context();
ContextStack* create_context();

#endif