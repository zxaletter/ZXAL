#include "lexer.h"
#include "compilercontext.h"
#include "token.h"
#include "assert.h"
#include "errors.h"

Lexer* initialize_lexer(CompilerContext* ctx, FileInfo* info) {
	Lexer* lexer = arena_allocate(ctx->lexer_arena, sizeof(Lexer));
	if (!lexer) {
		fprintf(stderr, "Error: Failed to allocate space for lexer\n");
		return NULL;
	}

	lexer->start = info->contents;
	lexer->end = info->contents;
	lexer->line = 1;
	lexer->column = 1;
	lexer->capacity = INITIAL_TOKEN_CAPACITY;
	lexer->size = 0;
	lexer->tokens = arena_allocate(ctx->lexer_arena, sizeof(Token) * lexer->capacity);
	if (!lexer->tokens) {
		printf("Error: Unable to allocate space for tokens\n");
		return NULL;
	}
	lexer->info = info;
	
	return lexer;
}

char peek_lexer(Lexer* lexer) {
	if (lexer_at_end(lexer)) return '\0';
	return *lexer->end;
}

char peek_lexer_next(Lexer* lexer) {
	if (lexer_at_end(lexer) || *(lexer->end + 1) == '\0') return '\0';
	lexer->end++;
	char c = *lexer->end;
	lexer->end--;
	return c;
}

