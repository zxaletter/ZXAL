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
		case TOKEN_ID: return "identifier";
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
	if (!copy_token->type) {
		perror("Unable to copy type\n");
		free(copy_token);
		return NULL;
	}
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

		copy_token->value.str = strdup(original_token->value.str);
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
		return;
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

	int gutter_width = snprintf(NULL, 0, "%d", error->token->line);
	char space[gutter_width + 1];
	for (int i = 0; i < gutter_width; i++) {
		space[i] = ' ';
	}
	space[gutter_width] = '\0';

	switch (error->type) {
		// case EXPECTED_STRUCT_KEYWORD:
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

		case EXPECTED_SINGLE_QUOTE: {
			printf("Syntax Error: Missing '%s'\n", get_token_string(TOKEN_SINGLE_QUOTE));
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

			if (error->token->value.str) {
				free(error->token->value.str);
			}
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
	free(errors->list);
	free(errors);
}

void report_error(Token* tok, FileInfo* info, error_t type) {
	
	char* message = malloc(1024);
	if (!message) return;

	snprintf(message, 1024, "\033[31mError\033[0m in file '%s' at line %d, column %d:\n", info->filename, tok->line, tok->column);
	create_error(type, message, tok, info);
	free(message);
	
}

struct type* create_type(data_t main_type, struct type* subtype, Node* params) {
	struct type* type = malloc(sizeof(struct type));
	if (!type) {
		perror("Unable to allocate space for type\n");
		return NULL;
	}

	type->kind = main_type;
	type->type_free = false;
	type->subtype = NULL;
	type->params = NULL;

	if (subtype) {
		type->subtype = type_copy(subtype);
		if (!type->subtype) {
			perror("Unable to copy subtype\n");
			free(type);
			return NULL;
		}
	}

	if (params) {
		type->params = params_copy(params);
		if (!type->params) {
			perror("Unable to copy params.\n");
			free_type(type);
			return NULL;
		}
	}

	return type;
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
		case TOKEN_BOOL_KEYWORD: kind = TYPE_BOOL; break;
		case TOKEN_STRUCT_KEYWORD: kind = TYPE_STRUCT; break;
		case TOKEN_VOID_KEYWORD: kind = TYPE_VOID; break;
		default: {
			printf("Defaulting to unknown type\n");
			kind = TYPE_UNKNOWN;
			break;
		}
	}

	return kind;
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
	if (t) {
		node->t = type_copy(t);
		if (!node->t) {
			free(node);
			return NULL;
		}		 
	} else {
		node->t = NULL;
	}
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

Node* create_char_node(node_t type, char ch, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {

	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		printf("Error in 'create_char_node()': NULL '<-' from create_node()\n");
		return NULL;
	}
	node->value.c = ch;
	return node;
}

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {
	
	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		printf("Error: In 'create_string_node()': NULL '<-' from 'create_node()'\n");
		return NULL;
	} else {
		printf("created string node\n");
	}

	if (id) {
		node->value.name = strdup(id);
		if (!node->value.name) {
			printf("Error: Unable to allocate space for string in 'create_string_node()'\n");
			if (node->t) {
				free_type(node->t);
			}
			free(node);
			return NULL;
		}
	} else {
		node->value.name = NULL;
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
		default: op = NODE_UNKNOWN; return op;
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
			if (node) { printf("Created node with name '%s' and type: '%d'\n", node->value.name, node->type); }
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(parser, info);
				struct type* t = create_type(TYPE_FUNCTION, NULL, NULL);
				node = create_node(NODE_CALL, node, args, NULL, NULL, t);
				
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
				node = create_node(NODE_SUBSCRIPT, node, expr_node, NULL, NULL, NULL);
				if (node) { printf("\033[32mCreated subscript node in parse_factor\033[0m\n"); }

				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				node = create_node(NODE_INCREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				node = create_node(NODE_DECREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			}

			break;
		}

		case TOKEN_INTEGER: {
			Token tok = peek_token(parser);
			int val = tok.value.val;
			struct type* t = create_type(TYPE_INTEGER, NULL, NULL);
			node = create_int_node(NODE_INTEGER, val, NULL, NULL, NULL, NULL, t);
			advance_parser(parser);
			break;
		}

		case TOKEN_CHAR_LITERAL: {
			Token tok = peek_token(parser);
			char ch = tok.value.c;
			struct type* t = create_type(TYPE_CHAR, NULL, NULL);
			node = create_char_node(NODE_CHAR, ch, NULL, NULL, NULL, NULL, t);
			printf("IN TOKEN CHAR LITERAL: current token type: '%d'\n", peek_token_type(parser));
			advance_parser(parser);
			printf("Before returning before token char literal new token type is: '%d'\n", peek_token_type(parser));
			break;
		}

		case TOKEN_TRUE_KEYWORD: {
			// printf("about to create NODE_BOOL for true\n");
			node = create_int_node(NODE_BOOL, 1, NULL, NULL, NULL, NULL, NULL);
			// if (node) {
			// 	printf("Created node bool for true\n");
			// }
			advance_parser(parser);
			break;
		}

		case TOKEN_FALSE_KEYWORD: {
			// printf("about to create NODE_BOOL for false\n");
			node = create_int_node(NODE_BOOL, 0, NULL, NULL, NULL, NULL, NULL);
			// if (node) {
			// 	printf("created node bool for false\n");
			// }
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

	}
	return node;
}

Node* parse_unary(Parser* parser, FileInfo* info) {
	Node* unary_op = NULL;
	Node* last_op = NULL;

	if (peek_token_type(parser) == TOKEN_NOT ||
		peek_token_type(parser) == TOKEN_ADD ||
		peek_token_type(parser) == TOKEN_SUB) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		unary_op = create_node(op_kind, NULL, NULL, NULL, NULL, NULL);
		advance_parser(parser);
	}

	while (peek_token_type(parser) == TOKEN_NOT ||
		   peek_token_type(parser) == TOKEN_ADD ||
		   peek_token_type(parser) == TOKEN_SUB) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		unary_op = create_node(op_kind, NULL, unary_op, NULL, NULL, NULL);
		advance_parser(parser);
	}

	Node* node = NULL;

	if (unary_op) {
		for (node = unary_op; node; node = node->right) {
			if (!node->right) { last_op = node; }
		}	
	}

	if (!unary_op) {
		last_op = parse_factor(parser, info);
		return last_op;
	} else {
		Node* right = parse_factor(parser, info);
		last_op->right = right;
		return unary_op;
	}
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
	printf("In parse additive\n");
	Node* node = parse_multiplicative(parser, info);

	while (peek_token_type(parser) == TOKEN_ADD || peek_token_type(parser) == TOKEN_SUB ||
		   peek_token_type(parser) == TOKEN_ADD_EQUAL || peek_token_type(parser) == TOKEN_SUB_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_multiplicative(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}

	if (node) { printf("Got node from parse additive\n"); }
	return node;
}

