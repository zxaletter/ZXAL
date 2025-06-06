#include "parser.h"

static bool has_errors = false;
ErrorList* errors;

Parser* initialize_parser(Token* tokens) {
	Parser* parser = malloc(sizeof(Parser));
	if (!parser) {
		printf("Error: Unable to allocate space for parser\n");
		return NULL;
	}

	parser->size = 0;
	parser->capacity = NUM_NODES;
	parser->end = tokens;
	return parser;

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
		case TOKEN_SINGLE_QUOTE: return "'";
	}

}

ErrorList* create_error_list() {
	ErrorList* errors = malloc(sizeof(ErrorList));
	if (!errors) {
		perror("Unable to allocate space for error list\n");
		return NULL;
	}

	errors->size = 0;
	errors->current_error = 0;
	errors->capacity = ERROR_CAPACITY;
	errors->list = calloc(errors->capacity, sizeof(Error*));

	return errors;
}

void init_error_list() {
	errors = create_error_list();
	if (!errors) {
		perror("Failed to initialize global error list\n");
		exit(EXIT_FAILURE);
	}
}

bool add_error(Error* err) {
	if (!err) return false;

	if (errors->size >= errors->capacity) {
		errors->capacity *= 2;
		errors->list = realloc(errors->list, errors->capacity);
		if (!errors->list) {
			perror("Failed to reallocate errors list\n");
			return false;
		}
	}

	errors->list[errors->current_error++] = err;
	errors->size++;
	return true;
}

Token* copy_token(Token* original_token) {
	Token* copy_token = malloc(sizeof(Token));
	if (!copy_token) {
		perror("Unable to allocate space for copy token\n");
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

		copy_token->value.str = original_token->value.str;
		if (!copy_token->value.str) {
			perror("Unable to copy original token string\n");
			free(copy_token);
			return NULL;
		}
	} else if (original_token->type == TOKEN_INTEGER) {
		copy_token->value.val = original_token->value.val;
	} else {
		copy_token->value.c = original_token->value.c;
	}

	return copy_token;

}

void create_error(error_t type, char* message, Token* token, FileInfo* info) {
	Error* error = malloc(sizeof(Error));
	if (!error) {
		perror("Unable to allocate space for error\n");
		return;
	}

	error->type = type;
	error->token = copy_token(token);
	if (!error->token) {
		perror("Unable to copy token\n");
		free(error);
		return NULL;
	}
	error->info = info;
	error->message = strdup(message);
	if (!error->message) {
		perror("Unable to duplicate error message\n");
		free(error->token);
		free(error);
		return;
	} 

	has_errors = true;
	add_error(error);
	// if (add_error(error)) {
	// 	printf("Added error with line %d and column %d\n", error->token->line, error->token->column);
	// 	printf("With message: '%s'\n", error->message);
	// }
}

void display_error(Error* error) {
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
			} else {
				token_length = 1;
			}
			break;
		}

		case TOKEN_INTEGER: {
			int length = snprintf(NULL, 0, "%d", error->token->value.val);
			char* buffer = malloc(length + 1);
			if (!buffer) return;
			snprintf(buffer, length + 1, "%d", error->token->value.val);
			token_length = length;
			free(buffer);
			break;
		}
	}


	switch (error->type) {
		case EXPECTED_STRUCT_KEYWORD:
		case EXPECTED_IDENTIFIER: {

			printf("%s\n", error->info->lines[error->token->line - 1]);
			for (int i = 1; i < error->token->column - 1; i++) {
				printf(" ");
			}
			
			printf("\033[31m^\033[0m");
			
			for (int i = 0; i < token_length; i++) {
				printf("\033[31m~\033[0m");
			}
			printf("\n");
			printf("Missing identifier\n");

			break;
		}

		case EXPECTED_ARROW:
		case EXPECTED_COMMA:
		case EXPECTED_SEMICOLON:
		case EXPECTED_COLON:
		case EXPECTED_SINGLE_QUOTE:
		case EXPECTED_ASSIGNMENT:
		case EXPECTED_LEFT_BRACKET:
		case EXPECTED_RIGHT_BRACKET:
		case EXPECTED_LEFT_BRACE:
		case EXPECTED_RIGHT_BRACE:
		case EXPECTED_RIGHT_PARENTHESES:
		case EXPECTED_LEFT_PARENTHESES: {
			printf("%s\n", error->info->lines[error->token->line - 1]);

			for (int i = 0; i < error->token->column; i++) {
				printf(" ");
			}

			printf("\033[31m^\033[0m\n");
			break;
		}
	}
}

void emit_errors() {
	for (int i = 0; i < errors->size; i++) {
		Error* err = errors->list[i];
		display_error(err);
	}
}