char advance_lexer(Lexer* lexer) {
	if (lexer_at_end(lexer)) return '\0';

	if (*lexer->end == '\n') {
		lexer->column = 1;
		lexer->line++;
	} else {
		lexer->column++;
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

bool skip_lexer_comment(Lexer* lexer) {
	while (!lexer_at_end(lexer) && peek_lexer(lexer) != '\n') {
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
		case KEYWORD_SWITCH: return TOKEN_SWITCH_KEYWORD;
		case KEYWORD_CASE: return TOKEN_CASE_KEYWORD;
		case KEYWORD_TRUE: return TOKEN_TRUE_KEYWORD;
		case KEYWORD_FALSE: return TOKEN_FALSE_KEYWORD;
		default: return TOKEN_UNKNOWN;
	}
}

keyword_t get_keyword_t(CompilerContext* ctx, char* identifier) {
	for (int i = 0; i < KEYWORDS; i++) {
		if (strcmp(ctx->keywords[i], identifier) == 0) {
			return (keyword_t)i;
		}
	}
	return KEYWORD_UNKNOWN;
}

Token create_char_token(token_t type, char c, int line, int column) {
	Token t = {
		.type = type,
		.line = line,
		.column = column
	};
	t.value.c = c;
	return t;
}

Token create_int_token(token_t type, int val, int line, int column) {
	Token t = {
		.type = type,
		.line = line,
		.column = column
	};
	t.value.val = val;
	return t;
}

Token create_string_token(CompilerContext* ctx, token_t type, char* str, int line, int column) {
	Token t = {
		.type = type,
		.line = line,
		.column = column
	};

	int length = strlen(str);
	t.value.str = arena_allocate(ctx->lexer_arena, length + 1);
	if (!t.value.str) {
		TokenValue error_val = {
			.str = NULL
		};

		Token error_token = {
			.type = TOKEN_UNKNOWN,
			.value = error_val,
			.line = -1,
			.column = -1,
		};
		return error_token;
	}
	strncpy(t.value.str, str, length);
	t.value.str[length] = '\0';

	return t;
}

void add_token(CompilerContext* ctx, Lexer* lexer, Token token) {
	if (lexer->size >= lexer->capacity) {
		size_t prev_capacity = lexer->capacity;

		lexer->capacity *= 2;
		size_t new_capacity = lexer->capacity;
		void* new_tokens = arena_reallocate(
			ctx->lexer_arena, 
			lexer->tokens, 
			prev_capacity * sizeof(Token), 
			new_capacity * sizeof(Token)
		);
		
		assert(new_tokens);
		lexer->tokens = new_tokens;
	}
	lexer->tokens[lexer->size++] = token;
}

void get_identifier(CompilerContext* ctx, Lexer* lexer) {
	while (!lexer_at_end(lexer) && (isalnum(peek_lexer(lexer)) || peek_lexer(lexer) == '_')) {
		advance_lexer(lexer);
	}

	int length = lexer->end - lexer->start;
	char* identifier = arena_allocate(ctx->lexer_arena, length + 1);
	assert(identifier);

	strncpy(identifier, lexer->start, length);
	identifier[length] = '\0';

	keyword_t key_t = get_keyword_t(ctx, identifier);
	if (key_t != KEYWORD_UNKNOWN) {
		token_t tok_t = key_t_to_token_t(key_t);
		add_token(ctx, lexer, create_string_token(ctx, tok_t, identifier, lexer->line, lexer->column));
	} else {
		add_token(ctx, lexer, create_string_token(ctx, TOKEN_ID, identifier, lexer->line, lexer->column));
	}
}	

void get_number(CompilerContext* ctx, Lexer* lexer) {
	while (!lexer_at_end(lexer) && isdigit(peek_lexer(lexer))) {
		advance_lexer(lexer);
	}

	int length = lexer->end - lexer->start;
	char* text = arena_allocate(ctx->lexer_arena, length + 1);
	assert(text);

	strncpy(text, lexer->start, length);
	text[length] = '\0';

	int number = atoi(text);
	add_token(ctx, lexer, create_int_token(TOKEN_INTEGER, number, lexer->line, lexer->column));
}

void get_string_literal(CompilerContext* ctx, Lexer* lexer) {
	while (!lexer_at_end(lexer) && (isalnum(peek_lexer(lexer)) || peek_lexer(lexer) == '_')) {
		advance_lexer(lexer);
	}

	if (peek_lexer(lexer) != '\"') {
		char* message = error_prelude(ctx, lexer->info->filename, lexer->line, lexer->column);
		Error e = {
			.type = EXPECTED_DOUBLE_QUOTE,
			.message = message,
			.line = lexer->line,
			.column = lexer->column
		};
		log_error(ctx, e);
		return;
	}
	int length = lexer->end - lexer->start;
	char* identifier = arena_allocate(ctx->lexer_arena, length + 1);
	if (!identifier) return;

	strncpy(identifier, lexer->start, length);
	identifier[length] = '\0';

	add_token(ctx, lexer, create_string_token(ctx, TOKEN_STR_LITERAL, identifier, lexer->line, lexer->column));
}

void get_delimeters(CompilerContext* ctx, Lexer* lexer) {
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
		case '\'': {
			add_token(ctx, lexer, create_char_token(TOKEN_CHAR_LITERAL, peek_lexer(lexer), lexer->line, lexer->column));
			advance_lexer(lexer);
			if (peek_lexer(lexer) != '\'') {
				char* message = error_prelude(ctx, lexer->info->filename, lexer->line, lexer->column);
				Error e = {
					.type = EXPECTED_SINGLE_QUOTE,
					.message = message,
					.line = lexer->line,
					.column = lexer->column
				};
				log_error(ctx, e);
			} else {
				advance_lexer(lexer);
			}
			return;
		}

		case '\"': {
			advance_lexer(lexer);
			get_string_literal(ctx, lexer);
			return;
		}
	}
	add_token(ctx, lexer, create_char_token(type, c, lexer->line, lexer->column - 1));
}

bool match(Lexer* lexer, char expected) {
	if (peek_lexer(lexer) != expected) return false;
	advance_lexer(lexer);
	return true;
}

void get_operator(CompilerContext* ctx, Lexer* lexer) {
	char c = advance_lexer(lexer);
	bool isCompoundOp = false;
	bool has_comment = false;
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
			} else if (match(lexer, '/')) {
				bool has_comment = true;
				skip_lexer_comment(lexer);
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
				printf("\033[31mGot unknown token in lexer\033[0m\n");
				type = TOKEN_UNKNOWN;
			}
			break;
		}

		case '%': {
			type = TOKEN_MODULO;
			break;
		}
	}

	if (has_comment) return;
	if (type == TOKEN_UNKNOWN) return;

	if (isCompoundOp) {
		char str_opr[3] = {c, *(lexer->end - 1), '\0'};
		add_token(ctx, lexer, create_string_token(ctx, type, str_opr, lexer->line, lexer->column - 2));
	} else {
		add_token(ctx, lexer, create_char_token(type, c, lexer->line, lexer->column - 1));
	}
}

FileInfo* create_info(CompilerContext* ctx, char* filename, int line_count, char* contents) {
	FileInfo* info = arena_allocate(ctx->lexer_arena, sizeof(FileInfo));
	if (!info) {
		perror("Failed to create file info\n");
		return NULL;
	}

	info->filename = filename;
	info->line_count = line_count;
	info->contents = contents;
	info->lines = NULL;

	return info;
}