Node* parse_relational(Parser* parser, FileInfo* info) {
	printf("In parse relational\n");
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

	if (node) {
		printf("Got node from parse relational\n");
	}
	return node;
}

Node* parse_logical_and(Parser* parser, FileInfo* info) {
	printf("In parse logical and\n");
	Node* node = parse_relational(parser, info);

	while (peek_token_type(parser) == TOKEN_LOGICAL_AND) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_relational(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}

	if (node) {
		printf("Got node from parse logical and\n");
	}

	return node;
}

Node* parse_logical_or(Parser* parser, FileInfo* info) {
	printf("In parse logical or\n");
	Node* node = parse_logical_and(parser, info);
	while (peek_token_type(parser) == TOKEN_LOGICAL_OR) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_logical_and(parser, info);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	if (node) {
		printf("Got node from parse logical or\n");
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
				return NULL;
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
				free(id);
				return NULL;
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_INT_KEYWORD &&
				peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
				peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
				peek_token_type(parser) != TOKEN_STRUCT_KEYWORD) {

				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_DATATYPE);	
				free(id);
				return NULL;
			}

			Token tok = peek_token(parser);
			data_t type = get_type(&tok);
			struct type* t = NULL;

			t = create_type(type, NULL, NULL);
			if (!t) {
				free(id);
				return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);

				struct type* array_type = create_type(TYPE_ARRAY, t, NULL);
				if (!array_type) {
					free_type(t);
					free(id);
					return NULL;
				}

				Node* expr_node = parse_logical_or(parser, info);
				if (!expr_node) {
					free_type(array_type);
					free(id);
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token token = peek_token(parser);
					report_error(&token, info, EXPECTED_RIGHT_BRACKET);
					free_expression(expr_node);
					free_type(array_type);
					free(id);
					return NULL;
				}

				advance_parser(parser);
				if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					Node* decl = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
					if (!decl) {
						free_expression(expr_node);
						free_type(array_type);
						free(id);
						return NULL;
					}
					stmt = create_node(NODE_DECL, NULL, NULL, NULL, NULL, NULL);
					if (!stmt) {
						free_expression(decl);
						return NULL;
					}
				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					advance_parser(parser); // move past '='
					int element_count = 0;
					Node* elements = parse_array_list(parser, info, &element_count);

 					Node* array_elements = create_node(NODE_ARRAY_LIST, NULL, elements, NULL, NULL, NULL);
					Node* count = create_int_node(NODE_INTEGER, element_count, NULL, NULL, NULL, NULL, NULL);
					Node* assignee = create_string_node(NODE_NAME, id, expr_node, count, NULL, NULL, array_type);
											
					Node* def = create_node(NODE_DEF, assignee, NULL, NULL, NULL, NULL);
					stmt = create_node(NODE_ASSIGNMENT, def, array_elements, NULL, NULL, NULL);  
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
				if (!assignee) {
					free_type(t);
					free(id);
					id = NULL;
					return NULL;
				}

				Node* def = create_node(NODE_DEF, assignee, NULL, NULL, NULL, NULL);
				if (!def) {
					free_expression(assignee);
					// free_type(t);
					free(id);
					id = NULL;
					return NULL;
				}

				advance_parser(parser);
				Node* expr_node = parse_logical_or(parser, info);
				if (!expr_node) {
					free_expression(def);
					free(id);
					id = NULL;
					return NULL;
				} 
				printf("After getting expr node, current token type is: '%d'\n", peek_token_type(parser));
				stmt = create_node(NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL);
				if (!stmt) {
					free_expression(def);
					free_expression(expr_node);
					free(id);
					id = NULL;
					return NULL;
				}
			}

			if (id) {
				free(id);
			}
			break;

		}
		
		case TOKEN_RETURN_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(NODE_RETURN, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {				
				Node* node = parse_logical_or(parser, info);
				if (node) {
					printf("got node in return\n");
				} else {
					printf("Didnt get node in return\n");
				}
				stmt = create_node(NODE_RETURN, NULL, node, NULL, NULL, NULL);
			}
			break;
		}

		case TOKEN_CONTINUE_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_SEMICOLON);
				
			}

			stmt = create_node(NODE_CONTINUE, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case TOKEN_BREAK_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_SEMICOLON);
				
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

				// printf("IN FOR CASE Current token type is %d\n", peek_token_type(parser));
				// advance_parser(parser); // skipping ';'
				Node* condition = parse_logical_or(parser, info);
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
						struct type* type = create_type(kind, NULL, NULL);

						Node* var = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, type); 
						Node* def = create_node(NODE_DEF, var, NULL, NULL, NULL, NULL);
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
						printf("Update type is %d\n", update->type);
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

		case TOKEN_STRUCT_KEYWORD: {
			printf("in token struct keyword case\n");
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_ID) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_IDENTIFIER);
			}

			char* id = NULL;
			{
				Token tok = peek_token(parser);
				id = strdup(tok.value.str);
				if (!id) {
					printf("Unable to duplicate struct identifier\n");
					return NULL;
				} 
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				report_error(&tok, info, EXPECTED_LEFT_BRACE);
			}

			advance_parser(parser);
			Node* struct_body = parse_block(parser, info);
			if (!struct_body) {
				free(id);
				return NULL;
			}

			struct type* t = create_type(NODE_STRUCT_DEF, NULL, NULL);
			if (!t) {
				free_statement(struct_body);
				free(id);
				return NULL;
			}
			stmt = create_string_node(NODE_STRUCT_DEF, id, NULL, struct_body, NULL, NULL, t);
			free(id);

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
				Node* subscript_node = create_node(NODE_SUBSCRIPT, node, index_node, NULL, NULL, NULL);
				if (node) {
					printf("created subscript node\n");
				}

				if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_ASSIGNMENT);
					printf("Currently not at assignment node\n");
					// return NULL;
				}

				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(parser, info);
				Node* aug_expr_node = create_node(NODE_AUG, expr_node, NULL, NULL, NULL, NULL);
				Node* aug = create_node(NODE_AUG, subscript_node, NULL, NULL, NULL, NULL);
				stmt = create_node(NODE_ASSIGNMENT, aug, aug_expr_node, NULL, NULL, NULL);

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
				Node* def = create_node(NODE_AUG, node, NULL, NULL, NULL, NULL);
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
				struct type* t = create_type(type, NULL, NULL);

				advance_parser(parser);
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					report_error(&tok, info, EXPECTED_SEMICOLON);
					free(id);
					// return NULL;
				}
				Node* decl = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				stmt = create_node(NODE_DECL, decl, NULL, NULL, NULL, NULL);
			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				stmt = create_node(NODE_INCREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				stmt = create_node(NODE_DECREMENT, node, NULL, NULL, NULL, NULL);
				advance_parser(parser);
			}
			free(id);
			break;
		}

	}

	if (!special_statement) {
		if (peek_token_type(parser) != TOKEN_SEMICOLON) {

			Token tok = peek_token(parser);
			printf("Current token type is: '%d'\n", tok.type);
			report_error(&tok, info, EXPECTED_SEMICOLON);
			// return NULL;
		}	
		advance_parser(parser);	
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
			printf("\033[31mStatement type is '%d'\033[0m\n", stmt->type);
			if (stmt->left) { printf("\033[31mLeft child of statement has type: '%d'\033[0m\n", stmt->left->type); }
			if (stmt->right) { printf("\033[31mRight child of statement has type: '%d'\033[0m\n", stmt->right->type); }
			if (!head) {
				printf("first connection of linked list\n");
				head = stmt;
				current = stmt;
				printf("Connected\n");
			} else {
				printf("subsequent connections of linked list\n");
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

	if (block) {
		block->right = head;
	}
	advance_parser(parser);
	return block;
}

Node* parse_parameters(Parser* parser, FileInfo* info) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;
	Node* wrapped_param = NULL;

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
			} else {
				printf("Got '%s'\n", id);
			}			
		}
		
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_COLON) {
			Token tok = peek_token(parser);
			report_error(&tok, info, EXPECTED_COLON);
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
		struct type* t = create_type(param_type, NULL, NULL);
		if (!t) {
			printf("Unable to create type in 'parse_parameters\n");
			free(id);
			return NULL;
		}

		advance_parser(parser);

		node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
		if (!node) {
			printf("Unable to create node with type NODE_NAME in 'parse_parameters.\n");
			free_type(t);
			free(id);
			return NULL;
		}
		
		wrapped_param = create_node(NODE_PARAM, NULL, node, NULL, NULL, NULL);
		if (!wrapped_param) {
			printf("Unable to create wrapped param node.\n");
			free_expression(node);
			free_type(t);
			free(id);
			return NULL;
		}

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
			} else {
				printf("Function id is: '%s'\n", id);
			}
		} else {
			printf("Token does not have string\n");
			return NULL;
		}
	}

	advance_parser(parser);
	if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_PARENTHESES);	
		free(id);
		return NULL;
	}

	advance_parser(parser);
	Node* params = parse_parameters(parser, info);
	
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ARROW) {
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_ARROW);
		free_parameters(params);
		free(id);
		return NULL;
	}

	advance_parser(parser);
	
	if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
		peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
		peek_token_type(parser) != TOKEN_VOID_KEYWORD) {
			
		Token tok = peek_token(parser);
		report_error(&tok, info, EXPECTED_DATATYPE);
		free_parameters(params);
		free(id);
		return NULL;
	}

	Token tok = peek_token(parser);
	data_t return_type = get_type(&tok);
	struct type* subtype = create_type(return_type, NULL, NULL);
	if (!subtype) {
		free_parameters(params);
		free(id);
		return NULL;
	}

	struct type* t = create_type(TYPE_FUNCTION, subtype, params);
	if (!t) {
		free_type(subtype);
		free_parameters(params);
		free(id);
		return NULL;
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token token = peek_token(parser);
		report_error(&tok, info, EXPECTED_LEFT_BRACE);
		free_type(t);
		free_parameters(params);
		free(id);
		return NULL;
	}

	advance_parser(parser);
	Node* body = parse_block(parser, info);
	if (body) {
		printf("got block in parse funciton\n");
	}
	if (!body) {
		free_type(t);
		free_type(subtype);
		free_parameters(params);
		free(id);
		return NULL;
	}

	function_node = create_string_node(NODE_NAME, id, NULL, body, NULL, NULL, t);

	free(id);

	if (!function_node) {
		free_statement(body);
		free_type(t);
		free_type(subtype);
		free_parameters(params);
		free(id);
		return NULL;
	} else {
		printf("Made function node\n");
	}

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

