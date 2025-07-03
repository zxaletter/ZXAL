#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "compilercontext.h"

#define KEYWORDS 20
#define INITIAL_TOKEN_CAPACITY 250
extern char* keyword[KEYWORDS];

typedef enum {
	TOKEN_ID, 
	TOKEN_CHAR_LITERAL,
	TOKEN_STR, 
	TOKEN_FUNCTION_KEYWORD, 
	TOKEN_LET_KEYWORD, // 4
	TOKEN_INT_KEYWORD, 
	TOKEN_CHAR_KEYWORD, 
	TOKEN_BOOL_KEYWORD, 
	TOKEN_VOID_KEYWORD, 
	TOKEN_STRUCT_KEYWORD, // 9
	TOKEN_ENUM_KEYWORD, // 10
	TOKEN_IF_KEYWORD, 
	TOKEN_ELSE_KEYWORD, 
	TOKEN_FOR_KEYWORD, 
	TOKEN_WHILE_KEYWORD, // 14
	TOKEN_CONTINUE_KEYWORD, // 15
	TOKEN_BREAK_KEYWORD, 
	TOKEN_RETURN_KEYWORD, 
	TOKEN_SWITCH_KEYWORD, 
	TOKEN_CASE_KEYWORD, // 19
	TOKEN_TRUE_KEYWORD,
	TOKEN_FALSE_KEYWORD,

	TOKEN_INTEGER, 
	TOKEN_LEFT_PARENTHESES, // 23
	TOKEN_RIGHT_PARENTHESES,  
	TOKEN_LEFT_BRACE, 
	TOKEN_RIGHT_BRACE, 
	TOKEN_LEFT_BRACKET, 
	TOKEN_RIGHT_BRACKET,

	TOKEN_ADD,  // 29
	TOKEN_SUB, 
	TOKEN_DIV, 
	TOKEN_MUL, 
	TOKEN_MODULO, 
	TOKEN_ADD_EQUAL,  // 34
	TOKEN_SUB_EQUAL, 
	TOKEN_DIV_EQUAL, 
	TOKEN_MUL_EQUAL, 
	
	TOKEN_LESS, 
	TOKEN_GREATER, // 39
	TOKEN_LESS_EQUAL, 
	TOKEN_GREATER_EQUAL, 
	TOKEN_NOT, 
	TOKEN_EQUAL, 
	TOKEN_NOT_EQUAL,  // 44
	TOKEN_INCREMENT, 
	TOKEN_DECREMENT, 
	TOKEN_LOGICAL_AND, 
	TOKEN_LOGICAL_OR, 
	TOKEN_ARROW,  // 49

	TOKEN_ASSIGNMENT, // 50
	TOKEN_COMMA, 
	TOKEN_COLON, // 52
	TOKEN_SEMICOLON, 
	TOKEN_AMPERSAND,  // 55
	TOKEN_PERIOD, 

	TOKEN_UNKNOWN, // 59
	TOKEN_EOF,
	TOKEN_STR_KEYWORD
} token_t;

typedef union {
	int val;
	char c;
	char* str;
} TokenValue;

typedef struct {
	token_t type;
	TokenValue value;
	int line;
	int column;
} Token;

typedef struct FileInfo {
	char* filename;
	int line_count;
	char* contents;
	char** lines;
} FileInfo;

typedef struct Lexer {
	char* start;
	char* end;
	int line;
	int column;
	Token* tokens;
	int tokenIdx;
	int capacity;
	FileInfo* info;
} Lexer;

typedef enum {
	KEYWORD_FUNCTION,
	KEYWORD_LET,
	KEYWORD_INT,
	KEYWORD_CHAR,
	KEYWORD_BOOL,
	KEYWORD_VOID,
	KEYWORD_STRUCT,
	KEYWORD_ENUM,
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_FOR,
	KEYWORD_WHILE,
	KEYWORD_CONTINUE,
	KEYWORD_BREAK,
	KEYWORD_RETURN,
	KEYWORD_SWITCH,
	KEYWORD_CASE,
	KEYWORD_TRUE,
	KEYWORD_FALSE,
	KEYWORD_STR,
	KEYWORD_UNKNOWN
} keyword_t;

typedef struct {
	keyword_t type;
	char* str;
} Keyword;

token_t key_t_to_token_t(keyword_t type);
keyword_t get_keyword_t(char* identififer);

char peek_lexer(Lexer* lexer);
char peek_lexer_next(Lexer* lexer);
char advance_lexer(Lexer* lexer);
bool lexer_at_end(Lexer* lexer);
bool skip_lexer_whitespace(Lexer* lexer);

void get_identifier(CompilerContext* ctx, Lexer* lexer);
void get_number(CompilerContext* ctx, Lexer* lexer);
void get_operator(CompilerContext* ctx, Lexer* lexer);
void get_delimeters(CompilerContext* ctx, Lexer* lexer);
bool match(Lexer* lexer, char expected);

Token create_token(token_t type, int line, int column);
Token create_char_token(token_t type, char c, int line, int column);
Token create_int_token(token_t type, int val, int line, int column);
Token create_string_token(CompilerContext* ctx, token_t type, char* str, int line, int column);
void add_token(CompilerContext* ctx, Lexer* lexer, Token token);

FileInfo* create_info(CompilerContext* ctx, char* filename, int line_count, char* contents);
FileInfo* retrieve_file_contents(CompilerContext* ctx, char* filename);
Lexer* initialze_lexer(CompilerContext* ctx, FileInfo* info);
Lexer* lex(CompilerContext* ctx, char* filename);

void print_tokens(Token* tokens);
void free_token(Token* tokens);
void free_duplicate_token(Token* token);
#endif