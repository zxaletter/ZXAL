#include "parser.h"

static bool has_errors = false;
ErrorTable error_table;
NodeTable node_table;


Parser initialize_parser(Token* tokens) {
	Parser parser = {
		.size = 0,
		.capacity = 0,
		.end = tokens
	};
	return parser;
}

void init_error_table(CompilerContext* ctx) {
	error_table = create_error_table(ctx);
	if (error_table.errors == NULL) {
		printf("In 'init_error_table', errors are NULL.\n");
		// free(node_table.nodes);
		exit(EXIT_FAILURE);
	}
}

ErrorTable create_error_table(CompilerContext* ctx) {
	// Error** errors = calloc(ERROR_CAPACITY, sizeof(Error*));
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

void add_error_to_error_table(CompilerContext* ctx, Error* err) {
	if (!err) return;

	if (error_table.size >= error_table.capacity) {
		size_t prev_capacity = error_table.capacity;
		error_table.capacity *= 2;
		size_t new_capacity = error_table.capacity;
		// error_table.errors = realloc(error_table.errors, error_table.capacity);
		void* new_errors = arena_reallocate(ctx->error_arena, error_table.errors, prev_capacity, new_capacity);
		if (!new_errors) {
			emit_errors(ctx);
			exit(EXIT_FAILURE);

		}
		error_table.errors = new_errors;
	}
	error_table.errors[error_table.error_index++] = err;
	error_table.size++;
}

Token* copy_token(CompilerContext* ctx, Token* original_token) {
	// Token* copy_token = malloc(sizeof(Token));
	Token* copy_token = arena_allocate(ctx->lexer_arena, sizeof(Token));
	if (!copy_token) {
		perror("In 'copy_token', unable to allocate space for copy token\n");
		return NULL;
	}

	copy_token->type = original_token->type;
	copy_token->line = original_token->line;
	copy_token->column = original_token->column;

	if (original_token->type == TOKEN_ARROW ||
		original_token->type == TOKEN_ADD_EQUAL ||
		original_token->type == TOKEN_SUB_EQUAL || 
		original_token->type == TOKEN_DIV_EQUAL ||
		original_token->type == TOKEN_MUL_EQUAL ||
		original_token->type == TOKEN_LESS_EQUAL ||
		original_token->type == TOKEN_GREATER_EQUAL ||
		original_token->type == TOKEN_NOT_EQUAL ||
		original_token->type == TOKEN_EQUAL ||
		original_token->type == TOKEN_INCREMENT ||
		original_token->type == TOKEN_DECREMENT ||
		original_token->type == TOKEN_INT_KEYWORD ||
		original_token->type == TOKEN_CHAR_KEYWORD ||
		original_token->type == TOKEN_BOOL_KEYWORD ||
		original_token->type == TOKEN_VOID_KEYWORD || 
		original_token->type == TOKEN_STRUCT_KEYWORD || 
		original_token->type == TOKEN_ENUM_KEYWORD ||
		original_token->type == TOKEN_FOR_KEYWORD ||
		original_token->type == TOKEN_WHILE_KEYWORD ||
		original_token->type == TOKEN_CONTINUE_KEYWORD ||
		original_token->type == TOKEN_BREAK_KEYWORD ||
		original_token->type == TOKEN_FUNCTION_KEYWORD ||
		original_token->type == TOKEN_LET_KEYWORD || 
		original_token->type == TOKEN_ID ||
		original_token->type == TOKEN_LOGICAL_OR ||
		original_token->type == TOKEN_LOGICAL_AND) {

		// copy_token->value.str = strdup(original_token->value.str);
		copy_token->value.str = arena_allocate(ctx->lexer_arena, strlen(original_token->value.str) + 1);
		if (!copy_token->value.str) {
			perror("In 'copy_token', unable to copy original token string\n");
			// free(copy_token);
			return NULL;
		}
		strcpy(copy_token->value.str, original_token->value.str);
	} else if (original_token->type == TOKEN_INTEGER) {
		copy_token->value.val = original_token->value.val;
	} else {
		copy_token->value.c = original_token->value.c;
	}

	return copy_token;
}

void create_error(CompilerContext* ctx, error_t type, char* message, Token* token, FileInfo* info) {
	// Error* error = malloc(sizeof(Error));
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
	
	// char* message = malloc(1024);
	char* message = arena_allocate(ctx->error_arena, 1024);
	if (!message) return;

	snprintf(message, 1024, "\033[31mError\033[0m in file '%s' at line %d, column %d:\n", info->filename, tok->line, tok->column);
	create_error(ctx, type, message, tok, info);
	// free(message);	
}

token_t peek_token_type(Parser* parser) {
	Token tok = peek_token(parser);
	return tok.type;
}

Token peek_token(Parser* parser) {
	return *parser->end;
}

token_t peek_next_token_type(Parser* parser) {
	parser->end++;
	Token tok = *parser->end;
	parser->end--;
	return tok.type;
}

Token advance_parser(Parser* parser) {
	Token token = *parser->end;
	parser->end++; 
	return token;
}

bool at_token_eof(Parser* parser) {
	return (peek_token_type(parser) == TOKEN_EOF);
}

data_t get_type(Token* token) {
	switch (token->type) {
		case TOKEN_INT_KEYWORD: return TYPE_INTEGER;
		case TOKEN_CHAR_KEYWORD: return TYPE_CHAR;
		case TOKEN_BOOL_KEYWORD: return TYPE_BOOL;
		case TOKEN_STRUCT_KEYWORD: return TYPE_STRUCT;
		case TOKEN_VOID_KEYWORD: return TYPE_VOID;
		default: return TYPE_UNKNOWN;		
	}
}

Node* create_node(CompilerContext* ctx, node_t type, Node* left, Node* right, Node* prev, Node* next, struct type* t) {
	// printf("In 'create_node'\n");
	// Node* node = malloc(sizeof(Node));
	Node* node = arena_allocate(ctx->ast_arena, sizeof(Node));
	if (!node) {
		printf("In 'create_node', unable to allocate space for node\n");
		return NULL;
	}

	node->type = type;
	node->left = left;
	node->right = right;
	node->prev = prev;
	node->next = next;
	node->t = NULL;
	node->freed = false;
	node->symbol = NULL;
	if (t) {
		node->t = t;
		if (!node->t) {
			// free(node);
			return NULL;
		}		 
	}
	return node;
}

Node* create_int_node(CompilerContext* ctx ,node_t type, int val, Node* left, Node* right, Node* prev, Node* next, struct type* t) {
	Node* node = create_node(ctx, type, left, right, prev, next, t);
	if (!node) {
		printf("In 'create_int_node', receieved NULL node from 'create_node'.\n");
		return NULL;
	}
	node->value.val = val;
	return node;
}

Node* create_char_node(CompilerContext* ctx, node_t type, char ch, Node* left, Node* right, Node* prev, Node* next, struct type* t) {
	Node* node = create_node(ctx, type, left, right, prev, next, t);
	if (!node) {
		printf("In 'create_char_node', received NULL node from 'create_node'\n");
		return NULL;
	}
	node->value.c = ch;
	return node;
}

Node* create_string_node(CompilerContext* ctx, node_t type, char* id, Node* left, Node* right, Node* prev, Node* next, struct type* t) {
	printf("In 'create_string_node' with name '%s'\n", id ? id : "N/A");
	Node* node = create_node(ctx, type, left, right, prev, next, t);
	if (!node) {
		printf("In 'create_string_node', received NULL node from 'create_node'.\n");
		return NULL;
	} 
	node->value.name = NULL;

	if (id) {
		// node->value.name = strdup(id);
		node->value.name = arena_allocate(ctx->ast_arena, strlen(id) + 1);
		if (!node->value.name) {
			// free_node(node);
			return NULL;
		}
		strcpy(node->value.name, id);
	}
	return node;
}

node_t get_op_kind(Token* token) {
	switch (token->type) {
		case TOKEN_ADD: return NODE_ADD;
		case TOKEN_SUB: return NODE_SUB;
		case TOKEN_MUL: return NODE_MUL;
		case TOKEN_DIV: return NODE_DIV;
		case TOKEN_MODULO: return NODE_MODULO;
		case TOKEN_ADD_EQUAL: return NODE_ADD_EQUAL;
		case TOKEN_SUB_EQUAL: return NODE_SUB_EQUAL;
		case TOKEN_MUL_EQUAL: return NODE_MUL_EQUAL;
		case TOKEN_DIV_EQUAL: return NODE_DIV_EQUAL;
		case TOKEN_LESS: return NODE_LESS;
		case TOKEN_GREATER: return NODE_GREATER;
		case TOKEN_LESS_EQUAL: return NODE_LESS_EQUAL;
		case TOKEN_GREATER_EQUAL: return NODE_GREATER_EQUAL;
		case TOKEN_NOT: return NODE_NOT;
		case TOKEN_EQUAL: return NODE_EQUAL;
		case TOKEN_NOT_EQUAL: return NODE_NOT_EQUAL;
		case TOKEN_INCREMENT: return NODE_INCREMENT;
		case TOKEN_DECREMENT: return NODE_DECREMENT;
		case TOKEN_LOGICAL_AND: return NODE_LOGICAL_AND;
		case TOKEN_LOGICAL_OR: return NODE_LOGICAL_OR;
		default: return NODE_UNKNOWN;
	}
}

Node* parse_factor(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* node = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_ID: {

			char* id = NULL;
			{
				Token tok = peek_token(parser);
				// id = strdup(tok.value.str);
				id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_factor()'\n");
					return NULL;
				}
				strcpy(id, tok.value.str);
			}
			printf("\033[31mGot id '%s'\033[0m\n", id);
			Node* identifier_node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
			if (!identifier_node) {
				printf("Unable to create node with name '%s' in 'parse_factor'.\n", id);
				// free(id);
				return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(ctx, parser, info);
				// struct type* t = type_create(ctx, TYPE_FUNCTION, NULL, NULL);
				// if (!t) {
				// 	printf("In 'parse_factor', unable to create TYPE_FUNCTION.\n");
				// 	// if (args) free_node(args);
				// 	// free_node(identifier_node);
				// 	// free(id);
				// 	return NULL;
				// }

				Node* call_node = create_node(ctx, NODE_CALL, identifier_node, args, NULL, NULL, NULL);
				if (!call_node) {
					printf("In parse_factor, unable to create node with type NODE_CALL.\n");
					// free_node(identifier_node);
					// if (args) free_node(args);
					// free_type(t);
					// free(id);
					return NULL;
				}

				node = call_node;
				
				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					// printf("Current token type is %d\n", peek_token_type(parser));
					log_error(ctx, &tok, info, EXPECTED_RIGHT_PARENTHESES);
				}
				// free(t);
				advance_parser(parser);
				break;
			
			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(ctx, parser, info);
				if (!expr_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL expression node.\n");
					// free_node(identifier_node);
					// free(id);
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_RIGHT_BRACKET);
				}

				Node* subscript_node = create_node(ctx, NODE_SUBSCRIPT, identifier_node, expr_node, NULL, NULL, NULL);
				if (!subscript_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL subscript node.\n");
					// free_node(expr_node);
					// free(identifier_node);
					// free(id);
					return NULL;
				}

				node = subscript_node;

				advance_parser(parser);
				break;

			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				Node* increment_node = create_node(ctx, NODE_INCREMENT, identifier_node, NULL, NULL, NULL, NULL);
				if (!increment_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL increment node.\n");
					// free_node(identifier_node);
					// free(id);
					return NULL;
				}

				node = increment_node;
				advance_parser(parser);
				break;

			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				Node* decrement_node = create_node(ctx, NODE_DECREMENT, identifier_node, NULL, NULL, NULL, NULL);
				if (!decrement_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL decrement node.\n");
					// free_node(identifier_node);
					// free(id);
					return NULL;
				}

				node = decrement_node; 
				advance_parser(parser);
				break;
			}

			node = identifier_node;
			// free(id);
			break;
		}

		case TOKEN_INTEGER: {
			Token tok = peek_token(parser);
			int val = tok.value.val;
			struct type* t = type_create(ctx, TYPE_INTEGER, NULL, NULL);
			if (!t) {
				printf("In case 'TOKEN_INTEGER' in 'parse_factor', unable to create type with kind %d\n", TYPE_INTEGER);
				return NULL;
			}

			Node* integer_node = create_int_node(ctx, NODE_INTEGER, val, NULL, NULL, NULL, NULL, t);
			if (!integer_node) {
				printf("In case 'TOKEN_INTEGER' in 'parse_factor', received NULL integer node.\n");
				// free_type(t);
				return NULL;
			}

			node = integer_node;
			// free(t);
			advance_parser(parser);
			break;
		}

		case TOKEN_CHAR_LITERAL: {
			Token tok = peek_token(parser);
			char ch = tok.value.c;
			struct type* t = type_create(ctx, TYPE_CHAR, NULL, NULL);
			if (!t) {
				printf("In case 'TOKEN_CHAR_LITERAL' in 'parse_factor', unable to create type with kind %d\n", TYPE_CHAR);
				return NULL;
			}

			Node* character_node = create_char_node(ctx, NODE_CHAR, ch, NULL, NULL, NULL, NULL, t);
			if (!character_node) {
				printf("In case 'TOKEN_CHAR_LITERAL' in 'parse_factor', received NULL CHAR LITERAL node.\n");
				// free_type(t);
				return NULL;
			}
			
			node = character_node;
			// free(t);
			advance_parser(parser);
			break;
		}

		case TOKEN_TRUE_KEYWORD: {
			Node* bool_true_node = create_int_node(ctx, NODE_BOOL, 1, NULL, NULL, NULL, NULL, NULL);
			if (!bool_true_node) {
				printf("In case 'TOKEN_TRUE_KEYWORD' in 'parse_factor', received NULL BOOL node.\n");
				return NULL;
			}

			node = bool_true_node;
			advance_parser(parser);
			break;
		}

		case TOKEN_FALSE_KEYWORD: {
			Node* bool_false_node = create_int_node(ctx, NODE_BOOL, 0, NULL, NULL, NULL, NULL, NULL);
			if (!bool_false_node) {
				printf("In case 'TOKEN_FALSE_KEYWORD' in 'parse_factor', received NULL BOOL node.\n");
				return NULL;
			}

			node = bool_false_node;
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_PARENTHESES: {
			advance_parser(parser);
			node = parse_logical_or(ctx, parser, info);
			if (!node) {
				printf("In case 'TOKEN_LEFT_PARENTHESES' in 'parse_factor', received NULL node after invoking 'parse_logical_or'.\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_RIGHT_PARENTHESES);
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_BRACKET: {
			advance_parser(parser);
			node = parse_logical_or(ctx, parser, info);
			if (!node) {
				printf("In case 'TOKEN_LEFT_BRACKET' in 'parse_factor', received NULL node after invoking 'parse_logical_or'.\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_RIGHT_BRACKET);
			}
			advance_parser(parser);
			break;
		}
	}
	return node;
}

Node* parse_unary(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* unary_op_node = NULL;
	Node* head = NULL;
	Node* current = NULL;

	while (peek_token_type(parser) == TOKEN_NOT || peek_token_type(parser) == TOKEN_SUB || peek_token_type(parser) == TOKEN_ADD) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		unary_op_node = create_node(ctx, op_kind, NULL, NULL, NULL, NULL, NULL);
		if (unary_op_node) {
			if (!head) {
				head = unary_op_node;
				current = unary_op_node;
			} else {
				current->right = unary_op_node;
				unary_op_node->prev = current;
				current = unary_op_node;
			}
		} else {
			return NULL;
		} 
		advance_parser(parser);
	}

	if (!head) {
		Node* next_factor = parse_factor(ctx, parser, info);
		if (!next_factor) {
			return NULL;
		}
		return next_factor;
	}

	if (current) {
		current->right = parse_factor(ctx, parser, info);
		return head;
	}
}

Node* parse_multiplicative(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* node = parse_unary(ctx, parser, info);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_MUL || peek_token_type(parser) == TOKEN_DIV ||
		   peek_token_type(parser) == TOKEN_MUL_EQUAL || peek_token_type(parser) == TOKEN_DIV_EQUAL ||
		   peek_token_type(parser) == TOKEN_MODULO) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_unary(ctx, parser, info);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_additive(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* node = parse_multiplicative(ctx, parser, info);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_ADD || peek_token_type(parser) == TOKEN_SUB ||
		   peek_token_type(parser) == TOKEN_ADD_EQUAL || peek_token_type(parser) == TOKEN_SUB_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_multiplicative(ctx, parser, info);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_relational(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* node = parse_additive(ctx, parser, info);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LESS || peek_token_type(parser) == TOKEN_GREATER ||
		   peek_token_type(parser) == TOKEN_LESS_EQUAL || peek_token_type(parser) == TOKEN_GREATER_EQUAL ||
		   peek_token_type(parser) == TOKEN_EQUAL || peek_token_type(parser) == TOKEN_NOT_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_additive(ctx, parser, info);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_and(CompilerContext* ctx,Parser* parser, FileInfo* info) {
	Node* node = parse_relational(ctx, parser, info);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LOGICAL_AND) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_relational(ctx, parser, info);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_or(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* node = parse_logical_and(ctx,parser, info);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LOGICAL_OR) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_logical_and(ctx, parser, info);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_statement(CompilerContext* ctx,Parser* parser, FileInfo* info) {
	Node* stmt = NULL;
	bool special_statement = false;

	switch (peek_token_type(parser)) {
		case TOKEN_LET_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_ID) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
				return NULL;
			}

			char* id = NULL;
			{
				Token tok = peek_token(parser);
				// id = strdup(tok.value.str);
				id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_statement()\n");
					return NULL;
				}
				strcpy(id, tok.value.str);
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_COLON) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_COLON);
				// free(id);
				return NULL;
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_INT_KEYWORD &&
				peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
				peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
				peek_token_type(parser) != TOKEN_STR_KEYWORD &&
				peek_token_type(parser) != TOKEN_STRUCT_KEYWORD) {

				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_DATATYPE);	
				// free(id);
				return NULL;
			}

			Token tok = peek_token(parser);
			data_t type = get_type(&tok);
			struct type* t = type_create(ctx, type, NULL, NULL);
			if (!t) {
				// free(id);
				return NULL;
			}
			printf("Successfully made type with kind %d\n", t->kind);
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);

				struct type* array_type = type_create(ctx, TYPE_ARRAY, t, NULL);
				if (!array_type) {
					// free_type(t);
					// free(id);
					return NULL;
				}

				Node* expr_node = parse_logical_or(ctx, parser, info);
				if (!expr_node) {
					// free_type(array_type);
					// free(id);
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token token = peek_token(parser);
					log_error(ctx, &token, info, EXPECTED_RIGHT_BRACKET);
					// free_node(expr_node);
					// free_type(array_type);
					// free(id);
					return NULL;
				}

				advance_parser(parser);
				if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					Node* decl = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
					if (!decl) {
						// free_node(expr_node);
						// free_type(array_type);
						// free(id);
						return NULL;
					}
					stmt = create_node(ctx, NODE_DECL, NULL, NULL, NULL, NULL, NULL);
					if (!stmt) {
						// free_node(decl);
						return NULL;
					}
				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					advance_parser(parser); // move past '='
					int element_count = 0;
					Node* elements = parse_array_list(ctx, parser, info, &element_count);

 					Node* array_list = create_node(ctx, NODE_ARRAY_LIST, NULL, elements, NULL, NULL, NULL);
					Node* count = create_int_node(ctx, NODE_INTEGER, element_count, NULL, NULL, NULL, NULL, NULL);
					Node* assignee = create_string_node(ctx, NODE_NAME, id, expr_node, count, NULL, NULL, array_type);
											
					Node* def = create_node(ctx, NODE_DEF, assignee, NULL, NULL, NULL, NULL);
					stmt = create_node(ctx, NODE_ASSIGNMENT, def, array_list, NULL, NULL, NULL);  
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						Token token = peek_token(parser);
						log_error(ctx, &token, info, EXPECTED_SEMICOLON);
						// return NULL;
					}
				}

			} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				Node* decl = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				stmt = create_node(ctx, NODE_DECL, decl, NULL, NULL, NULL, NULL);
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				printf("in assignment case.\n");
				Node* assignee = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				if (!assignee) {
					// free_type(t);
					// free(id);
					return NULL;
				}
				printf("before def\n");
				Node* def = create_node(ctx, NODE_DEF, assignee, NULL, NULL, NULL, NULL);
				if (!def) {
					// free_node(assignee);
					// free_type(t);
					// free(id);
					return NULL;
				}

				advance_parser(parser);
				Node* expr_node = parse_logical_or(ctx, parser, info);
				if (!expr_node) {
					// free_node(def);
					// free(id);
					return NULL;
				} 

				stmt = create_node(ctx, NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);
				if (!stmt) {
					// free_node(expr_node);
					// free_node(def);
					// free_type(t);
					// free(id);
					return NULL;
				}
				printf("leaving assignment case.\n");
			}

			// free(id);			
			printf("HELLO\n");
			break;

		}
		
		case TOKEN_RETURN_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(ctx, NODE_RETURN, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {				
				Node* node = parse_logical_or(ctx, parser, info);
				if (node) {
					printf("got node in return\n");
				} else {
					printf("Didnt get node in return\n");
				}
				stmt = create_node(ctx, NODE_RETURN, NULL, node, NULL, NULL, NULL);
			}
			break;
		}

		case TOKEN_CONTINUE_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
				
			}

			stmt = create_node(ctx, NODE_CONTINUE, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case TOKEN_BREAK_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
				
			}
			stmt = create_node(ctx, NODE_BREAK, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case TOKEN_IF_KEYWORD: {
			special_statement = true;
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			} 

			Node* condition_node = parse_logical_or(ctx, parser, info);
			if (!condition_node) {
				printf("Error: received null condition node in if statement\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}

			advance_parser(parser);
			Node* if_body = parse_block(ctx, parser, info);
			if (!if_body) {
				printf("Error: received NULL if body\n");
				return NULL;
			}

			stmt = create_node(ctx, NODE_IF, condition_node, if_body, NULL, NULL, NULL);
			if (!stmt) {
				printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
				return NULL;
			}
			return stmt;
		} 

		case TOKEN_ELSE_KEYWORD: {
			special_statement = true;
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_IF_KEYWORD) {
				advance_parser(parser);
				
				if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_LEFT_PARENTHESES);
					// return NULL;
				}			

				Node* condition_node = parse_logical_or(ctx, parser, info);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
					// return NULL;
				}		

				advance_parser(parser);
				
				Node* body = parse_block(ctx, parser, info);
				stmt = create_node(ctx, NODE_ELSE_IF, condition_node, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
					return NULL;
				}
				return stmt;

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACE) {
				advance_parser(parser);

				Node* body = parse_block(ctx, parser, info);
				if (!body) {
					printf("Error: recived NULL body in else TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				} 

				stmt = create_node(ctx, NODE_ELSE, NULL, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Could not create statement NODE_ELSE type in TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				}
				return stmt;
			} else {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}
		}

		case TOKEN_FOR_KEYWORD: {
			special_statement = true;
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LET_KEYWORD) {
				Node* initializer = parse_statement(ctx, parser, info);
				if (!initializer) {
					printf("Error: Initializer is NULL is for loop\n");
					return NULL;
				}

				// printf("IN FOR CASE Current token type is %d\n", peek_token_type(parser));
				// advance_parser(parser); // skipping ';'
				Node* condition = parse_logical_or(ctx, parser, info);
				if (!condition) {
					printf("Error: Condition in for loop is NULL\n");
					return NULL;
				} else {
					printf("\033[31mcondition node has type %d\033[0m\n", condition->type);
					if (condition->left) {
						printf("\033[31mleft node of condition has type: %d\033[0m\n", condition->left->type);
					}
					if (condition->right) { printf("\033[31mRight node of condition has type: %d\033[0m\n", condition->right->type); }
				}

				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
					// return NULL;
				}

				advance_parser(parser);

				Node* update = parse_logical_or(ctx, parser, info);
				if (!update) {
					printf("Error: Update expression in for loop is NULL\n");
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_RIGHT_PARENTHESES);
					// return NULL;
				}

				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
					// return NULL;
				}

				advance_parser(parser);
				Node* loop_body = parse_block(ctx, parser, info);
				
				initializer->next = condition;
				condition->next = update;
				
				stmt = create_node(ctx, NODE_FOR, initializer, loop_body, NULL, NULL, NULL);
				return stmt;

			} else if (peek_token_type(parser) == TOKEN_ID) {
				if (peek_next_token_type(parser) == TOKEN_COLON) {
					char* id = NULL;
					{
						Token tok = peek_token(parser);
						id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
						// id = strdup(tok.value.str);
						if (!id) {
							printf("Error: Unable to allocate space for id\n");
							return NULL;
						}
						strcpy(id, tok.value.str); 
					}

					advance_parser(parser); 

					advance_parser(parser); // skip ':' since we already know it exists
					{
						if (peek_token_type(parser) != TOKEN_INT_KEYWORD) {
							Token tok = peek_token(parser);
							log_error(ctx, &tok, info, EXPECTED_INT_KEYWORD);
							// return NULL;
						}

						Token tok = peek_token(parser);
						data_t kind = get_type(&tok);
						struct type* type = type_create(ctx, kind, NULL, NULL);

						Node* var = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, type); 
						Node* def = create_node(ctx, NODE_DEF, var, NULL, NULL, NULL, NULL);

						advance_parser(parser);

						if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
							Token tok = peek_token(parser);
							log_error(ctx, &tok, info, EXPECTED_ASSIGNMENT);
							// return NULL;
						}

						advance_parser(parser);
						
						Node* expr_node = parse_logical_or(ctx, parser, info);
						Node* assignment = create_node(ctx, NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);
						if (assignment) {
							printf("Made assignment node\n");
						}

						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							Token tok = peek_token(parser);
							log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
							// return NULL;
						}

						advance_parser(parser);
						
						Node* condition = parse_logical_or(ctx, parser, info);
						if (condition) {
							assignment->next = condition;
						}

						advance_parser(parser); // skip ';'
					
						Node* update = parse_logical_or(ctx, parser, info);
						printf("Update type is %d\n", update->type);
						if (update) {
							condition->next = update;
						}

						if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
							Token tok = peek_token(parser);
							log_error(ctx, &tok, info, EXPECTED_RIGHT_PARENTHESES);
							// return NULL;
						}

						advance_parser(parser);
						if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
							Token tok = peek_token(parser);
							log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
							// return NULL;
						}

						advance_parser(parser);
						Node* loop_body = parse_block(ctx, parser, info);
						if (!loop_body) {
							// free(id);
							return NULL;
						}

						stmt = create_node(ctx, NODE_FOR, assignment, loop_body, NULL, NULL, NULL);
					}
					// free(id);
					return stmt;

				} else {
					Node* initializer = parse_statement(ctx, parser, info);
					if (initializer) {
						printf("\033[1;31mAFTER initializer -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					advance_parser(parser); // ';'

					Node* condition = parse_logical_or(ctx, parser, info);
					if (condition) {
						printf("\033[1;31mAFTER condition -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						printf("Expected ';' after for loop condition\n");
						return NULL;
					}
					advance_parser(parser);
					Node* update = parse_logical_or(ctx, parser, info);
					if (update) {
						printf("\033[1;31mAFTER update -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));

					}
					initializer->next = condition;
					condition->next = update;

					if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
						printf("Error: Expected ')' to complete for loop expression\n");
						return NULL;
					}
					advance_parser(parser);
					printf("AFTER advancing -> Current token type is: '%d'\n", peek_token_type(parser));

					if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
						printf("\033[1;31mError: Expected '{' before for loop body\033[0m\n");
						return NULL;
					}
					advance_parser(parser);
					Node* loop_body = parse_block(ctx, parser, info);
					stmt = create_node(ctx, NODE_FOR, initializer, loop_body, NULL, NULL, NULL);
					return stmt;

				}
			}
		}

		case TOKEN_WHILE_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			}

			Node* condition_node = parse_logical_or(ctx, parser, info);
			if (!condition_node) {
				printf("Error: Received null condition in while \n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}

			advance_parser(parser);
			Node* while_body = parse_block(ctx, parser, info);

			stmt = create_node(ctx, NODE_WHILE, condition_node, while_body, NULL, NULL, NULL);
			return stmt;
		}

		// case TOKEN_STRUCT_KEYWORD: {
		// 	printf("in token struct keyword case\n");
		// 	advance_parser(parser);

		// 	if (peek_token_type(parser) != TOKEN_ID) {
		// 		Token tok = peek_token(parser);
		// 		log_error(&tok, info, EXPECTED_IDENTIFIER);
		// 	}

		// 	char* id = NULL;
		// 	{
		// 		Token tok = peek_token(parser);
		// 		id = strdup(tok.value.str);
		// 		if (!id) {
		// 			printf("Unable to duplicate struct identifier\n");
		// 			return NULL;
		// 		} 
		// 	}

		// 	advance_parser(parser);
		// 	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		// 		Token tok = peek_token(parser);
		// 		log_error(&tok, info, EXPECTED_LEFT_BRACE);
		// 	}

		// 	advance_parser(parser);
		// 	Node* struct_body = parse_block(parser, info);
		// 	if (!struct_body) {
		// 		free(id);
		// 		return NULL;
		// 	}

		// 	struct type* t = type_create(NODE_STRUCT_DEF, NULL, NULL);
		// 	if (!t) {
		// 		free_node(struct_body);
		// 		free(id);
		// 		return NULL;
		// 	}
		// 	stmt = create_string_node(NODE_STRUCT_DEF, id, NULL, struct_body, NULL, NULL, t);
		// 	free(id);

		// 	return stmt;
		// }

		case TOKEN_ID: {
			char* id = NULL;
			{
				Token tok = peek_token(parser);
				id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
				// id = strdup(tok.value.str);
				if (!id) {
					printf("Error: Unable to aalocate space for id\n");
					return NULL;
				}
				strcpy(id, tok.value.str);
			}

			Node* node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				Node* index_node = parse_logical_or(ctx, parser, info);
				if (index_node) {
					printf("got index node\n");
				}
				Node* subscript_node = create_node(ctx, NODE_SUBSCRIPT, node, index_node, NULL, NULL, NULL);
				if (node) {
					printf("created subscript node\n");
				}

				if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_ASSIGNMENT);
					printf("Currently not at assignment node\n");
					// return NULL;
				}

				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(ctx, parser, info);
				Node* aug_expr_node = create_node(ctx, NODE_AUG, expr_node, NULL, NULL, NULL, NULL);
				Node* aug = create_node(ctx, NODE_AUG, subscript_node, NULL, NULL, NULL, NULL);
				stmt = create_node(ctx, NODE_ASSIGNMENT, aug, aug_expr_node, NULL, NULL, NULL);

			} else if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(ctx, parser, info);
				stmt =  create_node(ctx, NODE_CALL, node, args, NULL, NULL, NULL);
				advance_parser(parser);
			
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* expr = parse_logical_or(ctx, parser, info);
				if (!expr) {
					printf("Received null expr\n");
					// free(id);
					return NULL;
				}
				Node* def = create_node(ctx, NODE_AUG, node, NULL, NULL, NULL, NULL);
				stmt = create_node(ctx, NODE_ASSIGNMENT, def, expr, NULL, NULL, NULL);
			} else if (peek_token_type(parser) == TOKEN_COLON) {
				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
					peek_token_type(parser) != TOKEN_CHAR_KEYWORD) {

					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_DATATYPE);
					// free(id);
					// return NULL;
				}

				Token token = peek_token(parser);
				data_t type = get_type(&token);
				struct type* t = type_create(ctx, type, NULL, NULL);

				advance_parser(parser);
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
					// free(id);
					// return NULL;
				}
				Node* decl = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				stmt = create_node(ctx, NODE_DECL, decl, NULL, NULL, NULL, NULL);
			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				stmt = create_node(ctx, NODE_INCREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				stmt = create_node(ctx, NODE_DECREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			}
			// free(id);
			break;
		}
	}

	if (!special_statement) {
		if (peek_token_type(parser) != TOKEN_SEMICOLON) {

			Token tok = peek_token(parser);
			// printf("Current token type is: '%d'\n", tok.type);
			log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
			// return NULL;
		}	
		advance_parser(parser);	
	}
	return stmt;
}