void free_error(Error* error) {
	if (!error) return;

	if (error->message) {
		free(error->message);
	}

	if (error->token) {
		if (error->type == TOKEN_ARROW || 
			error->type == TOKEN_ADD_EQUAL ||
			error->type == TOKEN_SUB_EQUAL ||
			error->type == TOKEN_DIV_EQUAL ||
			error->type == TOKEN_MUL_EQUAL ||
			error->type == TOKEN_LESS_EQUAL ||
			error->type == TOKEN_GREATER_EQUAL ||
			error->type == TOKEN_NOT_EQUAL ||
			error->type == TOKEN_EQUAL ||
			error->type == TOKEN_INCREMENT ||
			error->type == TOKEN_DECREMENT ||
			error->type == TOKEN_INT_KEYWORD ||
			error->type == TOKEN_CHAR_KEYWORD ||
			error->type == TOKEN_BOOL_KEYWORD ||
			error->type == TOKEN_VOID_KEYWORD ||
			error->type == TOKEN_STRUCT_KEYWORD ||
			error->type == TOKEN_ENUM_KEYWORD ||
			error->type == TOKEN_FOR_KEYWORD ||
			error->type == TOKEN_WHILE_KEYWORD ||
			error->type == TOKEN_CONTINUE_KEYWORD ||
			error->type == TOKEN_BREAK_KEYWORD ||
			error->type == TOKEN_FUNCTION_KEYWORD ||
			error->type == TOKEN_LET_KEYWORD ||
			error->type == TOKEN_ID ||
			error->type == TOKEN_LOGICAL_OR ||
			error->type == TOKEN_LOGICAL_AND) {

			free(error->token->value.str);
		}	
		free(error->token);
	}

	free(error);
}

void free_error_list() {
	for (int i = 0; i < errors->size; i++) {
		Error* error = errors->list[i];
		free_error(error);
	}

	free(errors);
}

void report_error(Token* tok, FileInfo* info, error_t type) {
	
	char* message = malloc(1024);
	if (!message) return;

	snprintf(message, 1024, "\033[31mError\033[0m in file '%s' at line %d, column %d:\n", info->filename, tok->line, tok->column);
	create_error(type, message, tok, info);
	free(message);
	
}

token_t peek_token_type(Parser* parser) {
	Token tok = peek_token(parser);
	return tok.type;
}

Token peek_token(Parser* parser) {
	if (!parser->end) { printf("We have null\n"); }
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
	data_t kind;

	switch (token->type) {
		case TOKEN_INT_KEYWORD: kind = TYPE_INTEGER; break;
		case TOKEN_CHAR_KEYWORD: kind = TYPE_CHAR; break;
		default: {
			printf("Defaulting to unknown type\n");
			kind = TYPE_UNKNOWN;
			break;
		}
	}

	return kind;
}

struct type* create_type(data_t main_type, struct type* subtype) {
	struct type* t = malloc(sizeof(struct type));
	if (!t) {
		printf("Error: Unable to allocate space for type\n");
		return NULL;
	}
	t->kind = main_type;
	t->type_free = false;

	if (subtype) {
		t->subtype = subtype;
	} else {
		t->subtype = NULL;
	}

	return t;
}

Node* create_node(node_t type, Node* left, Node* right, 
	Node* prev, Node* next, struct type* t) {

	Node* node = malloc(sizeof(Node));
	if (!node) {
		printf("Error: Unable to allocate space for node\n");
		return NULL;
	}

	node->type = type;
	node->left = left;
	node->right = right;
	node->prev = prev;
	node->next = next;
	node->t = t;
	node->node_free = false;
	node->symbol = NULL;

	return node;
}

Node* create_int_node(node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {

	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		printf("Error: In 'create_int_node()': NULL '<-' from 'create_node()'\n");
		return NULL;
	}
	node->value.val = val;

	return node;
}

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {
	
	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		printf("Error: In 'create_string_node()': NULL '<-' from 'create_node()'\n");
		return NULL;
	}

	if (id) {
		node->value.name = strdup(id);
		if (!node->value.name) {
			printf("Error: Unable to allocate space for string in 'create_string_node()'\n");
			free(node);
			return NULL;
		}
	}

	return node;
}

node_t get_op_kind(Token* token) {
	node_t op = NODE_UNKNOWN;

	switch (token->type) {
		case TOKEN_ADD: op = NODE_ADD; return op;
		case TOKEN_SUB: op = NODE_SUB; return op;
		case TOKEN_MUL: op = NODE_MUL; return op;
		case TOKEN_DIV: op = NODE_DIV; return op;
		case TOKEN_MODULO: op = NODE_MODULO; return op;
		case TOKEN_ADD_EQUAL: op = NODE_ADD_EQUAL; return op;
		case TOKEN_SUB_EQUAL: op = NODE_SUB_EQUAL; return op;
		case TOKEN_MUL_EQUAL: op = NODE_MUL_EQUAL; return op;
		case TOKEN_DIV_EQUAL: op = NODE_DIV_EQUAL; return op;
		case TOKEN_LESS: op = NODE_LESS; return op;
		case TOKEN_GREATER: op = NODE_GREATER; return op;
		case TOKEN_LESS_EQUAL: op = NODE_LESS_EQUAL; return op;
		case TOKEN_GREATER_EQUAL: op = NODE_GREATER_EQUAL; return op;
		case TOKEN_NOT: op = NODE_NOT; return op;
		case TOKEN_EQUAL: op = NODE_EQUAL; return op;
		case TOKEN_NOT_EQUAL: op = NODE_NOT_EQUAL; return op;
		case TOKEN_INCREMENT: op = NODE_INCREMENT; return op;
		case TOKEN_DECREMENT: op = NODE_DECREMENT; return op;
		case TOKEN_LOGICAL_AND: op = NODE_LOGICAL_AND; return op;
		case TOKEN_LOGICAL_OR: op = NODE_LOGICAL_OR; return op;
	}
}

