#include "parser.h"

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

	printf("About to create node\n");
	node->type = type;
	node->left = left;
	node->right = right;
	node->prev = prev;
	node->next = next;
	node->t = t;
	node->node_free = false;
	printf("About to return node\n");

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
			printf("IN token id\n");
			Token tok = peek_token(parser);
			char* id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for id in 'parse_factor()'\n");
				return NULL;
			}

			node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				Node* args = parse_args(parser, file);
				node = create_node(NODE_CALL, node, args, NULL, NULL, NULL);
				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					printf("Error: Expected ')' for node call\n");
					return NULL;
				}
				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);
				Node* expr_node = parse_logical_or(parser, file);
				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					printf("Error: Expected ']' in parse_factor\n");
					return NULL;
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
			printf("We have an integer\n");
			Token tok = peek_token(parser);
			int val = tok.value.val;
			printf("int value is: %d\n", val);
			node = create_int_node(NODE_INTEGER, val, NULL, NULL, NULL, NULL, NULL);
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_PARENTHESES: {
			advance_parser(parser);
			node = parse_logical_or(parser, file);
			if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
				printf("IN PARSE_FACTOR, Expected matching ')'\n");
				return NULL;
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_BRACKET: {
			advance_parser(parser);
			node = parse_logical_or(parser, file);
			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				printf("IN PARSE_FACTOR, Expected matching ']'\n");
				return NULL;
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_SINGLE_QUOTE: {
			advance_parser(parser);
			node = parse_logical_or(parser, file);
			if (peek_token_type(parser) != TOKEN_SINGLE_QUOTE) {
				printf("Missing SINGLE QUOTE\n");
				return NULL;
			}
			advance_parser(parser);
			break;
		}
	}
	return node;
}

