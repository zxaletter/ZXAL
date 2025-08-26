#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct CompilerContext CompilerContext;
typedef struct Token Token;
typedef enum token_t token_t;

#define INITIAL_TOKEN_CAPACITY 250
// extern char* keywords[KEYWORDS];

typedef struct FileInfo {
	char* filename;
	int line_count;
	char* contents;
	char** lines;
} FileInfo;

typedef struct Lexer {
	int size;
	int capacity;
	
	char* start;
	char* end;

	int line;
	int column;
	
	Token* tokens;
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
keyword_t get_keyword_t(CompilerContext* ctx, char* identififer);

char peek_lexer(Lexer* lexer);
char peek_lexer_next(Lexer* lexer);
char advance_lexer(Lexer* lexer);
bool lexer_at_end(Lexer* lexer);
bool skip_lexer_whitespace(Lexer* lexer);
bool skip_lexer_comment(Lexer* lexer);

void get_string_literal(CompilerContext* ctx, Lexer* lexer);
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
#endif