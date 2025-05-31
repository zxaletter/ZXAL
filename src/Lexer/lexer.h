#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define KEYWORDS 15
extern char* keyword[KEYWORDS];

typedef enum {
	TOKEN_ID, // 0
	TOKEN_FUNCTION_KEYWORD, // 1
	TOKEN_LET_KEYWORD, // 2
	TOKEN_INT_KEYWORD, // 3
	TOKEN_CHAR_KEYWORD, // 4
	TOKEN_BOOL_KEYWORD, // 5
	TOKEN_VOID_KEYWORD, // 6
	TOKEN_STRUCT_KEYWORD, // 7
	TOKEN_ENUM_KEYWORD, // 8
	TOKEN_IF_KEYWORD, // 9
	TOKEN_ELSE_KEYWORD, // 10
	TOKEN_FOR_KEYWORD, // 11
	TOKEN_WHILE_KEYWORD, // 12
	TOKEN_CONTINUE_KEYWORD, // 13
	TOKEN_BREAK_KEYWORD, // 14
	TOKEN_RETURN_KEYWORD, // 15

	TOKEN_INTEGER, // 16
	TOKEN_LEFT_PARENTHESES, // 17
	TOKEN_RIGHT_PARENTHESES, // 18
	TOKEN_LEFT_BRACE, // 19
	TOKEN_RIGHT_BRACE, // 20
	TOKEN_LEFT_BRACKET, // 21
	TOKEN_RIGHT_BRACKET, // 22

	TOKEN_ADD, // 23
	TOKEN_SUB, // 24
	TOKEN_DIV, // 25
	TOKEN_MUL, // 26
	TOKEN_MODULO, // 27
	TOKEN_ADD_EQUAL, // 28
	TOKEN_SUB_EQUAL, // 29
	TOKEN_DIV_EQUAL, // 30
	TOKEN_MUL_EQUAL, // 31
	
	TOKEN_LESS, // 32
	TOKEN_GREATER, // 33
	TOKEN_LESS_EQUAL, // 34
	TOKEN_GREATER_EQUAL, // 35
	TOKEN_NOT, // 36
	TOKEN_EQUAL, // 37
	TOKEN_NOT_EQUAL, // 38
	TOKEN_INCREMENT, // 39
	TOKEN_DECREMENT, // 40
	TOKEN_LOGICAL_AND, // 41
	TOKEN_LOGICAL_OR, // 42
	TOKEN_ARROW, // 43

	TOKEN_ASSIGNMENT, // 44
	TOKEN_COMMA, // 45
	TOKEN_COLON, // 46
	TOKEN_SEMICOLON, // 47
	TOKEN_AMPERSAND, // 48
	TOKEN_PERIOD, // 49
	TOKEN_SINGLE_QUOTE, // 50

	TOKEN_UNKNOWN, // 51
	TOKEN_EOF // 52
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

typedef struct Lexer {
	char* start;
	char* end;
	int line;
	int column;
	Token* tokens;
	int tokenIdx;
	int capacity;

} Lexer;

// Members need to be in corresponding order as elements of keywords

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

void get_identifier(Lexer* lexer);
void get_number(Lexer* lexer);
void get_operator(Lexer* lexer);
void get_delimeters(Lexer* lexer);
bool match(Lexer* lexer, char expected);

Token create_token(token_t type, int line, int column);
Token create_char_token(token_t type, char c, int line, int column);
Token create_int_token(token_t type, int val, int line, int column);
Token create_string_token(token_t type, char* str, int line, int column);
void add_token(Lexer* lexer, Token token);

char* get_file_contents(FILE* file);
Lexer* initialze_lexer(char* contents);
Token* lex(FILE* file);

void print_tokens(Token* tokens);
void free_tokens(Token* tokens);
#endif