Node* parse_factor(Parser* parser, FileInfo* info) {
	Node* node = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_ID: {
			char* id = NULL;
			{
				Token tok = peek_token(parser);
				id = strdup(tok.value.str);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_factor()'\n");
					return NULL;
				}
			}

			node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(parser, info);
				node = create_node(NODE_CALL, node, args, NULL, NULL, NULL);
				
				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_RIGHT_PARENTHESES);
				}
				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(parser, info);
				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_RIGHT_BRACKET);
				}
				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				Node* increment_node = create_node(NODE_INCREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				Node* decrement_node = create_node(NODE_DECREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			}

			break;
		}

		case TOKEN_INTEGER: {
			Token tok = peek_token(parser);
			int val = tok.value.val;
			node = create_int_node(NODE_INTEGER, val, NULL, NULL, NULL, NULL, NULL);
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_PARENTHESES: {
			advance_parser(parser);
			node = parse_logical_or(parser, info);
			if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_RIGHT_PARENTHESES);
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_BRACKET: {
			advance_parser(parser);
			node = parse_logical_or(parser, info);
			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_RIGHT_BRACKET);
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_SINGLE_QUOTE: {
			advance_parser(parser);
			node = parse_logical_or(parser, info);
			if (peek_token_type(parser) != TOKEN_SINGLE_QUOTE) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_SINGLE_QUOTE);
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_DOUBLE_QUOTE: {
			advance_parser(parser);
			node = parse_logical_or(parser, info);
			if (peek_token_type(parser) != TOKEN_DOUBLE_QUOTE) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_DOUBLE_QUOTE);
			}
			advance_parser(parser);
			break;
		}
	}
	return node;
}

