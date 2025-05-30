#include "lexer.h"

char* keywords[KEYWORDS] = {"function", "let", "int", "char", "bool",
							"void", "struct", "enum", "if",
							"else", "for", "while",
						    "continue", "break", "return"};

char* get_file_contents(FILE* file) {
	if (!file) {
		printf("Error in opening file\n");
		return NULL;
	}

	fpos_t position;
	fgetpos(file, &position);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);

	fsetpos(file, &position);
	char* buffer = malloc(size + 1);
	if (!buffer) {
		printf("Error: Unable to allocate buffer for file contents\n");
		return NULL;
	}

	for (long i = 0; i < size; i++) {
		char c = fgetc(file);
		buffer[i] = c;
		if (feof(file)) { break; }
	}

	buffer[size] = '\0';
	return buffer;
}

Lexer* initialize_lexer(char* src) {
	struct Lexer* lexer = malloc(sizeof(struct Lexer));
	if (!lexer) {
		fprintf(stderr, "Error: Failed to allocate space for lexer\n");
		return NULL;
	}

	lexer->start = src;
	lexer->end = src;
	lexer->line = 1;
	lexer->column = 1;
	lexer->capacity = 250;
	lexer->tokenIdx = 0;
	lexer->tokens = malloc(sizeof(Token) * lexer->capacity);
	if (!lexer->tokens) {
		printf("Error: Unable to allocate space for tokens\n");
		free(lexer);
		return NULL;
	}

	return lexer;
}

char peek_lexer(Lexer* lexer) {
	if (*lexer->end == '\0') return '\0';
	printf("Character being returned from 'peek_lexer()' is '%c'\n", *lexer->end);
	return *lexer->end;
}

char peek_lexer_next(Lexer* lexer) {
	if (*lexer->end == '\0' || *(lexer->end + 1) == '\0') return '\0';
	lexer->end++;
	char c = *lexer->end;
	lexer->end--;
	return c;
}

char advance_lexer(Lexer* lexer) {
	if (*lexer->end != '\0') {
		if (*lexer->end == '\n') {
			lexer->column = 1;
			lexer->line++;
		} else {
			lexer->column++;
		}

	}
	char current = *lexer->end;
	lexer->end++;
	return current;
}

bool skip_lexer_whitespace(Lexer* lexer) {
	while (!lexer_at_end(lexer) && isspace(peek_lexer(lexer))) {
		advance_lexer(lexer);
	}
	return true;
}

bool lexer_at_end(Lexer* lexer) {
	return *lexer->end == '\0';
}

token_t key_t_to_token_t(keyword_t type) {
	switch (type) {
		case KEYWORD_FUNCTION: return TOKEN_FUNCTION_KEYWORD;
		case KEYWORD_LET: return TOKEN_LET_KEYWORD;
		case KEYWORD_INT: return TOKEN_INT_KEYWORD;
		case KEYWORD_CHAR: return TOKEN_CHAR_KEYWORD;
		case KEYWORD_BOOL: return TOKEN_BOOL_KEYWORD;
		case KEYWORD_VOID: return TOKEN_VOID_KEYWORD;
		case KEYWORD_STRUCT: return TOKEN_STRUCT_KEYWORD;
		case KEYWORD_ENUM: return TOKEN_ENUM_KEYWORD;
		case KEYWORD_IF: return TOKEN_IF_KEYWORD;
		case KEYWORD_ELSE: return TOKEN_ELSE_KEYWORD;
		case KEYWORD_FOR: return TOKEN_FOR_KEYWORD;
		case KEYWORD_WHILE: return TOKEN_WHILE_KEYWORD;
		case KEYWORD_CONTINUE: return TOKEN_CONTINUE_KEYWORD;
		case KEYWORD_BREAK: return TOKEN_BREAK_KEYWORD;
		case KEYWORD_RETURN: return TOKEN_RETURN_KEYWORD;
		default: return TOKEN_UNKNOWN;
	}
}

