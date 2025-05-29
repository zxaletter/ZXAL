#include "parser.h"


Parser* initialize_parser(Token* tokens) {
	Parser* parser = malloc(sizeof(Parser));
	if (!parser) {
		perror("Error: Unable to allocate space for parser\n");
		return NULL;
	}

	parser->size = 0;
	parser->capacity = NUM_NODES;
	parser->end = tokens;
	return parser;

}

token_t peek_token_type(Parser* parser) {
	Token tok = peek_token(parser);
	return tok.type;
}

Token peek_token(Parser* parser) {
	return *parser->end;
}

token_t peek_next_token_type(Parser* parser) {
	Token tok = *(parser->end++);
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
	data_t kind = TYPE_UNKNOWN;

	switch (token->type) {
		case TOKEN_INT_KEYWORD: kind = TYPE_INTEGER; break;
		case TOKEN_CHAR_KEYWORD: kind = TYPE_CHAR; break;
	}

	return kind;
}

struct type* create_type(data_t main_type, struct type* subtype) {
	struct type* t = malloc(sizeof(struct type));
	if (!t) {
		perror("Error: Unable to allocate space for type\n");
		return NULL;
	}
	t->kind = main_type;

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
		perror("Error: Unable to allocate space for node\n");
		return NULL;
	}
	node->type = type;
	node->left = left;
	node->right = right;
	node->prev = prev;
	node->next = next;
	node->t = t;

	return node;

}