Node* parse_array_list(Parser* parser, FileInfo* info, int* element_count) {

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
	advance_parser(parser);
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
		struct type* t = create_type(type, NULL, NULL);
			
		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
			advance_parser(parser);
			struct type* array_type = create_type(TYPE_ARRAY, t, NULL);
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
				int element_count = 0;
				
				Node* assignee = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
				Node* elements = parse_array_list(parser, info, &element_count);

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
	struct type* t = create_type(TYPE_STRUCT, NULL, NULL);
	struct_node = create_string_node(NODE_STRUCT_DEF, id, NULL, struct_body, NULL, NULL, t);
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
	printf("in parse\n");
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
			printf("in func\n");
			node = parse_function(parser, info);
		}  else if (current_type == TOKEN_LET_KEYWORD) {
			printf("got let\n");
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

void print_expression_recursive(Node* expr, char* child_prefix, char* stmt_connector, bool* is_last_stmt, int* level) {
	if (!expr) return;
	switch (expr->type) {
		case NODE_NAME: {
			printf("Identifier\n");
			break;
		}

		case NODE_CALL: { 
			if (expr->right) {
				Node* arg = expr->right;
				while (arg) {
					Node* arg_next = arg->next;
					*level++;
					print_expression_recursive(arg, child_prefix, stmt_connector, is_last_stmt, level);
					arg = arg_next;
					*level--;
				}
			}
			break;
		}

		case NODE_ARG: { 
			if (expr->right) {
				Node* arg = expr->right;
				*level++;
				print_expression_recursive(arg, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}
			break;
		}

		case NODE_ARRAY_LIST: {
			if (expr->right) {
				Node* element = expr->right;
				while (element) {
					Node* next_element = element->next;
					*level++;
					print_expression_recursive(element, child_prefix, stmt_connector, is_last_stmt, level);
					*level--;
					element = next_element;
				}
			}
			break;
		}

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
				*level++;
				print_expression_recursive(expr->left, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}

			if (expr->right) {
				*level++;
				print_expression_recursive(expr->right, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}

			break;
		}

		case NODE_NOT: {
			if (expr->right) {
				*level++;
				print_expression_recursive(expr->right, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			if (expr->left) {
				*level++;
				print_expression_recursive(expr->left, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}
			break;
		}
	}
}

void print_statement_recursive(Node* stmt, char* child_prefix, char* stmt_connector, bool* is_last_stmt, int* level) {
	if (!stmt) return;

	char* stmt_prefix[100];

	switch (stmt->type) {
		case NODE_ASSIGNMENT: {
			if (stmt->left) {
				*level++;
				print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}

			if (stmt->right) {
				*level++;
				print_expression_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
				*level--;
			}
			break;
		}

		case NODE_RETURN: {
			if (stmt->right) {
				print_expression_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
			}
			break;
		}

		case NODE_BLOCK: {
			int stmt_count = 0;
			if (stmt->right) {
				Node* node = stmt->right;
				while (node) {
					Node* next = node->next;
					stmt_count++;
					node = next;
				}
			}

			int stmt_index = 0;
			Node* node = stmt->right; 
			char* stmt_prefix = malloc(100);
			bool last_body_stmt = (stmt_index == stmt_count - 1);
			
			for (int i =0; i < *level; i++) {
				stmt_prefix[i] = "    ";
			}

			char* connector = last_body_stmt ? LAST_BRANCH : BRANCH;
			
			printf("%s%sBlock\n", child_prefix, connector);
			while (node) {
				Node* next = node->next;
				*level++;
				print_statement_recursive(node, stmt_prefix, connector, &last_body_stmt, level);
				*level--;
				node = next;
				stmt_index++;
			}
			break;
		}

		case NODE_WHILE: {
			printf("%s%sWhile loop\n", child_prefix, stmt_connector);
			if (stmt->left) print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt, level);
			if (stmt->right) print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);

			break;
		}
		case NODE_IF: {
			printf("If");
			if (stmt->left) print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt, level);
			if (stmt->right) print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
			break;
		}

		case NODE_ELSE_IF: {
			printf("Else If");
			if (stmt->left) print_expression_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt, level);
			if (stmt->right) print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
			break;
		}

		case NODE_ELSE: {
			if (stmt->right) {
				print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
			}
			break;
		}

		case NODE_FOR: {
			if (stmt->left) {
				print_statement_recursive(stmt->left, child_prefix, stmt_connector, is_last_stmt, level);
			}

			Node* condition = stmt->left->next;
			if (condition) {
				print_expression_recursive(condition, child_prefix, stmt_connector, is_last_stmt, level);
			}

			Node* update = condition->next;
			if (update) {
				print_expression_recursive(update, child_prefix, stmt_connector, is_last_stmt, level);
			}

			if (stmt->right) {
				print_statement_recursive(stmt->right, child_prefix, stmt_connector, is_last_stmt, level);
			}

			break;
		}

		case NODE_CONTINUE: 
		case NODE_BREAK:  { break; }
		default: {
			printf("No matching cases\n");
			break;
		}
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
				int level = 1;

				printf("%sBody\n", child_prefix);
				while (stmt_node) {
					Node* stmt_next = stmt_node->next;
					bool is_last_stmt = (stmt_index == total_stmts - 1);
					char* stmt_connector = is_last_stmt ? LAST_BRANCH : BRANCH;
					print_statement_recursive(stmt_node, child_prefix, stmt_connector, &is_last_stmt, &level);
					stmt_index++;
					stmt_node = stmt_next;
				}
			}
		} 


		index++;
		node = next;
	}
	
}
	