Node* parse_unary(Parser* parser, FileInfo* info) {
	Node* node = parse_factor(parser, info);

	while (peek_token_type(parser) == TOKEN_NOT) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_factor(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}
Node* parse_multiplicative(Parser* parser, FileInfo* info) {
	Node* node = parse_unary(parser, info);
	
	while (peek_token_type(parser) == TOKEN_MUL || peek_token_type(parser) == TOKEN_DIV ||
		   peek_token_type(parser) == TOKEN_MUL_EQUAL || peek_token_type(parser) == TOKEN_DIV_EQUAL ||
		   peek_token_type(parser) == TOKEN_MODULO) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_unary(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_additive(Parser* parser, FileInfo* info) {
	Node* node = parse_multiplicative(parser, info);

	while (peek_token_type(parser) == TOKEN_ADD || peek_token_type(parser) == TOKEN_SUB ||
		   peek_token_type(parser) == TOKEN_ADD_EQUAL || peek_token_type(parser) == TOKEN_SUB_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_multiplicative(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_relational(Parser* parser, FileInfo* info) {
	Node* node = parse_additive(parser, info);

	while (peek_token_type(parser) == TOKEN_LESS || peek_token_type(parser) == TOKEN_GREATER ||
		   peek_token_type(parser) == TOKEN_LESS_EQUAL || peek_token_type(parser) == TOKEN_GREATER_EQUAL ||
		   peek_token_type(parser) == TOKEN_EQUAL || peek_token_type(parser) == TOKEN_NOT_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_additive(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_logical_and(Parser* parser, FileInfo* info) {
	Node* node = parse_relational(parser, info);

	while (peek_token_type(parser) == TOKEN_LOGICAL_AND) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_relational(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_or(Parser* parser, FileInfo* info) {
	Node* node = parse_logical_and(parser, info);

	while (peek_token_type(parser) == TOKEN_LOGICAL_OR) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_logical_and(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_statement(Parser* parser, FileInfo* info) {
	Node* stmt = NULL;
	bool special_statement = false;

	switch (peek_token_type(parser)) {
		case TOKEN_LET_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_ID) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_IDENTIFIER);

				// return NULL;
			}

			char* id = NULL;
			{
				Token tok = peek_token(parser);
				id = strdup(tok.value.str);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_statement()\n");
					return NULL;
				}
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_COLON) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_COLON);
				// return NULL;
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_INT_KEYWORD &&
				peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
				peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
				
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_DATATYPE);			
				// return NULL;
			}

			{
				Token tok = peek_token(parser);
				data_t type = get_type(&tok);
				struct type* t = create_type(type, NULL);
				
				advance_parser(parser);
				
				if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
					advance_parser(parser);

					struct type* array_type = create_type(TYPE_ARRAY, t);
					Node* expr_node = parse_logical_or(parser, info);
					if (!expr_node) {
						printf("Error: Unable to retrieve size of array\n");
						return NULL;
					}

					if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
						Token token = peek_token(parser);
						report_error(&token, info, EXPECTED_RIGHT_BRACKET);
						// return NULL;
					}

					advance_parser(parser);

					if (peek_token_type(parser) == TOKEN_SEMICOLON) {
						Node* decl = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
						stmt = create_node(NODE_DECL, NULL, NULL, NULL, NULL, NULL);

					} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
						advance_parser(parser); // move past '='
						
						Node* assignee = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
						Node* def = create_node(NODE_DEF, assignee, NULL, NULL, NULL, NULL);
						Node* elements = parse_array_list(parser, info);

						stmt = create_node(NODE_ASSIGNMENT, def, elements, NULL, NULL, NULL);  

						advance_parser(parser);

						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							Token token = peek_token(parser);
							report_error(&token, info, EXPECTED_SEMICOLON);
							// return NULL;
						}
					} 
				
				} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					Node* decl = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
					stmt = create_node(NODE_DECL, decl, NULL, NULL, NULL, NULL);

				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					Node* assignee = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
					Node* def = create_node(NODE_DEF, assignee, NULL, NULL, NULL, NULL);
					advance_parser(parser);
					Node* expr_node = parse_logical_or(parser, info);
					if (!expr_node) {
						printf("Error: EXPR NODE IN 'parse_let' is null\n");
						return NULL;
					}
					stmt = create_node(NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						Token token = peek_token(parser);
						report_error(&token, info, EXPECTED_SEMICOLON);
						// return NULL;
					}
				}
			}
			free(id);
			break;
		}

		case TOKEN_RETURN_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(NODE_RETURN, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {				
				Node* node = parse_logical_or(parser, info);
				stmt = create_string_node(NODE_RETURN, NULL, NULL, node, NULL, NULL, NULL);
			}
			break;
		}

		case TOKEN_CONTINUE_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_SEMICOLON);
				return NULL;
			}

			stmt = create_node(NODE_CONTINUE, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case TOKEN_BREAK_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_SEMICOLON);
				return NULL;
			}
			stmt = create_node(NODE_BREAK, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case TOKEN_IF_KEYWORD: {
			special_statement = true;
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			} 

			Node* condition_node = parse_logical_or(parser, info);
			if (!condition_node) {
				printf("Error: received null condition node in if statement\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}

			advance_parser(parser);
			Node* if_body = parse_block(parser, info);
			if (!if_body) {
				printf("Error: received NULL if body\n");
				return NULL;
			}

			stmt = create_node(NODE_IF, condition_node, if_body, NULL, NULL, NULL);
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
					report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);
					// return NULL;
				}			

				Node* condition_node = parse_logical_or(parser, info);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_LEFT_BRACE);
					// return NULL;
				}		

				advance_parser(parser);
				
				Node* body = parse_block(parser, info);
				if (!body) {
					printf("Error: body is NULL\n");
					return NULL;
				}

				stmt = create_node(NODE_ELSE_IF, condition_node, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
					return NULL;
				}
				return stmt;

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACE) {
				special_statement = true;
				advance_parser(parser);

				Node* body = parse_block(parser, info);
				if (!body) {
					printf("Error: recived NULL body in else TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				} 

				stmt = create_node(NODE_ELSE, NULL, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Could not create statement NODE_ELSE type in TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				}
				return stmt;
			} else {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}
		}

		case TOKEN_FOR_KEYWORD: {
			special_statement = true;
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LET_KEYWORD) {
				Node* initializer = parse_statement(parser, info);
				if (!initializer) {
					printf("Error: Initializer is NULL is for loop\n");
					return NULL;
				}

				advance_parser(parser); // skipping ';'
				
				Node* condition = parse_logical_or(parser, info);
				if (!condition) {
					printf("Error: Condition in for loop is NULL\n");
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_SEMICOLON);
					// return NULL;
				}

				advance_parser(parser);

				Node* update = parse_logical_or(parser, info);
				if (!update) {
					printf("Error: Update expression in for loop is NULL\n");
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_RIGHT_PARENTHESES);
					// return NULL;
				}

				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_LEFT_BRACE);
					// return NULL;
				}

				advance_parser(parser);
				Node* loop_body = parse_block(parser, info);
				
				initializer->next = condition;
				condition->next = update;
				
				stmt = create_node(NODE_FOR, initializer, loop_body, NULL, NULL, NULL);
				return stmt;

			} else if (peek_token_type(parser) == TOKEN_ID) {
				if (peek_next_token_type(parser) == TOKEN_COLON) {
					char* id = NULL;
					{
						Token tok = peek_token(parser);
						id = strdup(tok.value.str);
						if (!id) {
							printf("Error: Unable to allocate space for id\n");
							return NULL;
						}
					}

					advance_parser(parser); 

					advance_parser(parser); // skip ':' since we already know it exists
					{
						if (peek_token_type(parser) != TOKEN_INT_KEYWORD) {
							Token tok = peek_token(parser);
							report_error(&tok, info, EXPECTED_INT_KEYWORD);
							// return NULL;
						}

						Token tok = peek_token(parser);
						data_t kind = get_type(&tok);
						struct type* type = create_type(kind, NULL);

						Node* var = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, type); 
						Node* def = create_node(NODE_DEF, NULL, NULL, NULL, NULL, NULL);
						if (def) {
							printf("Created string node\n");
						}

						advance_parser(parser);

						if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
							Token tok = peek_token(parser);
							report_error(&tok, info, EXPECTED_ASSIGNMENT);
							// return NULL;
						}

						advance_parser(parser);
						
						Node* expr_node = parse_logical_or(parser, info);
						Node* assignment = create_node(NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);
						if (assignment) {
							printf("Made assignment node\n");
						}

						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							Token tok = peek_token(parser);
							report_error(&tok, info, EXPECTED_SEMICOLON);
							// return NULL;
						}

						advance_parser(parser);
						
						Node* condition = parse_logical_or(parser, info);
						if (condition) {
							assignment->next = condition;
						}

						advance_parser(parser); // skip ';'
					
						Node* update = parse_logical_or(parser, info);
						if (update) {
							condition->next = update;
						}

						if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
							Token tok = peek_token(parser);
							report_error(&tok, info, EXPECTED_RIGHT_PARENTHESES);
							// return NULL;
						}

						advance_parser(parser);
						if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
							Token tok = peek_token(parser);
							report_error(&tok, info, EXPECTED_LEFT_BRACE);
							// return NULL;
						}

						advance_parser(parser);
						Node* loop_body = parse_block(parser, info);
						if (!loop_body) {
							free(id);
							return NULL;
						}

						stmt = create_node(NODE_FOR, assignment, loop_body, NULL, NULL, NULL);
					}
					free(id);
					return stmt;

				} else {
					Node* initializer = parse_statement(parser, info);
					if (initializer) {
						printf("\033[1;31mAFTER initializer -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					advance_parser(parser); // ';'

					Node* condition = parse_logical_or(parser, info);
					if (condition) {
						printf("\033[1;31mAFTER condition -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						printf("Expected ';' after for loop condition\n");
						return NULL;
					}
					advance_parser(parser);
					Node* update = parse_logical_or(parser, info);
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
					Node* loop_body = parse_block(parser, info);
					stmt = create_node(NODE_FOR, initializer, loop_body, NULL, NULL, NULL);
					return stmt;

				}
			}
		}

		case TOKEN_WHILE_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);
				// return NULL;
			}

			Node* condition_node = parse_logical_or(parser, info);
			if (!condition_node) {
				printf("Error: Received null condition in while \n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_BRACE);
				// return NULL;
			}

			advance_parser(parser);
			Node* while_body = parse_block(parser, info);

			stmt = create_node(NODE_WHILE, condition_node, while_body, NULL, NULL, NULL);
			return stmt;
		}

		case TOKEN_ID: {
			char* id = NULL;
			{
				Token tok = peek_token(parser);
				id = strdup(tok.value.str);
				if (!id) {
					printf("Error: Unable to aalocate space for id\n");
					return NULL;
				}
			}

			Node* node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				Node* index_node = parse_logical_or(parser, info);
				if (index_node) {
					printf("got index node\n");
				}

				node = create_node(NODE_SUBSCRIPT, node, index_node, NULL, NULL, NULL);
				if (node) {
					printf("created subscript node\n");
				}

				if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_ASSIGNMENT);
					// return NULL;
				}

				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(parser, info);
				Node* def = create_node(NODE_DEF, node, NULL, NULL, NULL, NULL);
				stmt = create_node(NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);

			} else if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(parser, info);
				stmt =  create_node(NODE_CALL, node, args, NULL, NULL, NULL);
				advance_parser(parser);
			
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* expr = parse_logical_or(parser, info);
				if (!expr) {
					printf("Received null expr\n");
					free(id);
					return NULL;
				}
				Node* def = create_node(NODE_DEF, node, NULL, NULL, NULL, NULL);
				stmt = create_node(NODE_ASSIGNMENT, def, expr, NULL, NULL, NULL);
			} else if (peek_token_type(parser) == TOKEN_COLON) {
				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
					peek_token_type(parser) != TOKEN_CHAR_KEYWORD) {

					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_DATATYPE);
					free(id);
					// return NULL;
				}

				Token token = peek_token(parser);
				data_t type = get_type(&token);
				struct type* t = create_type(type, NULL);

				advance_parser(parser);
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_SEMICOLON);
					free(id);
					// return NULL;
				}
				Node* decl = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				stmt = create_node(NODE_DECL, decl, NULL, NULL, NULL, NULL);
			}
			free(id);
			break;
		}
		
		// case TOKEN_SWITCH: {
		// 	special_statement = true;
		// 	advance_parser(parser);
			
		// 	if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
		// 		Token tok = peek_token(parser);
		// 		report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);
		// 		return NULL;
		// 	}

		// 	Node* condition = parse_logical_or(parser, info);

		// 	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		// 		Token tok = peek_token(parser);
		// 		report_error(&tok, info, EXPECTED_LEFT_BRACE);
		// 		return NULL;
		// 	}

		// 	advance_parser(parser);
		// 	Node* switch_body = parse_switch(parser, info);
		// }

		// case TOKEN_CASE: {
		// 	advance_parser(parser);
		// 	Node* node = NULL;
		// 	char* id = NULL;
		// 	if (peek_token_type(parser) == TOKEN_SINGLE_QUOTE) {
		// 		node = parse_logical_or(parer, info);
		// 		if (!node) return NULL;
		// 	} else if (peek_token_type(parser) == TOKEN_NAME) {
				
		// 		Token token = peek_token(parser);
		// 		id = strdup(token.value.str);
		// 		if (!id) {
		// 			perror("Unable to allocate space for id\n");
		// 			return NULL;
		// 		}
				
		// 		node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
		// 		if (!node) {
		// 			perror("Unable to create node in TOKEN_CASE\n");
		// 			free(id);
		// 			return NULL;
		// 		}
		// 		advance_parser(parser);
		// 	}

		// 	if (peek_token_type(parser) != TOKEN_COLON) {
		// 		Token tok = peek_token(parser);
		// 		report_error(&tok, info, EXPECTED_COLON);
		// 		free(id);
		// 		return NULL;
		// 	}

		// 	advance_parser(parser);
		// 	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		// 		Token tok = peek_token(parser);
		// 		report_error(&tok info, EXPECTED_LEFT_BRACE);
		// 		free(id);
		// 		return NULL;
		// 	} 

		// 	advance_parser(parser);
		// 	Node* case_body = parse_block(parser, info);

		// }
	}

	if (!special_statement) {
		if (peek_token_type(parser) != TOKEN_SEMICOLON) {
			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_SEMICOLON);
			// return NULL;
		}		
	}
	return stmt;
}