Node* parse_block(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* block = create_node(ctx, NODE_BLOCK, NULL, NULL, NULL, NULL, NULL);
	
	Node* head = NULL;
	Node* current = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		Node* stmt = parse_statement(ctx, parser, info);

		if (stmt) {
			if (!head) {
				head = stmt;
				current = stmt;
			} else {
				current->next = stmt;
				stmt->prev = current;
				current = stmt;
			}

			if (current->type == NODE_ELSE_IF || current->type == NODE_ELSE) {
				if (!current->prev || (current->prev && (current->prev->type != NODE_IF && current->prev->type != NODE_ELSE_IF))) {
					printf("Error: else without matching if\n");
					return NULL;
				}
			}
		} 

	
		if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) { break; }
	}

	if (current) {
		current->next = NULL;
	}

	if (block) {
		block->right = head;
	}
	advance_parser(parser);
	printf("About to return from 'parse_block'.\n");
	return block;
}

Node* parse_parameters(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;
	Node* wrapped_param = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
		} 

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
			// id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for parameter id\n");
				return NULL;
			} 		
			strcpy(id, tok.value.str);
		}
		
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_COLON) {
			Token tok = peek_token(parser);
			log_error(ctx, &tok, info, EXPECTED_COLON);
		}

		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {

			Token tok = peek_token(parser);
			log_error(ctx, &tok, info, EXPECTED_DATATYPE);
			// return NULL;
		}

		Token tok = peek_token(parser);
		data_t param_type = get_type(&tok);
		struct type* t = type_create(ctx, param_type, NULL, NULL);
		if (!t) {
			printf("Unable to create type in 'parse_parameters\n");
			// free(id);
			return NULL;
		}

		advance_parser(parser);

		node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
		if (!node) {
			printf("In 'parse_parameters' unable to create node with name '%s'.\n", id);
			// free_node(head);
			// free_type(t);
			// free(id);
			return NULL;
		}
		
		wrapped_param = create_node(ctx, NODE_PARAM, NULL, node, NULL, NULL, NULL);
		if (!wrapped_param) {
			printf("Unable to create wrapped param node.\n");
			// free_node(head);
			// free_node(node);
			// free_type(t);
			// free(id);
			return NULL;
		}

		// free(id);
		
		if (wrapped_param) {
			if (!head) {
				head = wrapped_param;
				current = wrapped_param;
			} else {
				current->next = wrapped_param;
				wrapped_param->prev = current;
				current = wrapped_param;
			}
		}

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		} else if (peek_token_type(parser) == TOKEN_RIGHT_PARENTHESES) {
			break;
		} else {
			printf("Error: missing comma in parameters\n");
			return NULL;
		}
	}

	if (wrapped_param) {
		wrapped_param->next = NULL;
	}

	return head;
}