Node* parse_multiplicative(Parser* parser, FILE* file) {
	Node* node = parse_factor(parser, file);
	while (peek_token_type(parser) == TOKEN_MUL || peek_token_type(parser) == TOKEN_DIV ||
		   peek_token_type(parser) == TOKEN_MUL_EQUAL || peek_token_type(parser) == TOKEN_DIV_EQUAL ||
		   peek_token_type(parser) == TOKEN_MODULO) {

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
	bool special_statement = false;

	switch (peek_token_type(parser)) {
		case TOKEN_LET_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_ID) {
				printf("Error: Do not have an identifier after 'let' keyword\n");
				return NULL;
				//handle non token id after let
			}

			Token tok = peek_token(parser);
			char* id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for id in 'parse_statement()\n");
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

			{
				Token tok = peek_token(parser);
				data_t type = get_type(&tok);
				struct type* t = create_type(type, NULL);
				
				advance_parser(parser);
				
				if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
					advance_parser(parser);
					struct type* array_type = create_type(TYPE_ARRAY, t);
					printf(" JUST MOVED PAST TOKEN_LEFT_BRACKET and Current token type is '%d'\n", peek_token_type(parser));
					Node* expr_node = parse_logical_or(parser, file);
					if (!expr_node) {
						printf("Error: Unable to retrieve size of array\n");
						return NULL;
					}
					printf("IN PARSE_STATEMENT JUST GOT ARRAY SIZE-> CURRENT TOKEN TYPE IS '%d'\n", peek_token_type(parser));
					if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
						printf("Error: Expected ']' after parsing '%s' size\n", id);
						return NULL;
					}

					advance_parser(parser);

					if (peek_token_type(parser) == TOKEN_SEMICOLON) {
						stmt = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);

					} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
						advance_parser(parser); // move past '='
						Node* assignee = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
						Node* elements = parse_array_list(parser, file);

						stmt = create_string_node(NODE_ASSIGNMENT, NULL, assignee, elements, NULL, NULL, NULL);  
						printf("TOKEN TYPE IS NOW: '%d'\n", peek_token_type(parser)); 
						advance_parser(parser);
						printf("NEXT TOKEN TYPE IS NOW: '%d'\n", peek_token_type(parser));
						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							printf("Error: Expected ';' after defining '%s' elements\n", id);
						}
					} 
				} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					stmt = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					Node* assignee = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
					advance_parser(parser);
					Node* expr_node = parse_logical_or(parser, file);
					if (!expr_node) {
						printf("Error: EXPR NODE IN 'parse_let' is null\n");
						return NULL;
					}
					stmt = create_node(NODE_ASSIGNMENT, assignee, expr_node, NULL, NULL, NULL);
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						printf("Error: Expected ';' after assignment for '%s'\n", id);
						return NULL;
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
				Node* node = parse_logical_or(parser, file);
				stmt = create_string_node(NODE_RETURN, NULL, NULL, node, NULL, NULL, NULL);
			}
			break;
		}

		case TOKEN_IF_KEYWORD: {
			special_statement = true;
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				printf("Error: Expected '(' after 'IF STATEMENT'\n");
				return NULL;
			} 
			Node* condition_node = parse_logical_or(parser, file);
			if (!condition_node) {
				printf("Error: received null condition node in if statement\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				printf("Error: Expected '{' after condition for IF statement\n");
				return NULL;
			}

			advance_parser(parser);
			Node* if_body = parse_block(parser, file);
			if (!if_body) {
				printf("Error: received NULL if body\n");
				return NULL;
			}

			stmt = create_node(NODE_IF, condition_node, if_body, NULL, NULL, NULL);
			if (!stmt) {
				printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
				return NULL;
			}
			printf("\033[1;31mCREATED IF STATEMENT with node type: '%d'\033[0m\n", stmt->type);
			return stmt;
		} 

		case TOKEN_ELSE_KEYWORD: {
			special_statement = true;
			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_IF_KEYWORD) {
				advance_parser(parser);
				
				if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
					printf("Error: Expected '(' after else if keyword\n");
					return NULL;
				}			
				Node* condition_node = parse_logical_or(parser, file);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					printf("Error: Expected '{' after else if keyword\n");
					return NULL;
				}		
				advance_parser(parser);
				Node* body = parse_block(parser, file);
				if (!body) {
					printf("Error: body is NULL\n");
					return NULL;
				}

				stmt = create_node(NODE_ELSE_IF, condition_node, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
					return NULL;
				}
				printf("\033[1;31mCreated ELSE IF STATEMENT with node type: '%d'\033[0m\n", stmt->type);
				return stmt;

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACE) {
				special_statement = true;
				advance_parser(parser);

				Node* body = parse_block(parser, file);
				if (!body) {
					printf("Error: recived NULL body in else TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				} 

				stmt = create_node(NODE_ELSE, NULL, body, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Could not create statement NODE_ELSE type in TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				}
				printf("\033[1;31mCREATED ELSE STATEMENT with node type: '%d'\033[0m\n", stmt->type);

				return stmt;
			} else {
				printf("Error: missing '{' after else keyword\n");
				return NULL;
			}
		}

		case TOKEN_FOR_KEYWORD: {
			special_statement = true;
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				printf("Error: Expected '(' after for keyword\n");
				return NULL;
			}
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LET_KEYWORD) {
				Node* initializer = parse_statement(parser, file);
				if (!initializer) {
					printf("Error: Initializer is NULL is for loop\n");
					return NULL;
				}
				advance_parser(parser); // skipping ';'
				Node* condition = parse_logical_or(parser, file);
				if (!condition) {
					printf("Error: Condition in for loop is NULL\n");
					return NULL;
				}

				initializer->next = condition;
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					printf("Expected ';' after for loop condition\n");
					return NULL;
				}

				advance_parser(parser);
				Node* update = parse_logical_or(parser, file);
				if (!update) {
					printf("Error: Update expression in for loop is NULL\n");
					return NULL;
				}
				condition->next = update;

				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					printf("Error: Expected ')' to complete for loop expressions\n");
					return NULL;
				}

				advance_parser(parser);
				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					printf("\032[1;32mError: Expected '{' after for loop expression\032[0m\n");
					return NULL;
				}
				advance_parser(parser);
				Node* loop_body = parse_block(parser, file);

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
							printf("Error: for loop variable may only be an int\n");
							return NULL;
						}

						Token tok = peek_token(parser);
						data_t kind = get_type(&tok);
						struct type* type = create_type(kind, NULL);

						printf("About to create string node\n");
						Node* initializer = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, type);
						if (initializer) {
							printf("Created string node\n");
						}
						advance_parser(parser);

						if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
							printf("Error: need to assign loop variable to value\n");
							return NULL;
						}

						advance_parser(parser);
						Node* expr_node = parse_logical_or(parser, file);
						Node* assignment = create_node(NODE_ASSIGNMENT, initializer, expr_node, NULL, NULL, NULL);
						if (assignment) {
							printf("Made assignment node\n");
						}
						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							printf("Error: Missing ';' after initializer in for loop\n");
							return NULL;
						}

						advance_parser(parser);
						Node* condition = parse_logical_or(parser, file);
						if (condition) {
							assignment->next = condition;
						}
						advance_parser(parser); // skip ';'
					
						Node* update = parse_logical_or(parser, file);
						if (update) {
							condition->next = update;
						}

						if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
							printf("Error: Expected ')' after for loop expressions\n");
							return NULL;
						}

						advance_parser(parser);
						if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
							printf("Error: Expected '{' after before for loop body\n");
							return NULL;
						}
						advance_parser(parser);
						Node* loop_body = parse_block(parser, file);
						if (loop_body) {
							stmt = create_node(NODE_FOR, assignment, loop_body, NULL, NULL, NULL);
						}
					}
					free(id);
					return stmt;

				} else {
					Node* initializer = parse_statement(parser, file);
					if (initializer) {
						printf("\033[1;31mAFTER initializer -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					advance_parser(parser); // ';'

					Node* condition = parse_logical_or(parser, file);
					if (condition) {
						printf("\033[1;31mAFTER condition -> Current token type is: '%d'\033[0m\n", peek_token_type(parser));
					}
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						printf("Expected ';' after for loop condition\n");
						return NULL;
					}
					advance_parser(parser);
					Node* update = parse_logical_or(parser, file);
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
					Node* loop_body = parse_block(parser, file);
					stmt = create_node(NODE_FOR, initializer, loop_body, NULL, NULL, NULL);
					return stmt;

				}
			}
		}
		case TOKEN_WHILE_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				printf("Error: Expected '(' after while keyword\n");
				return NULL;
			}

			Node* condition_node = parse_logical_or(parser, file);
			if (!condition_node) {
				printf("Error: Received null condition in while \n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				printf("Error: Expected '{' after condition\n");
				return NULL;
			}

			advance_parser(parser);
			Node* while_body = parse_block(parser, file);

			stmt = create_node(NODE_WHILE, condition_node, while_body, NULL, NULL, NULL);
			return stmt;
		}

		case TOKEN_ID: {
			Token tok = peek_token(parser);
			char* id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to aalocate space for id\n");
				return NULL;
			}
			Node* node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, NULL);

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				printf("We have subscript\n");
				Node* index_node = parse_logical_or(parser, file);
				if (index_node) {
					printf("got index node\n");
				}
				node = create_node(NODE_SUBSCRIPT, node, index_node, NULL, NULL, NULL);
				if (node) {
					printf("created subscript node\n");
				}
				if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
					printf("Error: Expected '=' in for array statement\n");
					return NULL;
				}
				advance_parser(parser);
				Node* expr_node = parse_logical_or(parser, file);
				stmt = create_node(NODE_ASSIGNMENT, node, expr_node, NULL, NULL, NULL);

			} else if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				printf("We have func call\n");
				advance_parser(parser);
				Node* args = parse_args(parser, file);
				stmt =  create_node(NODE_CALL, node, args, NULL, NULL, NULL);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* expr = parse_logical_or(parser, file);
				if (!expr) {
					printf("Received null expr\n");
					return NULL;
				}
				stmt = create_node(NODE_ASSIGNMENT, node, expr, NULL, NULL, NULL);
			}
			free(id);
			break;
		}
	}

	if (!special_statement) {
		if (peek_token_type(parser) != TOKEN_SEMICOLON) {
			printf("Error: Statement should end with a semicolon but it does not\n");
			printf("Statement ends with token type: %d\n", peek_token_type(parser));
			return NULL;
		}		
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

			if (current->type == NODE_ELSE_IF || current->type == NODE_ELSE) {
				if (!current->prev || (current->prev && (current->prev->type != NODE_IF && current->prev->type != NODE_ELSE_IF))) {
					printf("Error: else without matching if\n");
					return NULL;
				}
			}
		} else {
			printf("Error: Received NULL statement from 'parse_statement()'\n");
			return NULL;
		}


		printf("Now im here\n");

		if (peek_token_type(parser) == TOKEN_SEMICOLON) { advance_parser(parser); }
	
		if (peek_token_type(parser) == TOKEN_RIGHT_BRACE) { break; }
	}

	// skip '}'
	printf("About to reutrn block, current token type is: '%d'\n", peek_token_type(parser));
	advance_parser(parser);
	printf("About to return block\n");
	return head;
}