Node* create_int_node(node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {

	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		perror("Error: In 'create_int_node()': NULL '<-' from 'create_node()'\n");
		return NULL;
	}
	node->value.val = val;

	return node;
}

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t) {
	
	Node* node = create_node(type, left, right, prev, next, t);
	if (!node) {
		perror("Error: In 'create_string_node()': NULL '<-' from 'create_node()'\n");
		return NULL;
	}

	if (id) {
		node->value.name = strdup(id);
		if (!node->value.name) {
			perror("Error: Unable to allocate space for string in 'create_string_node()'\n");
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

Node* parse_factor(Parser* parser, FILE* file) {
	Node* node = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_ID: {
			char* id = strdup((*(parser->end)).value.str);
			if (!id) {
				perror("Error: Unable to allocate space for id in 'parse_factor()'\n");
				return NULL;
			}

			Token tok = peek_token(parser);
			data_t id_type = get_type(&tok);
			struct type* t = create_type(id_type, TYPE_UNKNOWN); 
			node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
			advance_parser(parser);
			break;
		}

		case TOKEN_INTEGER: {
			printf("We have an integer\n");
			Token tok = peek_token(parser);
			int val = tok.value.val;
			printf("int value is: %d\n", val);
			node = create_int_node(NODE_INTEGER, val, NULL, NULL, NULL, NULL, NULL);
			advance_parser(parser);
			break;
		}


	}
	return node;
}


Node* parse_multiplicative(Parser* parser, FILE* file) {
	Node* node = parse_factor(parser, file);

	while (peek_token_type(parser) == TOKEN_MUL || peek_token_type(parser) == TOKEN_DIV ||
		   peek_token_type(parser) == TOKEN_MUL_EQUAL || peek_token_type(parser) == TOKEN_DIV_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_factor(parser, file);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_additive(Parser* parser, FILE* file) {
	Node* node = parse_multiplicative(parser, file);

	while (peek_token_type(parser) == TOKEN_ADD || peek_token_type(parser) == TOKEN_SUB ||
		   peek_token_type(parser) == TOKEN_ADD_EQUAL || peek_token_type(parser) == TOKEN_SUB_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_multiplicative(parser, file);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_relational(Parser* parser, FILE* file) {
	Node* node = parse_additive(parser, file);

	while (peek_token_type(parser) == TOKEN_LESS || peek_token_type(parser) == TOKEN_GREATER ||
		   peek_token_type(parser) == TOKEN_LESS_EQUAL || peek_token_type(parser) == TOKEN_GREATER_EQUAL ||
		   peek_token_type(parser) == TOKEN_EQUAL || peek_token_type(parser) == TOKEN_NOT_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_additive(parser, file);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_logical_and(Parser* parser, FILE* file) {
	Node* node = parse_relational(parser, file);

	while (peek_token_type(parser) == TOKEN_LOGICAL_AND) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_relational(parser, file);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_or(Parser* parser, FILE* file) {
	Node* node = parse_logical_and(parser, file);

	while (peek_token_type(parser) == TOKEN_LOGICAL_OR) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_logical_and(parser, file);
		node = create_node(op_kind, node, right_child, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_statement(Parser* parser, FILE* file) {
	Node* stmt = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_LET_KEYWORD: {
			advance_parser(parser);
			// if (peek_token_type(parser) != TOKEN_ID) {
			// 	//handle non token id after let
			// }
			char* id = strdup((*(parser->end)).value.str);
			if (!id) {
				perror("Error: Unable to allocate space for id in 'parse_statement()\n");
				return NULL;
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_COLON) {
				printf("Error: Unable to find ':' after '%s'\n", id);
				return NULL;
				// handle non token colon after variable name
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_INT_KEYWORD &&
				peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
				peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
				// handle no appropriate type for variable
			
				printf("Error: Unable to find data type after '%s'\n", id);
				return NULL;
			}

			Token tok = peek_token(parser);
			data_t type = get_type(&tok);
			struct type* var_type = create_type(type, NULL);

			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, var_type);
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* assignee = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
			 	Node* expr_node = parse_logical_or(parser, file);
				stmt = create_string_node(NODE_ASSIGNMENT, NULL, assignee, expr_node, NULL, NULL, NULL);
			} 
			free(id);
			break;
		}

		case TOKEN_RETURN_KEYWORD: {
			printf("IN PARSE STATEMENT WITH RETURN CASE\n");
			advance_parser(parser);
			printf("About to check token type\n");
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(NODE_RETURN, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {
				printf("We know we have expression after return\n");
				Node* node = parse_logical_or(parser, file);
				printf("Got expression\n");
				stmt = create_string_node(NODE_RETURN, NULL, NULL, node, NULL, NULL, NULL);
				printf("Got statement\n");
			}
			break;
		}
	}

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		perror("Error: Statement should end with a semicolon but it does not\n");
		printf("Statement ends with token type: %d\n", peek_token_type(parser));
		return NULL;
	}
	printf("Returning the statement\n");
	return stmt;
}

Node* parse_block(Parser* parser, FILE* file) {
	printf("IN PARSE_BLOCK\n");

	Node* head = NULL;
	Node* current = NULL;
	Node* stmt = NULL; 

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		printf("About to invoke 'parse_statement()'\n");
		stmt = parse_statement(parser, file);
		if (stmt) {
			printf("Got statement\n");
		}

		if (stmt) {
			if (!head) {
				head = stmt;
				current = stmt;
			} else {
				current->next = stmt;
				stmt->prev = current;
				current = stmt;
			}
		}
		printf("Now im here\n");
		// skip ';'
		advance_parser(parser);
	
		if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) { break; }
	}

	// skip '}'
	// advance_parser(parser);
	printf("About to return block\n");
	return head;
}

Node* parse_parameters(Parser* parser, FILE* file) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;
	printf("About to get parameters\n");
	printf("And token type is: %d\n\n", peek_token_type(parser));
	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		// Current token is TOKEN_COLON ??
		printf("Current token type is: %d\n", peek_token_type(parser));
		char* id = strdup((*(parser->end)).value.str);
		if (!id) {
			perror("Error: Unable to allocate space for parameter id\n");
			return NULL;
		}
		printf("Got '%s'\n", id);

		advance_parser(parser);
		printf("After id, next token type is: %d\n", peek_token_type(parser));

		if (peek_token_type(parser) != TOKEN_COLON) {
			printf("Error: Missing ':' for '%s'\n", id);
			return NULL;
		}
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
			// deal with not having any data type
			perror("Error: expected data type after colon\n");
			return NULL;
		}

		Token tok = peek_token(parser);
		data_t param_type = get_type(&tok);
		struct type* t = create_type(param_type, NULL);
		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		} else if (peek_next_token_type(parser) == TOKEN_RIGHT_PARENTHESES) {
			break;
		} else {
			perror("error: missing comma in parameters\n");
			return NULL;
		}

		node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
		if (node) {
			printf("Created node with type: '%d'-> name: '%s'\n\n", NODE_NAME, id);
		}
		free(id);

		if (node) {
			if (!head) {
				head = node;
				current = node;
			} else {
				current->next = node;
				node->prev = current;
				current = node;
			}
		}

		// advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_RIGHT_PARENTHESES) { break; }
	}

	// advance_parser(parser);
	printf("Returning PARAMETERS\n");

	return head;
}

Error create_error(error_t type) {
	Error err = {
		.type = type,
		.msg = NULL,
		.line = NULL
	};
	return err;
}

void emit_err_msg(Parser* parser, FILE* file, Error* err) {
	Token tok = peek_token(parser);
	int target_line = tok.line;

	fseek(file, 0, SEEK_SET);

	int current_line = 1;
	char ch;
	while (current_line < target_line && (ch = fgetc(file)) != EOF) {
		if (ch == '\n') {
			current_line++;
		}
	}

	int line_size = 0;
	while ((ch = fgetc(file)) != '\n') {
		line_size++;
	}

	char* buffer = malloc(line_size + 1);
	if (!buffer) {
		perror("Error: Unable to allocate space for buffer in 'emit_err_msg()'\n");
		return;
	}

	fgets(buffer, line_size, file);
	buffer[line_size] = '\0';

	printf("Line Where the error occurred is '%s'\n", buffer);


	fseek(file, 0, SEEK_SET);

	free(buffer);
}

Node* parse_function(Parser* parser, FILE* file) {
	Node* function_node = NULL;
	advance_parser(parser);

	printf("CURRENT token type is: %d\n", peek_token_type(parser));
	if (peek_token_type(parser) == TOKEN_ID) {
		char* id = strdup((*(parser->end)).value.str);
		if (!id) {
			perror("Error: Unable to allocate space for function identifier\n");
			return NULL;
		}
		printf("Current function: '%s'\n", id);

		advance_parser(parser);
		printf("After current function we have token type: %d\n\n", peek_token_type(parser));
		if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
			Error err = create_error(SYNTAX_ERROR);
			emit_err_msg(parser, file, &err);	
		}

		advance_parser(parser);
		printf("Before parsing parameters next token type is: %d\n\n", peek_token_type(parser));
		Node* params = parse_parameters(parser, file);
		if (params) {
			Node* curr = params;
			while (curr) {
				Node* next = curr->next;
				printf("Parameter: '%s'\n", curr->value.name);
				curr = next;
			}
		}
		
		printf("After returning parameters, current token type is: %d\n", peek_token_type(parser));
		advance_parser(parser);
		if (peek_token_type(parser) != TOKEN_ARROW) {
			printf("Error: Missing '->' after parameters for '%s'\n", id);
		}
		printf("You correctly have '->' after parameters for '%s'\n", id);

		advance_parser(parser);
		// if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
		// 	peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		// 	peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
		// 	// deal with not having any return data type
		// }
		Token tok = peek_token(parser);
		data_t return_type = get_type(&tok);
		struct type* subtype = create_type(return_type, NULL);
		struct type* t = create_type(TYPE_FUNCTION, subtype);
		advance_parser(parser);

		// if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		// 	// deal with missing '{'
		// }
		advance_parser(parser);
		Node* body = parse_block(parser, file);
		if (body) {
			printf("Got block\n");
		}
		function_node = create_string_node(NODE_NAME, id, params, body, NULL, NULL, t);

		if (!function_node) {
			perror("Error: Unable to create function node\n");
		}
		free(id);
		printf("About to return function node\n");
		return function_node;

	}

	return function_node;
}

Node* parse(Token* tokens, FILE* file) {
	printf("In parser\n");
	Parser* parser = initialize_parser(tokens);
	if (!parser) {
		perror("In 'build_AST', parser is NULL\n");
		return NULL;
	}

	Node* head = NULL;
	Node* current = NULL;
	Node* node = NULL;

	while (!at_token_eof(parser)) {

		if (at_token_eof(parser)) { break; }

		if (peek_token_type(parser) == TOKEN_FUNCTION_KEYWORD) {
			node = parse_function(parser, file);
			if (node) {
				printf("Got function node\n");
			}
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
		}

		advance_parser(parser);
	}

	free(parser);
	printf("About to return nodes\n");
	return head;
}

void free_type(struct type* t) {
	if (!t) return;

	if (t->subtype) {
		printf("here with subtype\n");
		free_type(t->subtype);
		printf("freed subtype\n");

	}
	
	free(t);
}


void free_node(Node* node) {
	if (!node) return;

	if (node->value.name) {
		printf("here with name\n");
		free(node->value.name);
	}

	if (node->left) {
		printf("here in left\n");
		free_node(node->left);
	}

	if (node->right) {
		printf("here in right\n");
		free_node(node->right);
	}

	if (node->t) {
		printf("here in type\n");
		free_type(node->t);
	}

	free(node);
}

void free_ast(Node* root) {
	if (!root) return;
	printf("About to free ast\n");
	Node* curr = root;
	while (curr) {
		Node* next = curr->next;
		if (curr->value.name) {
			printf("About to free '%s'\n", curr->value.name);
		}
		free_node(curr);
		printf("Freed '%s'\n", curr->value.name);
		curr = next;
	}
}