bool valid_function_return_type(token_t type) {
	switch (type) {
		case TOKEN_INT_KEYWORD: return true;
		case TOKEN_CHAR_KEYWORD: return true;
		case TOKEN_BOOL_KEYWORD: return true;
		case TOKEN_STRUCT_KEYWORD: return true;
		case TOKEN_STR_KEYWORD: return true;
		case TOKEN_VOID_KEYWORD: return true;
		default: return false;
	}
}

Node* parse_function(CompilerContext* ctx, Parser* parser, FileInfo* info) {	
	advance_parser(parser);
	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);

		if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
			int parentheses_count = 1;
			advance_parser(parser);
			while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
				if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
					parentheses_count++;
				} else if (peek_token_type(parser) == TOKEN_RIGHT_PARENTHESES) {
					parentheses_count--;
				}
				advance_parser(parser);
			}
			parentheses_count--;

			if (parentheses_count != 0) {
				printf("P count: %d\n", parentheses_count);
				printf("Mismatched parentheses\n");
				return NULL;
			}

			while (peek_token_type(parser) != TOKEN_LEFT_BRACE) { advance_parser(parser); }
			int brace_count = 1;
			advance_parser(parser);
			while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
				if (peek_token_type(parser) == TOKEN_LEFT_BRACE) {
					brace_count++;
				} else if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) {
					brace_count--;
				}
				advance_parser(parser);
			}
			brace_count--;

			if (brace_count != 0) {
				printf("Mismatched braces\n");
				return NULL;
			}
		}
		advance_parser(parser);
		return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		// id = strdup(tok.value.str);
		id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
		if (!id) {
			printf("In 'parse_function', error: unable to allocate space for function identifier '%s'.\n", tok.value.str);
			return NULL;
		}
		strcpy(id, tok.value.str);
	}
	printf("In function '%s'\n", id);
	advance_parser(parser);
	Node* params = NULL;
	if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_LEFT_PARENTHESES);	
		token_t synchronizations[1] = {TOKEN_ID};
		synchronize(parser, synchronizations, 1);
		params = parse_parameters(ctx, parser, info);
	} else {
		advance_parser(parser);
		params = parse_parameters(ctx, parser, info);
	}

	// add_node_to_node_table(params);	
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ARROW) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_ARROW);
		token_t synchronizations[5] = {TOKEN_INT_KEYWORD, TOKEN_CHAR_KEYWORD, TOKEN_BOOL_KEYWORD, TOKEN_STRUCT_KEYWORD, TOKEN_VOID_KEYWORD};
		synchronize(parser, synchronizations, 5);
	}
	
	advance_parser(parser);
	token_t type = peek_token_type(parser);
	struct type* function_subtype = NULL;
	struct type* function_maintype = NULL;

	if (!valid_function_return_type(type)) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_DATATYPE);
		token_t synchronizations[1] = {TOKEN_LEFT_BRACE};
		synchronize(parser, synchronizations, 1);
	} else {
		Token tok = peek_token(parser);
		data_t return_type = get_type(&tok);
		function_subtype = type_create(ctx, return_type, NULL, NULL);
		if (!function_subtype) {
			// free(id);
			return NULL;
		}

		function_maintype = type_create(ctx, TYPE_FUNCTION, function_subtype, params);
		if (!function_maintype) {
			// free(function_subtype);
			// free(id);
			return NULL;
		}
		// free(function_subtype);
		advance_parser(parser);
	} 

	// advance_parser(parser);
	Node* function_body = NULL;
	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token token = peek_token(parser);
		log_error(ctx, &token, info, EXPECTED_LEFT_BRACE);
		token_t synchronizations[7] = {TOKEN_LET_KEYWORD, TOKEN_ID, TOKEN_STRUCT_KEYWORD, TOKEN_ENUM_KEYWORD, TOKEN_FOR_KEYWORD, TOKEN_WHILE_KEYWORD, TOKEN_IF_KEYWORD};
		synchronize(parser, synchronizations, 7);
		function_body = parse_block(ctx, parser, info);
		if (!function_body) {
			// free_type(function_maintype);
			// free(function_subtype);
			// free(id);
			return NULL;
		}
		// add_node_to_node_table(function_body);
	} else {
		advance_parser(parser);
		function_body = parse_block(ctx, parser, info);
		if (!function_body) {
			// free_type(function_maintype);
			// free(function_subtype);			
			// free(id);
			return NULL;
		} 
		// add_node_to_node_table(function_body);
	}
	printf("Got here\n");
	printf("Got here after\n");
	Node* function_node = create_string_node(ctx, NODE_NAME, id, NULL, function_body, NULL, NULL, function_maintype);
	if (!function_node) {
		printf("Unable to create function node.\n");
		// free_type(function_maintype);
		// free_type(function_subtype);
		// free(id);
		return NULL;
	}
	// add_node_to_node_table(function_node);
	// free_type(function_maintype);
	// free_type(function_subtype);
	// free(id);

	printf("got all the way here\n");
	return function_node;
}