FileInfo* retrieve_file_contents(CompilerContext* ctx, char* filename) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("Could not open file\n");
		return NULL;
	}
	fpos_t position;
	fgetpos(file, &position);
	
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	
	fsetpos(file, &position);

	char* buffer = arena_allocate(ctx->lexer_arena, file_size + 1);
	if (!buffer) {
		printf("Could not allocate buffer\n");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	int line_count = 0;
	for (long i = 0; i < file_size; i++) {
		char c = fgetc(file);
		if (c == '\n') {
			line_count++;
		}
		if (feof(file)) { break; }
		buffer[i] = c;
	}
	buffer[file_size] = '\0';
	
	if (file_size > 0 && buffer[file_size - 1] != '\n') {
	    line_count++;
	}
	fsetpos(file, &position);

	FileInfo* info = create_info(ctx, filename, line_count, buffer);
	if (!info) {
		fclose(file);
		return NULL;
	}

	if (line_count > 0) {
		info->lines = arena_allocate(ctx->lexer_arena, sizeof(char*) * line_count);
		if (!info->lines) {
			perror("Failed to allocate space for info->lines\n");
			fclose(file);
			return NULL;
		}
	} else {
		info->lines = NULL;
	}

	int current_line_index = 0;
	char* line_start = buffer;
	for (long i = 0; i < file_size; i++) {
		if (buffer[i] == '\n') {
			if (current_line_index < line_count) {
				int length = &buffer[i] - line_start;
				info->lines[current_line_index] = arena_allocate(ctx->lexer_arena, length + 1);
				if (!info->lines[current_line_index]) {
					fclose(file);
					return NULL;
				}
				strncpy(info->lines[current_line_index], line_start, length);	
				info->lines[current_line_index][length] =  '\0';
				current_line_index++;

			}
			line_start = &buffer[i + 1];
		}
	}


	if (line_start < (buffer + file_size) && current_line_index < line_count) {
		long line_length = (buffer + file_size) - line_start;
		info->lines[current_line_index] = arena_allocate(ctx->lexer_arena, line_length + 1);
		if (!info->lines[current_line_index]) {
			perror("Failed to allocate memory for the last line\n");
			
			fclose(file);
			return NULL;
		}
		strncpy(info->lines[current_line_index], line_start, line_length);
		info->lines[current_line_index][line_length] = '\0';
		current_line_index++;
	}

	fclose(file);
	return info;
}

Lexer* lex(CompilerContext* ctx, char* filename) {
	ctx->phase = PHASE_LEXER;

	FileInfo* info = retrieve_file_contents(ctx, filename);
	Lexer* lexer = initialize_lexer(ctx, info);
	assert(info && lexer);

	while (!lexer_at_end(lexer)) {
		skip_lexer_whitespace(lexer);
		
		lexer->start = lexer->end;

		if (isalpha(peek_lexer(lexer))) {
			get_identifier(ctx, lexer);
		} else if (isdigit(peek_lexer(lexer))) {
			get_number(ctx, lexer);
		} else if (strchr("=+-*/<!&>|%", peek_lexer(lexer))) {
			get_operator(ctx, lexer);
		} else if (strchr("':()[]{},\";", peek_lexer(lexer))) {
			get_delimeters(ctx, lexer);
		} else {
			advance_lexer(lexer);
		}
	}
	Token eof_token = {
		.type = TOKEN_EOF,
		.line = lexer->line,
		.column = lexer->column
	};
	add_token(ctx, lexer, eof_token);
	return lexer;
}

void print_tokens(Token* tokens) {
	if (!tokens) return;
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
			case TOKEN_SWITCH_KEYWORD:
			case TOKEN_CASE_KEYWORD:
			case TOKEN_TRUE_KEYWORD:
			case TOKEN_FALSE_KEYWORD:
			case TOKEN_IF_KEYWORD:
			case TOKEN_ELSE_KEYWORD:
			case TOKEN_LET_KEYWORD:
			case TOKEN_LOGICAL_AND:
			case TOKEN_LOGICAL_OR:
			case TOKEN_ID: {
				printf("TOKEN TYPE: %d TOKEN: %s\n", tokens[i].type, tokens[i].value.str);
				break;
			}

			case TOKEN_INTEGER: {
				printf("TOKEN TYPE: %d TOKEN: %d\n", tokens[i].type, tokens[i].value.val);
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
			case TOKEN_CHAR_LITERAL:
			case TOKEN_PERIOD: {
				printf("TOKEN TYPE: %d, TOKEN: %c\n", tokens[i].type, tokens[i].value.c);
				break;				
			}
		}
	}
}