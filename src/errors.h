#ifndef ERRORS_AND_CONTEXT_H
#define ERRORS_AND_CONTEXT_H

#include "Lexer/lexer.h"
#include "Lexer/token.h"
#include <stdbool.h>
#include "phases.h"

typedef struct CompilerContext CompilerContext;
#define ERROR_CAPACITY 100

typedef enum error_t {
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

	MISMATCHING_TYPES
} error_t;

typedef union {
	Token token;
} error_unit;

typedef struct Error {
	error_t type;
	error_unit unit; 
	char* message;
	int line;
	int column;
	FileInfo* info;
} Error;

typedef struct ErrorTable {
	phase_t phase;
	Error* errors;
	int size;
	size_t capacity;
} ErrorTable;

char* get_token_string(token_t type);
char* error_prelude(CompilerContext* ctx, char* filename, int line, int column);
void log_error(CompilerContext* ctx, Error e);
void display_lexer_error(CompilerContext* ctx, Error* e);
void display_parser_error(CompilerContext* ctx, Error* e);
void emit_errors(CompilerContext* ctx);

ErrorTable* create_error_tables(CompilerContext* ctx);
bool phase_accumulated_errors(CompilerContext* ctx);
#endif