Node* parse_block(Parser* parser, FileInfo* info) {
	Node* block = create_node(NODE_BLOCK, NULL, NULL, NULL, NULL, NULL);

	Node* head = NULL;
	Node* current = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		Node* stmt = parse_statement(parser, info);

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

		if (peek_token_type(parser) == TOKEN_SEMICOLON) { advance_parser(parser); }
	
		if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) { break; }
	}

	if (block) {
		block->right = head;
	}

	advance_parser(parser); // go over '}'
	return block;
}

Node* parse_parameters(Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_IDENTIFIER);
			// return NULL;
		} 

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for parameter id\n");
				return NULL;
			}			
		}
		
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_COLON) {
			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_COLON);
			// return NULL;
		}

		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {

			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_DATATYPE);
			// return NULL;
		}

		Token tok = peek_token(parser);
		data_t param_type = get_type(&tok);
		struct type* t = create_type(param_type, NULL);
		advance_parser(parser);

		node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
		Node* wrapped_param = create_node(NODE_PARAM, NULL, node, NULL, NULL, NULL);
		free(id);

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

	return head;
}

Node* parse_function(Parser* parser, FileInfo* info) {
	Node* function_node = NULL;

	advance_parser(parser);
	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_IDENTIFIER);

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
		advance_parser(parser); // consume '}'
		return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		if (tok.value.str) {
			id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for function identifier\n");
				return NULL;
			}
		} else {
			printf("Token does not have string\n");
		}
	}

	advance_parser(parser);
	if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);	
		// return NULL;
	}

	advance_parser(parser);
	Node* params = parse_parameters(parser, info);
		
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ARROW) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_ARROW);
		// return NULL;
	}

	advance_parser(parser);
	if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
		peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
		peek_token_type(parser) != TOKEN_VOID_KEYWORD) {
			
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_DATATYPE);
		// return NULL;
	}

	Token tok = peek_token(parser);
	data_t return_type = get_type(&tok);
	struct type* subtype = create_type(return_type, NULL);
	struct type* t = create_type(TYPE_FUNCTION, subtype);
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token token = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_BRACE);
		// return NULL;
	}

	advance_parser(parser);
	Node* body = parse_block(parser, info);
	if (!body) {
		free(id);
		return NULL;
	}

	function_node = create_string_node(NODE_NAME, id, params, body, NULL, NULL, t);
	if (!function_node) {
		free(id);
		printf("Error: Unable to create function node\n");
		return NULL;
	}

	free(id);
	return function_node;
}