Node* parse_parameters(Parser* parser, FILE* file) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		char* id = strdup((*(parser->end)).value.str);
		if (!id) {
			printf("Error: Unable to allocate space for parameter id\n");
			return NULL;
		}

		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_COLON) {
			printf("Error: Missing ':' for '%s'\n", id);
			return NULL;
		}
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
			// deal with not having any data type
			printf("Error: expected data type after colon\n");
			return NULL;
		}

		Token tok = peek_token(parser);
		data_t param_type = get_type(&tok);
		printf("IN PARSE PARAMETERS -> TOKEN TYPE for '%s' is '%d'\n", id, param_type);
		struct type* t = create_type(param_type, NULL);
		advance_parser(parser);


		node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
		if (node) {
			printf("Created node with node type: '%d'-> name: '%s'\n\n", NODE_NAME, id);
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

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		} else if (peek_token_type(parser) == TOKEN_RIGHT_PARENTHESES) {
			break;
		} else {
			printf("Error: missing comma in parameters\n");
			return NULL;
		}

	}
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
		printf("Error: Unable to allocate space for buffer in 'emit_err_msg()'\n");
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
			printf("Error: Unable to allocate space for function identifier\n");
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
		
		printf("After returning parameters, current token type is: %d\n", peek_token_type(parser));
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_ARROW) {
			printf("Error: Missing '->' after parameters for '%s'\n", id);
			return NULL;
		} else {
			printf("You correctly have '->' after parameters for '%s'\n", id);
		}

		advance_parser(parser);
		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
			peek_token_type(parser) != TOKEN_VOID_KEYWORD) {
			
			printf("Errror: You have not specified return type for '%s'\n", id);
			return NULL;
			// deal with not having any return data type
		}
		Token tok = peek_token(parser);
		data_t return_type = get_type(&tok);
		struct type* subtype = create_type(return_type, NULL);
		struct type* t = create_type(TYPE_FUNCTION, subtype);
		printf("IN FUNCTION '%s' -> '%s' has return type: '%d'\n", id, id, return_type);
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
			printf("Error: missing '{' after return type for '%s'\n", id);
			return NULL;
			// deal with missing '{'
		}

		advance_parser(parser);
		Node* body = parse_block(parser, file);
		if (body) {
			printf("Got block\n");
		}
		function_node = create_string_node(NODE_NAME, id, params, body, NULL, NULL, t);

		if (!function_node) {
			printf("Error: Unable to create function node\n");
		}
		free(id);
		printf("About to return function node\n");
		return function_node;

	}
	// advance_parser(parser);

	return function_node;
}