keyword_t get_keyword_t(char* identifier) {
	for (int i = 0; i < KEYWORDS; i++) {
		if (strcmp(keywords[i], identifier) == 0) {
			return (keyword_t)i;
		}
	}
	return KEYWORD_UNKNOWN;
}

Token create_token(token_t type, int line, int column) {
	Token token = {
		.type = type,
		.line = line,
		.column = column
	};

	return token;
}

Token create_char_token(token_t type, char c, int line, int column) {
	Token token = create_token(type, line, column);
	token.value.c = c;
	return token;
}

Token create_int_token(token_t type, int val, int line, int column) {
	Token token = create_token(type, line, column);
	token.value.val = val;
	return token;
}

Token create_string_token(token_t type, char* str, int line, int column) {
	Token token = create_token(type, line, column);
	token.value.str = strdup(str);
	return token;
}

void add_token(Lexer* lexer, Token token) {
	if (lexer->tokenIdx >= lexer->capacity) {
		lexer->capacity *= 2;
		lexer->tokens = realloc(lexer->tokens, lexer->capacity);
		if (!lexer->tokens) {
			printf("Error: Unable to reallocate 'lexer->tokens' in 'add_token'\n");
			return;
		}
	}

	lexer->tokens[lexer->tokenIdx++] = token;
}

void get_identifier(Lexer* lexer) {
	while (isalnum(peek_lexer(lexer)) || peek_lexer(lexer) == '_') {
		advance_lexer(lexer);
	}

	int length = lexer->end - lexer->start;
	char* identifier = malloc(length + 1);
	if (!identifier) {
		printf("Error: identifier is NULL\n");
		return;
	}

	strncpy(identifier, lexer->start, length);
	identifier[length] = '\0';

	keyword_t key_t = get_keyword_t(identifier);
	if (key_t != KEYWORD_UNKNOWN) {
		token_t tok_t = key_t_to_token_t(key_t);
		add_token(lexer, create_string_token(tok_t, identifier, lexer->line, lexer->column));
	} else {
		add_token(lexer, create_string_token(TOKEN_ID, identifier, lexer->line, lexer->column));
	}

	free(identifier);
}	

void get_number(Lexer* lexer) {
	bool isNegative = false;
	if (peek_lexer(lexer) == '-' && isdigit(peek_lexer_next(lexer))) {
		isNegative = true;
		advance_lexer(lexer);
	}

	while (isdigit(peek_lexer(lexer))) {
		advance_lexer(lexer);
	}

	int length = lexer->end - lexer->start;
	char* text = malloc(length + 1);
	if (!text) {
		printf("Error: Unable to allocate number text\n");
		return;
	}

	strncpy(text, lexer->start, length);
	text[length] = '\0';

	int number = atoi(text);
	if (isNegative) {
		number = -number;
	}

	add_token(lexer, create_int_token(TOKEN_INTEGER, number, lexer->line, lexer->column));
	free(text);
}

void get_delimeters(Lexer* lexer) {
	char c = advance_lexer(lexer);
	token_t type = TOKEN_UNKNOWN;

	switch (c) {
		case '(': type = TOKEN_LEFT_PARENTHESES; break;
		case ')': type = TOKEN_RIGHT_PARENTHESES; break;
		case '[': type = TOKEN_LEFT_BRACKET; break;
		case ']': type = TOKEN_RIGHT_BRACKET; break;
		case '{': type = TOKEN_LEFT_BRACE; break;
		case '}': type = TOKEN_RIGHT_BRACE; break;
		case ';': type = TOKEN_SEMICOLON; break;
		case ':': type = TOKEN_COLON; break;
		case ',': type = TOKEN_COMMA; break;
		case '\'': type = TOKEN_SINGLE_QUOTE; break;
	}

	add_token(lexer, create_char_token(type, c, lexer->line, lexer->column - 1));
}

bool match(Lexer* lexer, char expected) {
	if (peek_lexer(lexer) != expected) return false;
	advance_lexer(lexer);
	return true;
}

