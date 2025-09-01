#ifndef ERRORS_AND_CONTEXT_H
#define ERRORS_AND_CONTEXT_H

#define ERROR_CAPACITY 100

#include "Lexer/lexer.h"
#include "compilercontext.h"

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

typedef struct ErrorTable {
	phase_t phase;
	Error* errors;
	int size;
	int capacity;
} ErrorTable;

void log_error(CompilerContext* ctx, Token* tok, FileInfo* info, error_t error);
void init_error_table(CompilerContext* ctx);
void add_error_to_error_table(CompilerContext* ctx, Error* err);
void create_error(CompilerContext* ctx, error_t type, char* message, Token* token, FileInfo* info);
void display_error(CompilerContext* ctx, Error* error);
void emit_errors(CompilerContext* ctx);

ErrorTable* create_error_tables(CompilerContext* ctx);

#endif