Node* parse_args(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current = NULL;
	Node* arg = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		arg = parse_logical_or(ctx, parser, info);
		if (!arg) {
			printf("Received NULL arg in 'parse_args()'\n");
			return NULL;
		}

		Node* wrapped_arg = create_node(ctx, NODE_ARG, NULL, arg, NULL, NULL, NULL);

		if (wrapped_arg) {
			if (!head) {
				head = wrapped_arg;
				current = wrapped_arg;
			} else {
				current->next = wrapped_arg;
				wrapped_arg->prev = current;
				current = wrapped_arg;
			}
		} else {
			printf("Error: Wrapped arg is NULL\n");
		}

		if (peek_token_type(parser) == TOKEN_COMMA) {
			advance_parser(parser);
		} else if (peek_token_type(parser) == TOKEN_RIGHT_PARENTHESES) {
			break;
		}
	}

	return head;
}

Node* parse_array_list(CompilerContext* ctx, Parser* parser, FileInfo* info, int* element_count) {

	Node* head = NULL;
	Node* current = NULL;
	Node* array_element = NULL;

	advance_parser(parser);	

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		array_element = parse_logical_or(ctx, parser, info);

		if (array_element) {
			if (!head) {
				head = array_element;
				current = array_element;
			} else {
				current->next = array_element;
				array_element->prev = current;
				current = array_element;
			}
		} else {
			printf("Error: In 'parse_array_list()' -> received NULL arg");
		}

		(*element_count)++;

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		} else if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) {
			break;
		} else {
			printf("MISSING VALUABLE character to define array\n");
			return NULL;
		}
	}
	printf("Element Count: %d\n", *element_count);

	if (current) {
		current->right = NULL;
	}

	advance_parser(parser);
	return head;
}