Node* parse_args(Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current = NULL;
	Node* arg = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		arg = parse_logical_or(parser, info);
		if (!arg) {
			printf("Received NULL arg in 'parse_args()'\n");
			return NULL;
		}

		Node* wrapped_arg = create_node(NODE_ARG, NULL, arg, NULL, NULL, NULL);

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

Node* parse_array_list(Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current = NULL;
	Node* array_element = NULL;

	advance_parser(parser);

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		array_element = parse_logical_or(parser, info);

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

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		} else if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) {
			break;
		} else {
			printf("MISSING VALUABLE character to define array\n");
			return NULL;
		}
	}
	return head;
}

Node* parse_let(Parser* parser, FileInfo* info) {
	Node* let_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		id = strdup(tok.value.str);
		if (!id) {
			printf("Error: Unable to allocate space for id in 'parse_let()'\n");
			return NULL;
		}
	}

	advance_parser(parser);
	
	if (peek_token_type(parser) != TOKEN_COLON) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_COLON);
		// return NULL;
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
			
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_DATATYPE);
		// return NULL;
	}


	{
		Token tok = peek_token(parser);
		data_t type = get_type(&tok);
		struct type* t = create_type(type, NULL);
			
		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
			advance_parser(parser);
			struct type* array_type = create_type(TYPE_ARRAY, t);
			printf(" JUST MOVED PAST TOKEN_LEFT_BRACKET and Current token type is '%d'\n", peek_token_type(parser));

			Node* expr_node = parse_logical_or(parser, info);
			if (!expr_node) {
				printf("Error: Unable to retrieve size of array\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token token = peek_token(parser);
				report_error(&token, info, EXPECTED_RIGHT_BRACKET);
				// return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				let_node = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser); 
				
				Node* assignee = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
				Node* elements = parse_array_list(parser, info);

				let_node = create_string_node(NODE_ASSIGNMENT, NULL, assignee, elements, NULL, NULL, NULL);   
				
				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token token = peek_token(parser);
					report_error(&tok, info, EXPECTED_SEMICOLON);
					// return NULL;
				}
				advance_parser(parser);
			} 
		} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
			let_node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
			advance_parser(parser);
		} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
			advance_parser(parser);

			Node* assignee = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
			Node* expr_node = parse_logical_or(parser, info);
			
			if (!expr_node) {
				printf("Error: EXPR NODE IN 'parse_let' is null\n");
				return NULL;
			}
			
			let_node = create_string_node(NODE_ASSIGNMENT, NULL, assignee, expr_node, NULL, NULL, NULL);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token token = peek_token(parser);
				report_error(&tok, info, EXPECTED_SEMICOLON);
				// return NULL;
			}
			advance_parser(parser);
		}
		free(id);
	}
		
	return let_node;
}

Node* parse_enum_body(Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current = NULL;
	Node* stmt = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_IDENTIFIER);
			// return NULL;
		}

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			id = strdup(tok.value.str);
			if (!id) return NULL;
		}

		stmt = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
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

Node* parse_enum(Parser* parser, FileInfo* info) {
	Node* enum_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		id = strdup(tok.value.str);
		if (!id) return NULL;
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_BRACE);
		free(id);
		// return NULL;
	}

	advance_parser(parser);

	Node* enum_body = parse_enum_body(parser, info);
	if (!enum_body) {
		printf("Error: body for enum '%s' is NULL\n", id);
		free(id);
		return NULL;
	}

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_SEMICOLON);
		free(id);
		// return NULL;
	}

	advance_parser(parser);

	enum_node = create_string_node(NODE_ENUM, id, NULL, enum_body, NULL, NULL, NULL);
	free(id);
	return enum_node;
}