void free_type(struct type* t) {
	if (!t || (t && t->type_free)) return;

	t->type_free = true;

	if (t->subtype) {
		free_type(t->subtype);
	}

	if (t->params) {
		free_parameters(t->params);
	}
	free(t);
}

void free_expression(Node* node) {
	printf("In free expression\n");
	if (!node || (node && node->node_free)) {
		printf("node is null");

		return;
	}
	printf("w\n");
	node->node_free = true;
	printf("got here\n");
	switch (node->type) {
		case NODE_BOOL:
		case NODE_CHAR: 
		case NODE_INTEGER: { 
			break; 
		}

		case NODE_PARAM: {
			if (node->right) {
				free_expression(node->right);
			}
			break;
		}

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
		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {
			if (node->left) free_expression(node->left);
			if (node->right) free_expression(node->right);
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			if (node->left) free_expression(node->left);
			break;
		}

		case NODE_NOT: {
			if (node->right) free_expression(node->right);
			break;
		}

		case NODE_CALL: {
			if (node->left) free_expression(node->left);
			if (node->right) {
				Node* arg = node->right;
				while (arg) {
					Node* arg_next = arg->next;
					free_expression(arg);
					arg = arg_next;
				}
			}
			break;
		}

		case NODE_ARG: {
			if (node->right) {
				Node* arg = node->right;
				free_expression(arg);
			}
			break;
		}

		case NODE_SUBSCRIPT: {
			if (node->left) free_expression(node->left);
			if (node->right) free_expression(node->right);
			if (node->t) free_type(node->t);
			break;
		}

		case NODE_ARRAY_LIST: {
			if (node->right) {
				Node* element = node->right;
				while (element) {
					Node* next_element = element->next;
					free_expression(element);
					element = next_element;
				}
			}
			break;
		}

		case NODE_NAME: {
			if (node->t) {
				if (node->t->kind == TYPE_ARRAY) {
					if (node->value.name) {
						free(node->value.name);
						node->value.name = NULL;
					}
					
					if (node->left) {
						free_expression(node->left);
						node->left = NULL;
					}
					if (node->right) { 
						printf("About to free node->right\n");
						free_expression(node->right);
						node->right = NULL;
					}
					if (node->t) {
						free_type(node->t);
						node->t = NULL;
					}
					if (node->symbol) free_symbol(node->symbol);
				
				} else {
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

					if (node->symbol) free_symbol(node->symbol);
				}
			}
			break;
		}
		case NODE_DEF: {
			printf("in node def\n");
			if (node->left) free_expression(node->left);
			break;
		}
		case NODE_DECL: {
			printf("in node decl\n");
			if (node->left) free_expression(node->left);
			break;
		}
		case NODE_AUG: {
			printf("in node aug\n");
			if (node->left) free_expression(node->left);
			break;
		}
	}

	free(node);
}