Node* parse_let(CompilerContext* ctx,Parser* parser, FileInfo* info) {
	Node* let_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		// id = strdup(tok.value.str);
		id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
		if (!id) {
			printf("Error: Unable to allocate space for id in 'parse_let()'\n");
			return NULL;
		}
		strcpy(id, tok.value.str);
	}

	advance_parser(parser);
	
	if (peek_token_type(parser) != TOKEN_COLON) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_COLON);
		// return NULL;
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
			
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_DATATYPE);
		// return NULL;
	}


	{
		Token tok = peek_token(parser);
		data_t type = get_type(&tok);
		struct type* t = type_create(ctx, type, NULL, NULL);
			
		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
			advance_parser(parser);
			struct type* array_type = type_create(ctx, TYPE_ARRAY, t, NULL);
			printf(" JUST MOVED PAST TOKEN_LEFT_BRACKET and Current token type is '%d'\n", peek_token_type(parser));

			Node* expr_node = parse_logical_or(ctx, parser, info);
			if (!expr_node) {
				printf("Error: Unable to retrieve size of array\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token token = peek_token(parser);
				log_error(ctx, &token, info, EXPECTED_RIGHT_BRACKET);
				// return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				let_node = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser); 
				int element_count = 0;
				
				Node* assignee = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
				Node* elements = parse_array_list(ctx, parser, info, &element_count);

				let_node = create_string_node(ctx, NODE_ASSIGNMENT, NULL, assignee, elements, NULL, NULL, NULL);   
				
				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token token = peek_token(parser);
					log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
					// return NULL;
				}
				advance_parser(parser);
			} 
		} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
			let_node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
			advance_parser(parser);
		} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
			advance_parser(parser);

			Node* assignee = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, t);
			Node* expr_node = parse_logical_or(ctx, parser, info);
			
			if (!expr_node) {
				printf("Error: EXPR NODE IN 'parse_let' is null\n");
				return NULL;
			}
			
			let_node = create_string_node(ctx, NODE_ASSIGNMENT, NULL, assignee, expr_node, NULL, NULL, NULL);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token token = peek_token(parser);
				log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
				// return NULL;
			}
			advance_parser(parser);
		}
		// free(id);
	}
		
	return let_node;
}

