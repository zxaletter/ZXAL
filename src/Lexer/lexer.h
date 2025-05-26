#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include <stdio.h>

typedef enum {
	TOKEN_INTEGER,
	TOKEN_ID,
	TOKEN_KEYWORD,

	TOKEN_LEFT_PARENTHESES,
	TOKEN_RIGHT_PARENTHESES,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,

	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_DIV,
	TOKEN_MUL,
	TOKEN_ADD_EQUAL,
	TOKEN_SUB_EQUAL,
	TOKEN_DIV_EQUAL,
	TOKEN_MUL_EQUAL,

	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_AMPERSAND,
	TOKEN_HYPEN,
	TOKEN_RIGHT_ANGLE_BRACKET,
	TOKEN_PERIOD,

	TOKEN_UNKNOWN,
	TOKEN_EOF
} TokenType;

typedef union {
	int val;
	char c;
	char* str;
} TokenValue;


typedef struct {
	TokenType type;
	TokenValue value;
	int line;
	int column;
} Token;

typedef struct Lexer {
	char* start;
	char* end;

	int line;
	int column;
} Lexer;

Lexer* initialze_lexer(char* src);
char* get_file_contents(char* name);
#endif