Node* parse_struct(Parser* parser, FileInfo* info) {
	Node* struct_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_IDENTIFIER);
		// return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		id = strdup(tok.value.str);
		if (!id) return NULL;
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_BRACE);
		free(id);
		// return NULL;
	}

	advance_parser(parser);

	Node* struct_body = parse_block(parser, info);
	if (!struct_body) {
		printf("Error: body for Struct '%s' is NULL\n", id);
		free(id);
		return NULL;
	}

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_SEMICOLON);
		free(id);
		// return NULL;
	}

	advance_parser(parser); // skip ';'

	struct_node = create_string_node(NODE_STRUCT, id, NULL, struct_body, NULL, NULL, NULL);
	return struct_node;
}

void synchronize(Parser* parser, token_t* synchronizations) {
	size_t length = sizeof(synchronizations) / sizeof(synchronizations[0]);

	while (!at_token_eof(parser)) {
		token_t current_type = peek_token_type(parser);
		for (size_t i = 0; i < length; i++) {
			if (current_type == synchronizations[i]) {
				printf("Found synchronization type: %d\n", current_type);
				return;
			}
		}
		advance_parser(parser);
	}
}

Node* parse(Token* tokens, FileInfo* info) {
	init_error_list();

	Parser* parser = initialize_parser(tokens);
	if (!parser) {
		printf("In 'build_AST', parser is NULL\n");
		return NULL;
	}

	Node* head = NULL;
	Node* current = NULL;
	Node* node = NULL;

	while (!at_token_eof(parser)) {

		if (at_token_eof(parser)) { break; }

		token_t current_type = peek_token_type(parser);

		if (current_type == TOKEN_FUNCTION_KEYWORD) {
			node = parse_function(parser, info);
		}  else if (current_type == TOKEN_LET_KEYWORD) {
			node = parse_let(parser, info);
		} else if (current_type == TOKEN_STRUCT_KEYWORD) {
			node = parse_struct(parser, info);
		} else if (current_type == TOKEN_ENUM_KEYWORD) {
			node = parse_enum(parser, info);
		} else {
			printf("Unexpected token type: %d\n", peek_token_type(parser));
			advance_parser(parser);
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
			synchronize(parser, synchronizations);
		}
	}

	if (has_errors) {
		emit_errors(errors);
		free_error_list();
		exit(EXIT_FAILURE);
	}

	free(parser);
	return head;

}

#define BRANCH "├"
#define LAST_BRANCH "└"
#define CONTINUATION "│    "

char* get_type_name(data_t type) {
	switch (type) {
		case TYPE_INTEGER: return "int";
		case TYPE_CHAR: return "char";
		case TYPE_BOOL: return "bool";
		case TYPE_VOID: return "void";
	}
}