Node* parse_enum_body(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current = NULL;
	Node* stmt = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
			// return NULL;
		}

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			// id = strdup(tok.value.str);
			id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
			if (!id) return NULL;
			strcpy(id, tok.value.str);
		}

		stmt = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
		if (stmt) {
			if (!head) {
				head = stmt;
				current = stmt;
			} else {
				current->next = stmt;
				stmt->prev = current;
				current = stmt;
			}

		} else {
			printf("Empty statement in enum'\n");
			return NULL;
		}

		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_COMMA) { advance_parser(parser); }

		if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) { break; }
	}

	advance_parser(parser); // go over '}'
	return head;
}

Node* parse_enum(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* enum_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		// id = strdup(tok.value.str);
		id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
		if (!id) return NULL;
		strcpy(id, tok.value.str);
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
		// free(id);
		// return NULL;
	}

	advance_parser(parser);

	Node* enum_body = parse_enum_body(ctx, parser, info);
	if (!enum_body) {
		printf("Error: body for enum '%s' is NULL\n", id);
		// free(id);
		return NULL;
	}

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
		// free(id);
		// return NULL;
	}

	advance_parser(parser);

	enum_node = create_string_node(ctx, NODE_ENUM, id, NULL, enum_body, NULL, NULL, NULL);
	// free(id);
	return enum_node;
}

