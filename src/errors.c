#include "errors.h"
#include "compilercontext.h"
#include "Lexer/token.h"
#include "assert.h"

// void create_error(CompilerContext* ctx, error_t type, char* message, Token* token, FileInfo* info) {
// 	Error* error = arena_allocate(ctx->error_arena, sizeof(Error));
// 	if (!error) {
// 		perror("Unable to allocate space for error\n");
// 		return;
// 	}

// 	error->type = type;
// 	error->token = copy_token(ctx, token);
// 	if (!error->token) {
// 		perror("In 'create_error', unable to copy token\n");
// 		return;
// 	}
// 	error->info = info;
// 	error->message = arena_allocate(ctx->error_arena, sizeof(message) + 1);
// 	if (!error->message) {
// 		perror("Unable to duplicate error message\n");
// 		return;
// 	} 
// }

char* get_token_string(token_t type) {
	switch (type) {
		case TOKEN_LET_KEYWORD: return "let";
		case TOKEN_INT_KEYWORD: return "int";
		case TOKEN_CHAR_KEYWORD: return "char";
		case TOKEN_BOOL_KEYWORD: return "bool";
		case TOKEN_VOID_KEYWORD: return "void";
		case TOKEN_STRUCT_KEYWORD: return "struct";
		case TOKEN_ENUM_KEYWORD: return "enum";
		case TOKEN_IF_KEYWORD: return "if";
		case TOKEN_ELSE_KEYWORD: return "else";
		case TOKEN_FOR_KEYWORD: return "for";
		case TOKEN_WHILE_KEYWORD: return "while";
		case TOKEN_CONTINUE_KEYWORD: return "continue";
		case TOKEN_BREAK_KEYWORD: return "break";
		case TOKEN_RETURN_KEYWORD: return "return";
		case TOKEN_LEFT_PARENTHESES: return "(";
		case TOKEN_RIGHT_PARENTHESES: return ")";
		case TOKEN_LEFT_BRACE: return "{";
		case TOKEN_RIGHT_BRACE: return "}";
		case TOKEN_LEFT_BRACKET: return "[";
		case TOKEN_RIGHT_BRACKET: return "]";
		case TOKEN_ARROW: return "->";
		case TOKEN_COMMA: return ",";
		case TOKEN_COLON: return ":";
		case TOKEN_SEMICOLON: return ";";
		case TOKEN_ID: return "identifier";
	}
}

