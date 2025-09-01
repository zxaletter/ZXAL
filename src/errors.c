#include "errors.h"
#include "compilercontext.h"


void create_error(CompilerContext* ctx, error_t type, char* message, Token* token, FileInfo* info) {
	Error* error = arena_allocate(ctx->error_arena, sizeof(Error));
	if (!error) {
		perror("Unable to allocate space for error\n");
		return;
	}

	error->type = type;
	error->token = copy_token(ctx, token);
	if (!error->token) {
		perror("In 'create_error', unable to copy token\n");
		// free(error);
		return;
	}
	error->info = info;
	// error->message = strdup(message);
	error->message = arena_allocate(ctx->error_arena, sizeof(message) + 1);
	if (!error->message) {
		perror("Unable to duplicate error message\n");
		// free_duplicate_token(error->token);
		// free(error);
		return;
	} 

	has_errors = true;
	add_error_to_error_table(ctx, error);
}

void display_error(CompilerContext* ctx, Error* error) {
	if (!error) return;

	printf("%s", error->message);

	int token_length = 1;

	switch (error->token->type) {
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
			if (error->token->value.str) {
				token_length = strlen(error->token->value.str);
			} 
			break;
		}

		case TOKEN_INTEGER: {
			int length = snprintf(NULL, 0, "%d", error->token->value.val);
			token_length = length;
			break;
		}
		default: token_length = 1; break;
	}

	int gutter_width = snprintf(NULL, 0, "%d", error->token->line);
	char space[gutter_width + 1];
	for (int i = 0; i < gutter_width; i++) {
		space[i] = ' ';
	}
	space[gutter_width] = '\0';

	switch (error->type) {
		case EXPECTED_IDENTIFIER: {
			printf("error: missing %s\n", get_token_string(TOKEN_ID));
			
			printf("%s%s-> %s:%d:%d\n", space, space, error->info->filename, error->token->line, error->token->column);
			
			printf("%s%s|\n", space, space);

			printf("%d%s| %s\n", error->token->line, space, error->info->lines[error->token->line - 1]);
			printf("%s%s|", space, space);
			char buffer[2 * gutter_width + 1];
			snprintf(buffer, sizeof(buffer), "%s%s|", space, space);

			for (int i = 1; i <= error->token->column - 1; i++) {
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
	for (int i = 0; i < error_table.size; i++) {
		display_error(ctx, error_table.errors[i]);
	}
}

void log_error(CompilerContext* ctx, Token* tok, FileInfo* info, error_t type) {
	
	char* message = arena_allocate(ctx->error_arena, 1024);
	if (!message) return;

	snprintf(message, 1024, "\033[31mError\033[0m in file '%s' at line %d, column %d:\n", info->filename, tok->line, tok->column);
	create_error(ctx, type, message, tok, info);
}

void add_error_to_error_table(CompilerContext* ctx, Error* err) {
	if (!err) return;

	if (error_table.size >= error_table.capacity) {
		size_t prev_capacity = error_table.capacity;
		error_table.capacity *= 2;
		size_t new_capacity = error_table.capacity;
		// error_table.errors = realloc(error_table.errors, error_table.capacity);
		void* new_errors = arena_reallocate(
			ctx->error_arena, 
			error_table.errors, 
			prev_capacity * sizeof(Error*), 
			new_capacity *  sizeof(Error*)
		);
		
		if (!new_errors) {
			emit_errors(ctx);
			// exit(EXIT_FAILURE);

		}
		error_table.errors = new_errors;
	}
	error_table.errors[error_table.error_index++] = err;
	error_table.size++;
}

ErrorTable create_error_table(CompilerContext* ctx) {
	Error** errors = arena_allocate(ctx->error_arena, sizeof(Error*) * ERROR_CAPACITY);
	if (!errors) {
		ErrorTable failed_error_table = {
			.size = 0,
			.capacity = 0,
			.error_index = 0,
			.errors = NULL
		};
		return failed_error_table;
	}

	ErrorTable new_error_table = {
		.size = 0,
		.capacity = ERROR_CAPACITY,
		.error_index = 0,
		.errors = errors
	};
	return new_error_table;
}

ErrorTables** create_error_tables(CompilerContext* ctx) {
	ErrorTable** tables = arena_allocate(ctx->error_arena, NUM_PHASES * sizeof(ErrorTable*));
	if (!tables) return NULL;

	for (int i = 0; i < NUM_PHASES; i++) {
		tables[i]->size = 0;
		tables[i]->capacity = 20;
		tables[i]->phase = (phase_t)i;
		tables[i]->errors = arena_allocate(ctx->error_arena, 20 * sizeof(Error));
		if (tables[i]->errors) return NULL;
	}
	return tables;
}