Node* parse_struct(CompilerContext* ctx, Parser* parser, FileInfo* info) {
	Node* struct_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		// id = strdup(tok.value.str);
		id = arena_allocate(ctx->ast_arena, strlen(tok.value.str) + 1);
		if (!id) return NULL;
		strcpy(id, tok.value.str);
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_LEFT_BRACE);
		// free(id);
		// return NULL;
	}

	advance_parser(parser);

	Node* struct_body = parse_block(ctx, parser, info);
	if (!struct_body) {
		printf("Error: body for Struct '%s' is NULL\n", id);
		// free(id);
		return NULL;
	}

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		log_error(ctx, &tok, info, EXPECTED_SEMICOLON);
		// free(id);
		// return NULL;
	}

	advance_parser(parser); // skip ';'
	struct type* t = type_create(ctx, TYPE_STRUCT, NULL, NULL);
	struct_node = create_string_node(ctx, NODE_STRUCT_DEF, id, NULL, struct_body, NULL, NULL, t);
	return struct_node;
}

void synchronize(Parser* parser, token_t* synchronizations, size_t length) {

	if (length > 0) {
		while (!at_token_eof(parser)) {
			token_t current_type = peek_token_type(parser);
			for (size_t i = 0; i < length; i++) {
				if (current_type == synchronizations[i]) {
					return;
				}
			}
			advance_parser(parser);
		}		
	}
}