void get_operator(Lexer* lexer) {
	char c = advance_lexer(lexer);
	bool isCompoundOp = false;
	token_t type = TOKEN_UNKNOWN;

	switch (c) {
		case '+': {
			if (match(lexer, '=')) {
				type = TOKEN_ADD_EQUAL;
				isCompoundOp = true;
			} else if (match(lexer, '+')) {
				type = TOKEN_INCREMENT;
				isCompoundOp = true;
			} else {
				type = TOKEN_ADD;
			}
			break;
		}

		case '-': {
			if (match(lexer, '=')) {
				type = TOKEN_SUB_EQUAL;
				isCompoundOp = true;
			} else if (match(lexer, '-')) {
				type = TOKEN_DECREMENT;
				isCompoundOp = true;
			} else if (match(lexer, '>')) {
				printf(" we have lexed an arrow\n");
				type = TOKEN_ARROW;
				isCompoundOp = true;
			} else {
				type = TOKEN_SUB;
			}
			break;
		}

		case '*': {
			if (match(lexer, '=')) {
				type = TOKEN_MUL_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_MUL;
			}
			break;
		}

		case '/': {
			if (match(lexer, '=')) {
				type = TOKEN_DIV_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_DIV;
			}
			break;
		}

		case '<': {
			if (match(lexer, '=')) {
				type = TOKEN_LESS_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_LESS;
			}
			break;
		}

		case '>': {
			if (match(lexer, '=')) {
				type = TOKEN_GREATER_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_GREATER;
			}
			break;
		}

		case '!': {
			if (match(lexer, '=')) {
				type = TOKEN_NOT_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_NOT;
			}
			break;
		}

		case '=': {
			if (match(lexer, '=')) {
				type = TOKEN_EQUAL;
				isCompoundOp = true;
			} else {
				type = TOKEN_ASSIGNMENT;
			}
			break;
		}

		case '&': {
			if (match(lexer, '&')) {
				type = TOKEN_LOGICAL_AND;
				isCompoundOp = true;
			} else {
				type = TOKEN_AMPERSAND;
			}
			break;
		}

		case '|': {
			if (match(lexer, '|')) {
				type = TOKEN_LOGICAL_OR;
				isCompoundOp = true;
			} else {
				type = TOKEN_UNKNOWN;
			}
			break;
		}

		case '%': {
			type = TOKEN_MODULO;
			break;
		}

	}

	if (isCompoundOp) {
		char str_opr[3] = {c, *(lexer->end - 1), '\0'};
		add_token(lexer, create_string_token(type, str_opr, lexer->line, lexer->column - 2));
	} else {
		add_token(lexer, create_char_token(type, c, lexer->line, lexer->column - 1));
	}
}

Token* lex(FILE* file) {
	char* contents = get_file_contents(file);

	printf("\n\nHere is the file Contents: \n'%s'\n", contents);
	
	Lexer* lexer = initialize_lexer(contents);
	if (!lexer) return NULL;

	while (!lexer_at_end(lexer)) {
		skip_lexer_whitespace(lexer);
		
		if (lexer_at_end(lexer)) { break; }

		lexer->start = lexer->end;

		if (isalpha(peek_lexer(lexer))) {
			get_identifier(lexer);
		} else if (isdigit(peek_lexer(lexer)) || (peek_lexer(lexer) == '-' && isdigit(peek_lexer_next(lexer)))) {
			get_number(lexer);
		} else if (strchr("=+-*/<!&>|%", peek_lexer(lexer))) {
			get_operator(lexer);
		} else if (strchr("':()[]{},;", peek_lexer(lexer))) {
			get_delimeters(lexer);
		} else {
			advance_lexer(lexer);
		}

	}

	add_token(lexer, create_token(TOKEN_EOF, lexer->line, lexer->column));
	Token* tokens = lexer->tokens;
	free(lexer);
	return tokens;
}