void free_parameters(Node* node) {
	if (!node) return;
	// node type -> NODE_PARAM
	// nodde->right type -> e.g. NODE_NAME
	Node* wrapped_param = node;
	while (wrapped_param) {
		Node* next_wrapped_param = wrapped_param->next;
		free_expression(wrapped_param);
		wrapped_param = next_wrapped_param;
	}
}

void free_statement(Node* node) {
	if (!node || (node && node->node_free)) return;

	node->node_free = true;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			printf("in node assignment in free_statement()\n");
			if (node->left) free_expression(node->left);
			if (node->right) free_expression(node->right);
			node->left = NULL;
			node->right = NULL;
			break;
		}

		case NODE_RETURN: {
			free_expression(node->right);
			break;
		}

		case NODE_WHILE:
		case NODE_IF:
		case NODE_ELSE_IF: {
			if (node->left) free_expression(node->left);
			if (node->right) free_expression(node->right);
			break;
		}

		case NODE_STRUCT_DEF: {
			if (node->value.name) {
				free(node->value.name);
				node->value.name = NULL;
			}

			if (node->right) free_statement(node->right);
			
			if (node->t) free_type(node->t);
			break;
		}
		case NODE_FOR: {
			Node* initializer = node->left;
			Node* condition = NULL;
			Node* update = NULL;
			Node* body = node->right;

			if (initializer) {
				condition = initializer->next;

				if (condition) {
					update = condition->next;
				}
			}
			if (initializer) free_statement(initializer);
			if (condition) free_expression(condition);
			if (update) free_expression(update);

			if (body) free_statement(body);
			break;
		}

		case NODE_BLOCK: {
			if (node->right) {
				Node* stmt = node->right;
				while (stmt) {
					Node* next_stmt = stmt->next;
					free_statement(stmt);
					stmt = next_stmt;
				}
			}
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			if (node->left) free_expression(node->left);
			break;
		}

		case NODE_BREAK:
		case NODE_CONTINUE: { break;}

		case NODE_ELSE: {
			if (node->right) free_expression(node->right);
			break;
		}

	}
	free(node);
}

void free_globals(Node* node) {
	printf("in free globals\n");
	if (!node || node->node_free) return;

	node->node_free = true;

	switch (node->type) {
		case NODE_NAME: {
			if (node->t) {
				if (node->t->kind == TYPE_FUNCTION) {
					if (node->value.name) {
						free(node->value.name);
						node->value.name = NULL;
					}

					if (node->left) {
						Node* params = node->left;
						while (params) {
							Node* next_params = params->next;
							free_parameters(params);
							params = next_params;
						}
					}

					if (node->right) {
						free_statement(node->right);
					}

					if (node->t) {
						free_type(node->t);
					}
				} else if (node->t->kind == TYPE_ARRAY) {
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
				free_parameters(node->right);
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
    printf("--- Starting AST Deallocation ---\n");
    Node* current = root;
    while (current) {
        Node* next = current->next;
        free_globals(current);
        current = next;
    }
    printf("--- AST Deallocation Complete ---\n");
}