Node* parse(CompilerContext* ctx, Token* tokens, FileInfo* info) {
	// init_node_table();
	init_error_table(ctx);
	
	Parser parser = initialize_parser(tokens);

	Node* head = NULL;
	Node* current = NULL;
	Node* node = NULL;

	while (!at_token_eof(&parser)) {

		if (at_token_eof(&parser)) { break; }

		token_t current_type = peek_token_type(&parser);

		if (current_type == TOKEN_FUNCTION_KEYWORD) {
			node = parse_function(ctx, &parser, info);
		}  else if (current_type == TOKEN_LET_KEYWORD) {
			node = parse_let(ctx, &parser, info);
		} else if (current_type == TOKEN_STRUCT_KEYWORD) {
			node = parse_struct(ctx, &parser, info);
		} else if (current_type == TOKEN_ENUM_KEYWORD) {
			node = parse_enum(ctx, &parser, info);
		} else {
			printf("Unexpected token type: %d\n", peek_token_type(&parser));
			advance_parser(&parser);
			continue;
		}

		if (node) {
			if (!head) {
				head = node;
				current = node;
			} else {
				current->next = node;
				node->prev = current;
				current = node;
			}
		} else {
			token_t synchronizations[4] = {TOKEN_FUNCTION_KEYWORD, TOKEN_LET_KEYWORD, TOKEN_STRUCT_KEYWORD, TOKEN_ENUM_KEYWORD};
			size_t length = sizeof(synchronizations) / sizeof(synchronizations[0]);
			synchronize(&parser, synchronizations, length);
		}
	}

	if (has_errors) {
		emit_errors(ctx);
	}

	return head;
}

// void free_type(struct type* t) {
// 	if (!t || (t && t->type_free)) return;
// 	t->type_free = true;
// 	if (t->subtype) free_type(t->subtype);
// 	if (t->params) free_node(t->params);
// 	free(t);
// }

// void free_node(Node* node) {
// 	if (!node || (node && node->freed)) return;
// 	node->freed = true;
// 	switch (node->type) {
// 		case NODE_IF:
// 		case NODE_ELSE_IF:
// 		case NODE_WHILE:
// 		case NODE_ASSIGNMENT:
// 		case NODE_ADD:
// 		case NODE_SUB:
// 		case NODE_MUL:
// 		case NODE_DIV:
// 		case NODE_ADD_EQUAL:
// 		case NODE_SUB_EQUAL:
// 		case NODE_MUL_EQUAL:
// 		case NODE_DIV_EQUAL:
// 		case NODE_MODULO:
// 		case NODE_LESS:
// 		case NODE_GREATER:
// 		case NODE_LESS_EQUAL:
// 		case NODE_GREATER_EQUAL:
// 		case NODE_EQUAL:
// 		case NODE_NOT_EQUAL:
// 		case NODE_LOGICAL_AND:
// 		case NODE_LOGICAL_OR: {
// 			if (node->left) free_node(node->left);
// 			if (node->right) free_node(node->right);
// 			break;
// 		}

// 		case NODE_INTEGER:
// 		case NODE_CHAR:
// 		case NODE_BOOL: {
// 			if (node->t) free_type(node->t);
// 			break;
// 		}

// 		case NODE_ELSE:
// 		case NODE_ARG:
// 		case NODE_NOT: {
// 			if (node->right) free_node(node->right);
// 			break;
// 		}

// 		case NODE_INCREMENT:
// 		case NODE_DECREMENT: {
// 			if (node->left) free_node(node->left);
// 			break;
// 		}

// 		case NODE_PARAM: {
// 			Node* current = node;
// 			while (current) {
// 				Node* next = current->next;
// 				free_node(current->right);
// 				current = current->next;
// 			}
// 			break;
// 		}

// 		case NODE_CALL: {
// 			if (node->left) free_node(node->left);

// 			Node* wrapped_arg = node->right;
// 			while (wrapped_arg) {
// 				free_node(wrapped_arg);
// 				wrapped_arg = wrapped_arg->next;
// 			}
// 			if (node->t) free_type(node->t);
// 			if (node->symbol) free_symbol(node->symbol);
// 			break;
// 		}
// 		// node array list
// 		case NODE_NAME: {
// 			if (node->value.name) free(node->value.name);
// 			if (node->left) free_node(node->left);
// 			if (node->right) free_node(node->right);
// 			if (node->t) free_type(node->t);
// 			if (node->symbol) free_symbol(node->symbol);
// 			break;
// 		}

// 		case NODE_SUBSCRIPT: {
// 			if (node->left) free_node(node->left);
// 			if (node->right) free_node(node->right);
// 			if (node->t) free_type(node->t);
// 			if (node->symbol) free_symbol(node->symbol);
// 			break;
// 		}

// 		case NODE_DEF:
// 		case NODE_DECL:
// 		case NODE_AUG: {
// 			if (node->left) free_node(node->left);
// 			break;
// 		}

// 		case NODE_BLOCK: {
// 			Node* stmt = node->right;
// 			while (stmt) {
// 				free_node(stmt);
// 				stmt = stmt->next;
// 			}
// 			break;
// 		}

// 		case NODE_FOR: {
// 			Node* initializer = node->left;
// 			Node* condition = initializer->next;
// 			Node* update = condition->next;
// 			Node* body = node->right;
// 			if (initializer) free_node(initializer);
// 			if (condition) free_node(condition);
// 			if (update) free_node(update);
// 			if (body) free_node(body);
// 			break;
// 		}

// 		case NODE_RETURN: {
// 			if (!node->right) break;
// 			free_node(node->right);
// 			break;
// 		}

// 		case NODE_BREAK:
// 		case NODE_CONTINUE: {
// 			break;
// 		}
// 	}

	
// 	free(node);
// }

// void free_globals(Node* node) {
// 	if (!node || (node && node->freed)) return;

// 	node->freed = true;
// 	switch (node->type) {
// 		case NODE_NAME: {
// 			if (node->t) {
// 				if (node->t->kind == TYPE_FUNCTION) {
// 					if (node->value.name) free(node->value.name);
// 					if (node->right) free_node(node->right);
// 					if (node->t) free_type(node->t);
					
// 				} else if (node->t->kind == TYPE_ARRAY) {
// 					if (node->value.name) {
// 						free(node->value.name);
// 					}

// 					if (node->left) {
// 						free_node(node->left);
// 					}

// 					if (node->right) {
// 						free_node(node->right);
// 						node->right = NULL;
// 					}

// 					if (node->t) {
// 						free_type(node->t);
// 						node->t = NULL;
// 					}
// 				}
// 			} 
// 			break;
// 		}

// 		case NODE_ASSIGNMENT: {
// 			if (node->value.name) {
// 				free(node->value.name);
// 				node->value.name = NULL;
// 			}

// 			if (node->left) {
// 				free_node(node->left);
// 				node->left = NULL;
// 			}

// 			if (node->right) {
// 				free_node(node->right);
// 				node->right = NULL;
// 			}
// 			break;
// 		}
// 	}
// 	free(node);
// }

// void free_ast(Node* root) {
//     printf("--- Starting AST Deallocation ---\n");

//     Node* current = root;
//     while (current) {
//         Node* next = current->next;
//         free_globals(current);
//         current = next;
//     }
//     printf("--- AST Deallocation Complete ---\n");

//     free_node_table();
// }