void print_expression_recursive(Node* expr, char* child_prefix, char* stmt_connector, bool* is_last_stmt) {
	if (!expr) return;

	switch (expr->type) {
		case NODE_NAME: {
			printf("Identifier");
			break;
		}

		case NODE_CALL: { break; }

		case NODE_ARG: { break; }

		case NODE_PARAM: { break; }

		case NODE_ADD: 
		case NODE_SUB:
		case NODE_MUL:
		case NODE_DIV:
		case NODE_ADD_EQUAL:
		case NODE_SUB_EQUAL:
		case NODE_MUL_EQUAL:
		case NODE_DIV_EQUAL:
		case NODE_LESS:
		case NODE_GREATER:
		case NODE_LESS_EQUAL:
		case NODE_GREATER_EQUAL:
		case NODE_EQUAL:
		case NODE_NOT_EQUAL:
		case NODE_LOGICAL_AND:
		case NODE_LOGICAL_OR: {
			if (expr->left) {
				print_expression_recursive(expr->left, child_prefix, stmt_connector, is_last_stmt);
			}

			if (expr->right) {
				print_expression_recursive(expr->right, child_prefix, stmt_connector, is_last_stmt);
			}

			break;
		}

		case NODE_NOT: {
			if (expr->right) {
				print_expression_recursive(expr->right, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			if (expr->left) {
				print_expression_recursive(expr->left, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}
	}
}

void print_statement_recursive(Node* stmt, char* child_prefix, char* stmt_connector, bool* is_last_stmt) {
	if (!stmt) return;

	switch (stmt->type) {
		case NODE_ASSIGNMENT: {
			printf("Assignment\n");
			if (stmt->left) {
				print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt);
			}

			if (stmt->right) {
				print_expression_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}

		case NODE_RETURN: {
			if (stmt->right) {
				print_expression_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}

		case NODE_WHILE:
		case NODE_IF:
		case NODE_ELSE_IF: {
			if (stmt->left) {
				print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt);
			}

			if (stmt->right) {
				print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}

		case NODE_ELSE: {
			if (stmt->right) {
				print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt);
			}
			break;
		}

		case NODE_FOR: {
			if (stmt->left) {
				print_statement_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt);
			}

			Node* condition = stmt->left->next;
			if (condition) {
				print_expression_recursive(condition, child_prefix, stmt_connector, is_last_stmt);
			}

			Node* update = condition->next;
			if (update) {
				print_expression_recursive(update, child_prefix, stmt_connector, is_last_stmt);
			}

			if (stmt->right) {
				print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt);
			}

			break;
		}

		case NODE_CONTINUE: 
		case NODE_BREAK:  { break; }
	}
}

void print_ast(Node* root) {
	printf("\033[31mProgram\033[0m\n");

	int total_nodes = 0;
	Node* node = root;
	while (node) {
		Node* next = node->next;
		total_nodes++;
		node = next;
	}

	int index = 0;
	node = root;
	while (node) {
		Node* next = node->next;
		bool is_last = (index == total_nodes - 1);
		
		char* prefix = is_last ? LAST_BRANCH : BRANCH;

		if (node->t && node->t->kind == TYPE_FUNCTION) {
			printf("%sIdentifier: '%s'", prefix, node->value.name);
			printf(" [function -> ");
			if (node->t->subtype) {
				printf("%s]\n", get_type_name(node->t->subtype->kind));
			}
			
			char* child_prefix = is_last ? "    " : CONTINUATION;

			if (node->left) {
				int total_params = 0;
				Node* param = node->left;
				while (param) {
					Node* next = param->next;
					total_params++;
					param = next;
				}

				int param_index = 0;
				param = node->left;
				printf("%sParameters\n", child_prefix);
				while (param) {
					Node* next = param->next;
					bool is_last_param = (param_index == total_params - 1);
					char* param_connector = is_last_param ? LAST_BRANCH : BRANCH;

					printf("%s%sIdentifier: \'%s\'", child_prefix, param_connector, param->right->value.name);
					if (param->right->t) {
						printf(" [%s]\n", get_type_name(param->right->t->kind));
					}

					param_index++;
					param = next;
				}
			}

			if (node->right) {
				int total_stmts = 0;
				Node* stmt_node = node->right;
				while (stmt_node) {
					Node* next = stmt_node->next;
					total_stmts++;
					stmt_node = next;
				
				}

				int stmt_index = 0;
				stmt_node = node->right;
				
				printf("%sBody\n", child_prefix);
				while (node) {
					Node* next = node->next;
					bool is_last_stmt = (stmt_index == total_stmts - 1);
					char* stmt_connector = is_last_stmt ? LAST_BRANCH : BRANCH;
					print_statement_recursive(node, child_prefix, stmt_connector, &is_last_stmt);
					stmt_index++;
					node = next;
				}
			}
		} 


		index++;
		node = next;
	}
	
}
	
void free_type(struct type* t) {
	if (!t || t->type_free) return;

	t->type_free = true;

	if (t->subtype) {
		free_type(t->subtype);
	}
	free(t);
}

void free_expression(Node* node) {
	if (!node || node->node_free) return;

	node->node_free = true;

	if (node->type == NODE_NAME) {
		if (node->value.name) {
			free(node->value.name);
			node->value.name = NULL;
		}
	}

	if (node->left) {
		free_expression(node->left);
	}

	if (node->right) {
		free_expression(node->right);
	}

	if (node->t) {
		free_type(node->t);
		node->t = NULL;
	}

	node->prev = NULL;
	node->next = NULL;

	free(node);
}

void free_parameter_list(Node* head) {
	if (!head) return;

	Node* current = head;
	while (current) {
		Node* next = current->next;
		current->next = NULL;
		current->prev = NULL;
		free_expression(current);
		current = next;
	}
}

void free_statement_list(Node* head) {
	if (!head) return;

	Node* current = head;
	while (current) {
		Node* next_stmt = current->next;
		current->next = NULL;
		current->prev = NULL;

		switch (current->type) {
			case NODE_ASSIGNMENT: {
				free_expression(current->left);
				free_expression(current->right);
				break;
			}

			case NODE_RETURN: {
				free_expression(current->right);
				break;
			}

			case NODE_NAME: {
				break;
			}

			default: 
				if (current->left) free_expression(current->left);
				if (current->right) free_expression(current->right);
				break;

		}

		if (current->type == NODE_NAME) {
			if (current->value.name) {
				free(current->value.name);
				current->value.name = NULL;
			}
		}

		if (current->t) {
			free_type(current->t);
			current->t = NULL;
		}

		free(current);
		current = next_stmt;
	}
}

void free_node(Node* node) {
	if (!node || node->node_free) return;

	node->node_free = true;

	switch (node->type) {
		case NODE_NAME: {
			if (node->t && node->t->kind == TYPE_FUNCTION) {
				if (node->value.name) {
					free(node->value.name);
					node->value.name = NULL;
				}

				if (node->left) {
					free_parameter_list(node->left);
					node->left = NULL;
				}

				if (node->right) {
					free_statement_list(node->right);
					node->right = NULL;
				}

				if (node->t) {
					free_type(node->t);
					node->t = NULL;
				}

			} else if (node->t && node->t->kind == TYPE_ARRAY) {
				if (node->value.name) {
					free(node->value.name);
					node->value.name = NULL;
				}

				if (node->left) {
					free_expression(node->left);
					node->left = NULL;
				}

				if (node->right) {
					free_expression(node->right);
					node->right = NULL;
				}

				if (node->t) {
					free_type(node->t);
					node->t = NULL;
				}
			}
			break;
		}

		case NODE_ASSIGNMENT: {
			if (node->value.name) {
				free(node->value.name);
				node->value.name = NULL;
			}

			if (node->left) {
				free_expression(node->left);
				node->left = NULL;
			}

			if (node->right) {
				free_parameter_list(node->right);
				node->right = NULL;
			}

			if (node->t) {
				free_type(node->t);
				node->t = NULL;
			}
			break;
		}
	}

	node->prev = NULL;
	node->next = NULL;
	free(node);
}

void free_ast(Node* root) {
	if (!root) return;

	Node* current = root;
	while (current) {
		Node* next = current->next;
		current->next = NULL;
		current->prev = NULL;
		free_node(current);
		current = next;
	}
}