void display_error(CompilerContext* ctx, Error* e) {
	printf("%s", e->message);

	int token_length = 1;

	switch (e->token->type) {
		case TOKEN_FUNCTION_KEYWORD:
		case TOKEN_LET_KEYWORD:
		case TOKEN_INT_KEYWORD:
		case TOKEN_CHAR_KEYWORD:
		case TOKEN_BOOL_KEYWORD:
		case TOKEN_VOID_KEYWORD:
		case TOKEN_STRUCT_KEYWORD:
		case TOKEN_ENUM_KEYWORD:
		case TOKEN_IF_KEYWORD:
		case TOKEN_ELSE_KEYWORD:
		case TOKEN_FOR_KEYWORD:
		case TOKEN_WHILE_KEYWORD:
		case TOKEN_CONTINUE_KEYWORD:
		case TOKEN_BREAK_KEYWORD:
		case TOKEN_RETURN_KEYWORD:
		case TOKEN_ID: {
			if (e->token->value.str) {
				token_length = strlen(e->token->value.str);
			} 
			break;
		}

		case TOKEN_INTEGER: {
			int length = snprintf(NULL, 0, "%d", e->token->value.val);
			token_length = length;
			break;
		}
		default: {
			token_length = 1; 
			break;
		}
	}

	int gutter_width = snprintf(NULL, 0, "%d", e->token->line);
	char space[gutter_width + 1];
	for (int i = 0; i < gutter_width; i++) {
		space[i] = ' ';
	}
	space[gutter_width] = '\0';

	switch (e->type) {
		case EXPECTED_IDENTIFIER: {
			printf("error: missing %s\n", get_token_string(TOKEN_ID));
			
			printf("%s%s-> %s:%d:%d\n", space, space, e->info->filename, e->line, e->column);
			
			printf("%s%s|\n", space, space);

			printf("%d%s| %s\n", e->line, space, e->info->lines[e->line - 1]);
			printf("%s%s|", space, space);
			char buffer[2 * gutter_width + 1];
			snprintf(buffer, sizeof(buffer), "%s%s|", space, space);

			for (int i = 1; i <= e->column - 1; i++) {
				printf(" ");
			}

			printf("\033[31m^\033[0m");
			for (int i = 0; i < token_length - 1; i++) {
				printf("\033[31m^\033[0m");
			}
			printf("\n");
			break;
		}

		case EXPECTED_ARROW: { 
			printf("error: missing token \"%s\"\n", get_token_string(TOKEN_ARROW));
			break;
		}

		case EXPECTED_COMMA: {
			printf("Syntax Error: Missing '%s'\n", get_token_string(TOKEN_COMMA));
			break;
		}
		case EXPECTED_SEMICOLON: {
			printf("Syntax Error: Missing '%s'\n", get_token_string(TOKEN_SEMICOLON));
			break;
		}

		case EXPECTED_COLON: {
			printf("Syntax Error: Missing '%s'\n", get_token_string(TOKEN_COLON));
			break;
		}

		case EXPECTED_ASSIGNMENT: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_ASSIGNMENT));
			break;
		}

		case EXPECTED_DOUBLE_QUOTE: {
			printf("Syntax Error: Expected '"'\n');
			break;
		}

		case EXPECTED_SINGLE_QUOTE: {
			printf("Syntax Error: Expected '\''\n");
			break;
		}

		case EXPECTED_LEFT_BRACKET: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_LEFT_BRACKET));
			break;
		}
		case EXPECTED_RIGHT_BRACKET: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_RIGHT_BRACKET));
			break;
		}

		case EXPECTED_LEFT_BRACE: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_LEFT_BRACE));
			break;
		}

		case EXPECTED_RIGHT_BRACE: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_RIGHT_BRACE));
			break;
		}

		case EXPECTED_RIGHT_PARENTHESES: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_RIGHT_PARENTHESES));
			break;
		}

		case EXPECTED_LEFT_PARENTHESES: {
			printf("Syntax Error: Expected '%s'\n", get_token_string(TOKEN_LEFT_PARENTHESES));
			break;
		}
	}
}

void emit_errors(CompilerContext* ctx) {
	for (int i = 0; i < NUM_PHASES; i++) {
		ErrorTable* table = &ctx->error_tables[i];
		if (table && (table->phase == ctx->phase)) {
			for (int j = 0; j < table->size; j++) {
				display_error(ctx, &table->errors[i]);
			}
		}
	}
}

char* error_prelude(CompilerContext* ctx, char* filename, int line, int column) {
	char* message = arena_allocate(ctx->error_arena, 100);
	assert(message);

	snprintf(message, 1024, "\033[31mError\033[0m in file '%s' at line %d, column %d:\n", filename, line, column);
	return message;
}

void log_error(CompilerContext* ctx, Error e) {
	for (int i = 0; i < NUM_PHASES; i++) {
		ErrorTable* table = &ctx->error_tables[i];
		if (table->phase == ctx->phase) {
			if (table->size >= table->capacity) {
				size_t prev_capacity = table->capacity;

				table->capacity *= 2;
				size_t new_capacity = table->capacity;
				void* new_errors = arena_reallocate(
					ctx->error_arena,
					table->errors,
					prev_capacity * sizeof(Error),
					new_capacity * sizeof(Error)
				);
				
				assert(new_errors);
				table->errors = new_errors;
			}
			table->errors[table->size++] = e;
		}
	}
}

ErrorTable* create_error_tables(CompilerContext* ctx) {
	ErrorTable* tables = arena_allocate(ctx->error_arena, NUM_PHASES * sizeof(ErrorTable));
	if (!tables) {
		perror("couldn't allocate space for tables\n");
		return NULL;
	}
	for (int i = 0; i < NUM_PHASES; i++) {
		tables[i].size = 0;
		tables[i].capacity = ERROR_CAPACITY;
		tables[i].phase = (phase_t)i;
		tables[i].errors = arena_allocate(ctx->error_arena, tables[i].capacity * sizeof(Error));
		if (!tables[i].errors) {
			return NULL;
		}
	}
	return tables;
}

bool phase_accumulated_errors(CompilerContext* ctx) {
	for (int i = 0; i < NUM_PHASES; i++) {
		ErrorTable* table = &ctx->error_tables[i];
		if (table && (table->phase == ctx->phase && table->size > 0)) return true;
	}
	return false;
}