void free_token(Token* token) {
	if (token->type == TOKEN_ARROW || 
		token->type == TOKEN_ADD_EQUAL ||
		token->type == TOKEN_SUB_EQUAL ||
		token->type == TOKEN_DIV_EQUAL ||
		token->type == TOKEN_MUL_EQUAL ||
		token->type == TOKEN_LESS_EQUAL ||
		token->type == TOKEN_GREATER_EQUAL ||
		token->type == TOKEN_NOT_EQUAL ||
		token->type == TOKEN_EQUAL ||
		token->type == TOKEN_INCREMENT ||
		token->type == TOKEN_DECREMENT ||
		token->type == TOKEN_INT_KEYWORD ||
		token->type == TOKEN_CHAR_KEYWORD ||
		token->type == TOKEN_BOOL_KEYWORD ||
		token->type == TOKEN_VOID_KEYWORD ||
		token->type == TOKEN_STRUCT_KEYWORD ||
		token->type == TOKEN_ENUM_KEYWORD ||
		token->type == TOKEN_FOR_KEYWORD ||
		token->type == TOKEN_WHILE_KEYWORD ||
		token->type == TOKEN_CONTINUE_KEYWORD ||
		token->type == TOKEN_BREAK_KEYWORD ||
		token->type == TOKEN_FUNCTION_KEYWORD ||
		token->type == TOKEN_LET_KEYWORD ||
		token->type == TOKEN_ID ||
		token->type == TOKEN_LOGICAL_OR ||
		token->type == TOKEN_LOGICAL_AND
	) {
		free(token->value.str);
	}

}

void free_tokens(Token* tokens) {
	for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
		free_token(&tokens[i]);
	}

	free(tokens);
}

void print_tokens(Token* tokens) {
	for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
		switch (tokens[i].type) {
			case TOKEN_ARROW:
			case TOKEN_ADD_EQUAL:
			case TOKEN_SUB_EQUAL:
			case TOKEN_DIV_EQUAL:
			case TOKEN_MUL_EQUAL:
			case TOKEN_LESS_EQUAL:
			case TOKEN_GREATER_EQUAL:
			case TOKEN_NOT_EQUAL:
			case TOKEN_EQUAL:
			case TOKEN_INCREMENT:
			case TOKEN_DECREMENT:
			case TOKEN_INT_KEYWORD:
			case TOKEN_CHAR_KEYWORD:
			case TOKEN_BOOL_KEYWORD:
			case TOKEN_VOID_KEYWORD:
			case TOKEN_STRUCT_KEYWORD:
			case TOKEN_ENUM_KEYWORD:
			case TOKEN_FOR_KEYWORD:
			case TOKEN_WHILE_KEYWORD:
			case TOKEN_CONTINUE_KEYWORD:
			case TOKEN_BREAK_KEYWORD:	
			case TOKEN_FUNCTION_KEYWORD:
			case TOKEN_RETURN_KEYWORD:
			case TOKEN_IF_KEYWORD:
			case TOKEN_ELSE_KEYWORD:
			case TOKEN_LET_KEYWORD:
			case TOKEN_LOGICAL_AND:
			case TOKEN_LOGICAL_OR:
			case TOKEN_ID: {
				printf("TOKEN: %s\n", tokens[i].value.str);
				break;
			}

			case TOKEN_INTEGER: {
				printf("TOKEN: %d\n", tokens[i].value.val);
				break;
			}

			case TOKEN_LEFT_PARENTHESES:
			case TOKEN_RIGHT_PARENTHESES:
			case TOKEN_LEFT_BRACE:
			case TOKEN_RIGHT_BRACE:
			case TOKEN_LEFT_BRACKET:
			case TOKEN_RIGHT_BRACKET:
			case TOKEN_ADD:
			case TOKEN_SUB:
			case TOKEN_DIV:
			case TOKEN_MUL:
			case TOKEN_MODULO:
			case TOKEN_LESS:
			case TOKEN_GREATER:
			case TOKEN_NOT:
			case TOKEN_ASSIGNMENT:
			case TOKEN_COMMA:
			case TOKEN_COLON:
			case TOKEN_SEMICOLON:
			case TOKEN_AMPERSAND:
			case TOKEN_SINGLE_QUOTE:
			case TOKEN_PERIOD: {
				printf("TOKEN: %c\n", tokens[i].value.c);
				break;				
			}
		}
	}
}