Node* parse_args(Parser* parser, FILE* file) {
	Node* head = NULL;
	Node* current = NULL;
	Node* arg = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		arg = parse_logical_or(parser, file);
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

Node* parse_array_list(Parser* parser, FILE* file) {
	Node* head = NULL;
	Node* current = NULL;
	Node* array_element = NULL;

	advance_parser(parser);

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		array_element = parse_logical_or(parser, file);

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

Node* parse_let(Parser* parser, FILE* file) {
	Node* let_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) == TOKEN_ID) {
		char* id = NULL;
		{
			Token tok = peek_token(parser);
		 	id = strdup(tok.value.str);
			if (!id) {
				printf("Error: Unable to allocate space for id in 'parse_let()'\n");
				return NULL;
			}
			printf("In 'parse_let()', ID -> '%s'\n", id);
		}

		advance_parser(parser);
		if (peek_token_type(parser) != TOKEN_COLON) {
			printf("Error: Expected ':' after '%s'\n", id);
			return NULL;
		}

		advance_parser(parser);
		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
			
			printf("Error: Expected data type after ':' for '%s'\n", id);
			return NULL;
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

				Node* expr_node = parse_logical_or(parser, file);
				if (!expr_node) {
					printf("Error: Unable to retrieve size of array\n");
					return NULL;
				}
				printf("Got array size and currrent token TYPE is '%d'\n", peek_token_type(parser));
				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					printf("Error: Expected ']' after parsing '%s' size\n", id);
					return NULL;
				}

				advance_parser(parser);

				if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					let_node = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
					advance_parser(parser);

				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					printf("CURRRENT TOKEN TYPE is '%d'\n", peek_token_type(parser));
					advance_parser(parser); // move past '='
					Node* assignee = create_string_node(NODE_NAME, id, expr_node, NULL, NULL, NULL, array_type);
					Node* elements = parse_array_list(parser, file);

					let_node = create_string_node(NODE_ASSIGNMENT, NULL, assignee, elements, NULL, NULL, NULL);   
					advance_parser(parser);
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						printf("Error: Expected ';' after defininng '%s' elements\n", id);
					}
					advance_parser(parser);
				} 
			} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				let_node = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				advance_parser(parser);
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* assignee = create_string_node(NODE_NAME, id, NULL, NULL, NULL, NULL, t);
				Node* expr_node = parse_logical_or(parser, file);
				if (!expr_node) {
					printf("Error: EXPR NODE IN 'parse_let' is null\n");
					return NULL;
				}
				let_node = create_string_node(NODE_ASSIGNMENT, NULL, assignee, expr_node, NULL, NULL, NULL);
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					printf("Error: Expected ';' after assignment for '%s'\n", id);
					return NULL;
				}
				advance_parser(parser);
			}
			free(id);
		}
		

		return let_node;
	}
	
}

Node* parse(Token* tokens, FILE* file) {
	printf("In parser\n");
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

		if (peek_token_type(parser) == TOKEN_FUNCTION_KEYWORD) {
			node = parse_function(parser, file);
			
		}  else if (peek_token_type(parser) == TOKEN_LET_KEYWORD) {
			node = parse_let(parser, file);
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
			exit(EXIT_FAILURE);
		}

	}

	free(parser);
	printf("About to return nodes\n");
	return head;

}

#define BRANCH "├"
#define LAST_BRANCH "└"
#define CONTINUATION "│"

// void print_ast(Node* root) {

// }

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