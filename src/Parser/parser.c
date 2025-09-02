#include "parser.h"
#include "compilercontext.h"
#include "errors.h"

bool validate_token(CompilerContext* ctx, Parser* parser, token_t target_type) {
	Token tok = peek_token(parser);
	
	if (tok.type != target_type) {
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_SEMICOLON,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
		return false;
	}
	return true;
}

bool have_valid_statement(CompilerContext* ctx, Parser* parser, Node* stmt) {
	if (!stmt) return false;

	bool needs_semicolon = false;

	switch (stmt->type) {
		case NODE_IF:
		case NODE_ELSE_IF:
		case NODE_ELSE:
		case NODE_FOR:
		case NODE_WHILE:
			break;

		default:
			needs_semicolon = true;
			break;
	}
	
	if (needs_semicolon) {
		return validate_token(ctx, parser, TOKEN_SEMICOLON);
	} 
	return true;
}

Token* copy_token(CompilerContext* ctx, Token* original_token) {
	if (!original_token) return NULL;

	Token* copy_token = arena_allocate(ctx->lexer_arena, sizeof(Token));
	if (!copy_token) {
		perror("In 'copy_token', unable to allocate space for copy token\n");
		return NULL;
	}

	copy_token->type = original_token->type;
	copy_token->line = original_token->line;
	copy_token->column = original_token->column;
	switch (original_token->type) {
		case TOKEN_ARROW:
		case TOKEN_ADD_EQUAL:
		case TOKEN_SUB_EQUAL:
		case TOKEN_MUL_EQUAL:
		case TOKEN_DIV_EQUAL:
		case TOKEN_LESS_EQUAL:
		case TOKEN_GREATER_EQUAL:
		case TOKEN_NOT_EQUAL:
		case TOKEN_EQUAL:
		case TOKEN_DECREMENT:
		case TOKEN_INCREMENT:
		case TOKEN_INT_KEYWORD:
		case TOKEN_CHAR_KEYWORD:
		case TOKEN_BOOL_KEYWORD:
		case TOKEN_STR_KEYWORD:
		case TOKEN_VOID_KEYWORD:
		case TOKEN_STRUCT_KEYWORD:
		case TOKEN_ENUM_KEYWORD:
		case TOKEN_FOR_KEYWORD:
		case TOKEN_WHILE_KEYWORD:
		case TOKEN_CONTINUE_KEYWORD:
		case TOKEN_BREAK_KEYWORD:
		case TOKEN_FUNCTION_KEYWORD:
		case TOKEN_LET_KEYWORD:
		case TOKEN_ID:
		case TOKEN_LOGICAL_OR:
		case TOKEN_LOGICAL_AND: {
			if (original_token->value.str) {
				int length = strlen(original_token->value.str);
				copy_token->value.str = arena_allocate(ctx->lexer_arena, length + 1);
				if (!copy_token->value.str) return NULL;
				strncpy(copy_token->value.str, original_token->value.str, length);
				copy_token->value.str[length] = '\0';
			}
			break;
		}

		case TOKEN_INTEGER: {
			copy_token->value.val = original_token->value.val;
			break;
		}

		case TOKEN_CHAR_LITERAL: {
			copy_token->value.c = original_token->value.c;
			break;
		}
	}
	return copy_token;
}

token_t peek_token_type(Parser* parser) {
	Token tok = peek_token(parser);
	return tok.type;
}

Token peek_token(Parser* parser) {
	return *parser->tokens;
}

token_t peek_next_token_type(Parser* parser) {
	parser->tokens++;
	Token tok = *parser->tokens;
	parser->tokens--;
	return tok.type;
}

Token advance_parser(Parser* parser) {
	Token token = *parser->tokens;
	parser->tokens++; 
	return token;
}

bool at_token_eof(Parser* parser) {
	return (peek_token_type(parser) == TOKEN_EOF);
}

TypeKind get_type(Token* token) {
	switch (token->type) {
		case TOKEN_INT_KEYWORD: return TYPE_INTEGER;
		case TOKEN_CHAR_KEYWORD: return TYPE_CHAR;
		case TOKEN_BOOL_KEYWORD: return TYPE_BOOL;
		case TOKEN_STRUCT_KEYWORD: return TYPE_STRUCT;
		case TOKEN_VOID_KEYWORD: return TYPE_VOID;
		default: return TYPE_UNKNOWN;		
	}
}

Node* create_node(CompilerContext* ctx, node_t type, 
	Node* left, Node* right, Node* prev, 
	Node* next, Node* params, struct Type* t) {

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
	node->params = params;
	node->t = t;
	node->symbol = NULL;

	return node;
}

Node* create_int_node(CompilerContext* ctx, node_t type, int val, 
	Node* left, Node* right, Node* prev, 
	Node* next, Node* params, struct Type* t) {

	Node* node = create_node(ctx, type, left, right, prev, next, params, t);
	if (!node) return NULL;
	node->value.val = val;
	return node;
}

Node* create_string_node(CompilerContext* ctx, node_t type, char* id, 
	Node* left, Node* right, Node* prev, 
	Node* next, Node* params, struct Type* t) {

	Node* node = create_node(ctx, type, left, right, prev, next, params, t);
	if (!node) return NULL;
	node->value.name = NULL;

	if (id) {
		int length = strlen(id);
		node->value.name = arena_allocate(ctx->ast_arena, length + 1);
		if (!node->value.name) return NULL;
		strncpy(node->value.name, id, length);
		id[length] = '\0';
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
		case TOKEN_EQUAL: return NODE_EQUAL;
		case TOKEN_NOT_EQUAL: return NODE_NOT_EQUAL;
		case TOKEN_INCREMENT: return NODE_INCREMENT;
		case TOKEN_DECREMENT: return NODE_DECREMENT;
		case TOKEN_LOGICAL_AND: return NODE_LOGICAL_AND;
		case TOKEN_LOGICAL_OR: return NODE_LOGICAL_OR;
		default: return NODE_UNKNOWN;
	}
}

Node* parse_factor(CompilerContext* ctx, Parser* parser) {
	Node* node = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_ID: {
			char* id = NULL;
			{
				Token tok = peek_token(parser);
				int length = strlen(tok.value.str);
				id = arena_allocate(ctx->ast_arena, length + 1);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_factor'\n");
					return NULL;
				}
				strncpy(id, tok.value.str, length);
				id[length] = '\0';
			}

			Node* identifier_node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!identifier_node) {
				printf("Unable to create node with name '%s' in 'parse_factor'.\n", id);
				return NULL;
			}

			node = identifier_node;
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				advance_parser(parser);
				
				Node* args = parse_args(ctx, parser);
				
				Node* call_node = create_node(ctx, NODE_CALL, identifier_node, NULL, NULL, NULL, args, NULL);
				if (!call_node) {
					// printf("In parse_factor, unable to create node with type NODE_CALL.\n");
					return NULL;
				} 

				node = call_node;
				
				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_RIGHT_PARENTHESES,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
					// log_error(ctx, &tok, parser->info, EXPECTED_RIGHT_PARENTHESES);
				}

				advance_parser(parser);
				break;
			
			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(ctx, parser);
				if (!expr_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL expression node.\n");
					return NULL;
				}

				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_RIGHT_BRACKET,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				Node* subscript_node = create_node(ctx, NODE_SUBSCRIPT, identifier_node, expr_node, NULL, NULL, NULL, NULL);
				if (!subscript_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL subscript node.\n");
					return NULL;
				}

				node = subscript_node;

				advance_parser(parser);
				break;

			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				Node* increment_node = create_node(ctx, NODE_INCREMENT, identifier_node, NULL, NULL, NULL, NULL, NULL);
				if (!increment_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL increment node.\n");					
					return NULL;
				}

				node = increment_node;
				advance_parser(parser);
				break;

			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				Node* decrement_node = create_node(ctx, NODE_DECREMENT, identifier_node, NULL, NULL, NULL, NULL, NULL);
				if (!decrement_node) {
					printf("In case 'TOKEN_ID' in 'parse_factor', received NULL decrement node.\n");
					return NULL;
				}

				node = decrement_node; 
				advance_parser(parser);
				break;
			} 

			break;
		}

		case TOKEN_INTEGER: {
			Token tok = peek_token(parser);
			int val = tok.value.val;
			struct Type* t = type_create(ctx, TYPE_INTEGER, NULL);
			if (!t) {
				printf("In case 'TOKEN_INTEGER' in 'parse_factor', unable to create type with kind %d\n", TYPE_INTEGER);
				return NULL;
			}

			Node* integer_node = create_int_node(ctx, NODE_INTEGER, val, NULL, NULL, NULL, NULL, NULL, t);
			if (!integer_node) {
				printf("In case 'TOKEN_INTEGER' in 'parse_factor', received NULL integer node.\n");
				return NULL;
			}

			node = integer_node;
			advance_parser(parser);
			break;
		}

		case TOKEN_CHAR_LITERAL: {
			Token tok = peek_token(parser);
			char ch = tok.value.c;
			struct Type* t = type_create(ctx, TYPE_CHAR, NULL);
			if (!t) {
				printf("In case 'TOKEN_CHAR_LITERAL' in 'parse_factor', unable to create type with kind %d\n", TYPE_CHAR);
				return NULL;
			}

			Node* character_node = create_int_node(ctx, NODE_CHAR, ch, NULL, NULL, NULL, NULL, NULL, t);
			if (!character_node) {
				printf("In case 'TOKEN_CHAR_LITERAL' in 'parse_factor', received NULL CHAR LITERAL node.\n");
				return NULL;
			}
			
			node = character_node;
			advance_parser(parser);
			break;
		}

		case TOKEN_TRUE_KEYWORD: {
			Node* bool_true_node = create_int_node(ctx, NODE_BOOL, 1, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!bool_true_node) {
				printf("In case 'TOKEN_TRUE_KEYWORD' in 'parse_factor', received NULL BOOL node.\n");
				return NULL;
			}

			node = bool_true_node;
			advance_parser(parser);
			break;
		}

		case TOKEN_FALSE_KEYWORD: {
			Node* bool_false_node = create_int_node(ctx, NODE_BOOL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
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
			node = parse_logical_or(ctx, parser);
			if (!node) {
				printf("In case 'TOKEN_LEFT_PARENTHESES' in 'parse_factor', received NULL node after invoking 'parse_logical_or'.\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_RIGHT_PARENTHESES,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}
			advance_parser(parser);
			break;
		}

		case TOKEN_LEFT_BRACKET: {
			advance_parser(parser);
			node = parse_logical_or(ctx, parser);
			if (!node) {
				printf("In case 'TOKEN_LEFT_BRACKET' in 'parse_factor', received NULL node after invoking 'parse_logical_or'.\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_RIGHT_BRACKET,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}
			advance_parser(parser);
			break;
		}
	}
	return node;
}

Node* parse_unary(CompilerContext* ctx, Parser* parser) {
	Node* unary_op_node = NULL;
	Node* head = NULL;
	Node* current = NULL;

	while (peek_token_type(parser) == TOKEN_NOT || peek_token_type(parser) == TOKEN_SUB || peek_token_type(parser) == TOKEN_ADD) {
		Token tok = peek_token(parser);

		node_t op_kind = NODE_UNKNOWN;
		switch (peek_token_type(parser)) {
			case TOKEN_NOT: op_kind = NODE_NOT; break;
			case TOKEN_ADD: op_kind = NODE_UNARY_ADD; break;
			case TOKEN_SUB: op_kind = NODE_UNARY_SUB; break;
		}

		unary_op_node = create_node(ctx, op_kind, NULL, NULL, NULL, NULL, NULL, NULL);
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
		Node* next_factor = parse_factor(ctx, parser);
		if (!next_factor) {
			return NULL;
		}
		return next_factor;
	}

	if (current) {
		current->right = parse_factor(ctx, parser);
		return head;
	}
}

Node* parse_multiplicative(CompilerContext* ctx, Parser* parser) {
	Node* node = parse_unary(ctx, parser);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_MUL || peek_token_type(parser) == TOKEN_DIV ||
		   peek_token_type(parser) == TOKEN_MUL_EQUAL || peek_token_type(parser) == TOKEN_DIV_EQUAL ||
		   peek_token_type(parser) == TOKEN_MODULO) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_unary(ctx, parser);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL, NULL);
	}
	return node;
}

Node* parse_additive(CompilerContext* ctx, Parser* parser) {
	Node* node = parse_multiplicative(ctx, parser);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_ADD || peek_token_type(parser) == TOKEN_SUB ||
		   peek_token_type(parser) == TOKEN_ADD_EQUAL || peek_token_type(parser) == TOKEN_SUB_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_multiplicative(ctx, parser);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_relational(CompilerContext* ctx, Parser* parser) {
	Node* node = parse_additive(ctx, parser);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LESS || peek_token_type(parser) == TOKEN_GREATER ||
		   peek_token_type(parser) == TOKEN_LESS_EQUAL || peek_token_type(parser) == TOKEN_GREATER_EQUAL ||
		   peek_token_type(parser) == TOKEN_EQUAL || peek_token_type(parser) == TOKEN_NOT_EQUAL) {

		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_additive(ctx, parser);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_and(CompilerContext* ctx,Parser* parser) {
	Node* node = parse_relational(ctx, parser);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LOGICAL_AND) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_relational(ctx, parser);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_logical_or(CompilerContext* ctx, Parser* parser) {
	Node* node = parse_logical_and(ctx,parser);
	if (!node) return NULL;

	while (peek_token_type(parser) == TOKEN_LOGICAL_OR) {
		Token tok = peek_token(parser);
		node_t op_kind = get_op_kind(&tok);
		advance_parser(parser);
		Node* right_child = parse_logical_and(ctx, parser);
		node = create_node(ctx, op_kind, node, right_child, NULL, NULL, NULL, NULL);
	}

	return node;
}

Node* parse_statement(CompilerContext* ctx, Parser* parser) {
	Node* stmt = NULL;

	switch (peek_token_type(parser)) {
		case TOKEN_LET_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_ID) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_IDENTIFIER,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
				// log_error(ctx, &tok, parser->info, EXPECTED_IDENTIFIER);
				return NULL;
			}

			char* id = NULL;
			{
				Token tok = peek_token(parser);
				int length = strlen(tok.value.str);
				id = arena_allocate(ctx->ast_arena, length + 1);
				if (!id) {
					printf("Error: Unable to allocate space for id in 'parse_statement()\n");
					return NULL;
				}
				strncpy(id, tok.value.str, length);
				id[length] = '\0';
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_COLON) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_COLON,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
				// log_error(ctx, &tok, parser->info, EXPECTED_COLON);
				return NULL;
			}

			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_INT_KEYWORD &&
				peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
				peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
				peek_token_type(parser) != TOKEN_STR_KEYWORD &&
				peek_token_type(parser) != TOKEN_STRUCT_KEYWORD) {

				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_DATATYPE,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_DATATYPE);	
				return NULL;
			}

			Token tok = peek_token(parser);
			TypeKind type = get_type(&tok);
			struct Type* t = type_create(ctx, type, NULL);
			if (!t) {
				return NULL;
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				advance_parser(parser);

				struct Type* array_type = type_create(ctx, TYPE_ARRAY, t);
				if (!array_type) return NULL;

				Node* expr_node = parse_logical_or(ctx, parser);
				if (!expr_node) return NULL;

				if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
					Token token = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_RIGHT_BRACKET,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);

					// log_error(ctx, &token, parser->info, EXPECTED_RIGHT_BRACKET);					
				}

				advance_parser(parser);
				if (peek_token_type(parser) == TOKEN_SEMICOLON) {
					Node* decl = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, NULL, array_type);
					if (!decl) return NULL;

					stmt = create_node(ctx, NODE_DECL, NULL, NULL, NULL, NULL, NULL, NULL);
					if (!stmt) return NULL;

				} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
					advance_parser(parser); // move past '='
					int element_count = 0;
					Node* elements = parse_array_list(ctx, parser, &element_count);

 					Node* array_list = create_node(ctx, NODE_ARRAY_LIST, NULL, elements, NULL, NULL, NULL, NULL);
					Node* count = create_int_node(ctx, NODE_INTEGER, element_count, NULL, NULL, NULL, NULL, NULL, NULL);
					Node* assignee = create_string_node(ctx, NODE_NAME, id, expr_node, count, NULL, NULL, NULL, array_type);
											
					Node* def = create_node(ctx, NODE_DEF, assignee, NULL, NULL, NULL, NULL, NULL);
					stmt = create_node(ctx, NODE_ASSIGNMENT, def, array_list, NULL, NULL, NULL, NULL);  
					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						Token token = peek_token(parser);
						char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
						Error e = {
							.type = EXPECTED_SEMICOLON,
							.message = message,
							.line = tok.line,
							.column = tok.column
						};
						log_error(ctx, e);

						// log_error(ctx, &token, parser->info, EXPECTED_SEMICOLON);
					}
				}

			} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				Node* decl = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
				stmt = create_node(ctx, NODE_DECL, decl, NULL, NULL, NULL, NULL, NULL);
			
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				Node* assignee = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
				if (!assignee) return NULL;

				Node* def = create_node(ctx, NODE_DEF, assignee, NULL, NULL, NULL, NULL, NULL);
				if (!def) return NULL;

				advance_parser(parser);
				Node* expr_node = parse_logical_or(ctx, parser);
				if (!expr_node) return NULL; 

				stmt = create_node(ctx, NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;				
			}
			break;
		}
		
		case TOKEN_RETURN_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				stmt = create_string_node(ctx, NODE_RETURN, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;

			} else {				
				Node* node = parse_logical_or(ctx, parser);
				if (!node) return NULL;

				stmt = create_node(ctx, NODE_RETURN, NULL, node, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;
			}
			break;
		}

		case TOKEN_CONTINUE_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_SEMICOLON,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_SEMICOLON);
			}

			stmt = create_node(ctx, NODE_CONTINUE, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!stmt) return NULL;
			break;
		}

		case TOKEN_BREAK_KEYWORD: {
			advance_parser(parser);
			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_SEMICOLON,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_SEMICOLON);
			}

			stmt = create_node(ctx, NODE_BREAK, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!stmt) return NULL;
			break;
		}

		case TOKEN_IF_KEYWORD: {
			// printf("\033[32mBefore advancing in TOKEN_IF_KEYWORD, current token type is %d\033[0m\n", peek_token_type(parser));
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_PARENTHESES,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_LEFT_PARENTHESES);
			} 

			Node* condition_node = parse_logical_or(ctx, parser);
			if (!condition_node) {
				printf("Error: received null condition node in if statement\n");
				return NULL;
			}

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_BRACE,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_LEFT_BRACE);
			}

			advance_parser(parser);
	
			Node* if_body = parse_block(ctx, parser);
			stmt = create_node(ctx, NODE_IF, condition_node, if_body, NULL, NULL, NULL, NULL);
			if (!stmt) return NULL;
			break;
		} 

		case TOKEN_ELSE_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_IF_KEYWORD) {
				advance_parser(parser);
				
				if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_LEFT_PARENTHESES,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);

					// log_error(ctx, &tok, parser->info, EXPECTED_LEFT_PARENTHESES);
				}			

				Node* condition_node = parse_logical_or(ctx, parser);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_LEFT_BRACE,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);

					// log_error(ctx, &tok, parser->info, EXPECTED_LEFT_BRACE);
				}		

				advance_parser(parser);
				
				Node* body = parse_block(ctx, parser);
				stmt = create_node(ctx, NODE_ELSE_IF, condition_node, body, NULL, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Unable to create if statement in PARSE_STATEMENT\n");
					return NULL;
				}

				// return stmt;

			} else if (peek_token_type(parser) == TOKEN_LEFT_BRACE) {
				advance_parser(parser);

				Node* body = parse_block(ctx, parser);
				if (!body) {
					printf("Error: recived NULL body in else TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				} 

				stmt = create_node(ctx, NODE_ELSE, NULL, body, NULL, NULL, NULL, NULL);
				if (!stmt) {
					printf("Error: Could not create statement NODE_ELSE type in TOKEN_ELSE_KEYWORD case\n");
					return NULL;
				}
				// return stmt;
			} else {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_BRACE,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);

				// log_error(ctx, &tok, parser->info, EXPECTED_LEFT_BRACE);
			}
			break;
		}

		case TOKEN_FOR_KEYWORD: {
			advance_parser(parser); 

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_PARENTHESES,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LET_KEYWORD) {
				Node* initializer = parse_statement(ctx, parser);
				if (!initializer) {
					printf("Error: Initializer is NULL is for loop\n");
					return NULL;
				}
				

				Node* condition = parse_logical_or(ctx, parser);
				if (!condition) {
					printf("Error: Condition in for loop is NULL\n");
					return NULL;
				} 


				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_SEMICOLON,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				advance_parser(parser);

				Node* update = parse_logical_or(ctx, parser);
				if (!update) {
					printf("Error: Update expression in for loop is NULL\n");
					return NULL;
				} 

				if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_RIGHT_PARENTHESES,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_LEFT_BRACE,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				advance_parser(parser);
				Node* loop_body = parse_block(ctx, parser);
				
				initializer->next = condition;
				condition->next = update;
				
				stmt = create_node(ctx, NODE_FOR, initializer, loop_body, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;


			} else if (peek_token_type(parser) == TOKEN_ID) {
				if (peek_next_token_type(parser) == TOKEN_COLON) {
					char* id = NULL;
					{
						Token tok = peek_token(parser);
						int length = strlen(tok.value.str);
						id = arena_allocate(ctx->ast_arena, length + 1);
						if (!id) {
							printf("Error: Unable to allocate space for id\n");
							return NULL;
						}
						strncpy(id, tok.value.str, length); 
						id[length] = '\0';
					}

					advance_parser(parser); 

					advance_parser(parser); // skip ':' since we already know it exists
					{
						if (peek_token_type(parser) != TOKEN_INT_KEYWORD) {
							Token tok = peek_token(parser);
							char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
							Error e = {
								.type = EXPECTED_INT_KEYWORD,
								.message = message,
								.line = tok.line,
								.column = tok.column
							};
							log_error(ctx, e);
						}

						Token tok = peek_token(parser);
						TypeKind kind = get_type(&tok);
						struct Type* type = type_create(ctx, kind, NULL);
						if (!type) return NULL;

						Node* var = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, type); 
						Node* def = create_node(ctx, NODE_DEF, var, NULL, NULL, NULL, NULL, NULL);
						if (!var || !def) return NULL;

						advance_parser(parser);

						if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
							Token tok = peek_token(parser);
							char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
							Error e = {
								.type = EXPECTED_ASSIGNMENT,
								.message = message,
								.line = tok.line,
								.column = tok.column
							};
							log_error(ctx, e);
						}

						advance_parser(parser);
						
						Node* expr_node = parse_logical_or(ctx, parser);
						if (!expr_node) return NULL;

						Node* assignment = create_node(ctx, NODE_ASSIGNMENT, def, expr_node, NULL, NULL, NULL, NULL);
						if (!assignment) return NULL;

						if (peek_token_type(parser) != TOKEN_SEMICOLON) {
							Token tok = peek_token(parser);
							char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
							Error e = {
								.type = EXPECTED_SEMICOLON,
								.message = message,
								.line = tok.line,
								.column = tok.column
							};
							log_error(ctx, e);
						}

						advance_parser(parser);
						
						Node* condition = parse_logical_or(ctx, parser);
						if (condition) {
							assignment->next = condition;
						}

						advance_parser(parser); // skip ';'
					
						Node* update = parse_logical_or(ctx, parser);
						if (update) {
							condition->next = update;
						}

						if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
							Token tok = peek_token(parser);
							char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
							Error e = {
								.type = EXPECTED_RIGHT_PARENTHESES,
								.message = message,
								.line = tok.line,
								.column = tok.column
							};
							log_error(ctx, e);
						}

						advance_parser(parser);
						if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
							Token tok = peek_token(parser);
							char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
							Error e = {
								.type = EXPECTED_LEFT_BRACE,
								.message = message,
								.line = tok.line,
								.column = tok.column
							};
							log_error(ctx, e);
						}

						advance_parser(parser);
						Node* loop_body = parse_block(ctx, parser);

						stmt = create_node(ctx, NODE_FOR, assignment, loop_body, NULL, NULL, NULL, NULL);
						if (!stmt) return NULL;
					}

				} else {
					Node* initializer = parse_statement(ctx, parser);
					if (!initializer) return NULL;

					advance_parser(parser); 

					Node* condition = parse_logical_or(ctx, parser);
					if (!condition) return NULL;

					if (peek_token_type(parser) != TOKEN_SEMICOLON) {
						Token tok = peek_token(parser);
						char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
						Error e = {
							.type = EXPECTED_SEMICOLON,
							.message = message,
							.line = tok.line,
							.column = tok.column
						};
						log_error(ctx, e);
						return NULL;
					}

					advance_parser(parser);
					Node* update = parse_logical_or(ctx, parser);
					if (!update) return NULL;
					
					initializer->next = condition;
					condition->next = update;

					if (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
						Token tok = peek_token(parser);
						char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
						Error e = {
							.type = EXPECTED_RIGHT_PARENTHESES,
							.message = message,
							.line = tok.line,
							.column = tok.column
						};
						log_error(ctx, e);
					}

					advance_parser(parser);

					if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
						Token tok = peek_token(parser);
						char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
						Error e = {
							.type = EXPECTED_LEFT_BRACE,
							.message = message,
							.line = tok.line,
							.column = tok.column
						};
						log_error(ctx, e);
					}

					advance_parser(parser);
					Node* loop_body = parse_block(ctx, parser);
					stmt = create_node(ctx, NODE_FOR, initializer, loop_body, NULL, NULL, NULL, NULL);
					if (!stmt) return NULL;
				}
			}
			break;
		}

		case TOKEN_WHILE_KEYWORD: {
			advance_parser(parser);

			if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_PARENTHESES,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}

			Node* condition_node = parse_logical_or(ctx, parser);
			if (!condition_node) return NULL;

			if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
				Token tok = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_LEFT_BRACE,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}

			advance_parser(parser);
			Node* while_body = parse_block(ctx, parser);

			stmt = create_node(ctx, NODE_WHILE, condition_node, while_body, NULL, NULL, NULL, NULL);
			if (!stmt) return NULL;
			break;
		}

		case TOKEN_ID: {
			char* id = NULL;
			{
				Token tok = peek_token(parser);
				if (tok.value.str) {
					int length = strlen(tok.value.str);
					id = arena_allocate(ctx->ast_arena, length + 1);
					if (!id) {
						printf("Error: Unable to aalocate space for id\n");
						return NULL;
					}
					strncpy(id, tok.value.str, length);
					id[length] = '\0';
				}
			}

			Node* node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!node) return NULL;

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
				Node* index_node = parse_logical_or(ctx, parser);
				if (!index_node) return NULL;

				Node* subscript_node = create_node(ctx, NODE_SUBSCRIPT, node, index_node, NULL, NULL, NULL, NULL);
				if (!subscript_node) return NULL;

				if (peek_token_type(parser) != TOKEN_ASSIGNMENT) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_ASSIGNMENT,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				advance_parser(parser);
				
				Node* expr_node = parse_logical_or(ctx, parser);
				if (!expr_node) return NULL;

				Node* aug = create_node(ctx, NODE_AUG, subscript_node, NULL, NULL, NULL, NULL, NULL);
				if (!aug) return NULL;

				stmt = create_node(ctx, NODE_ASSIGNMENT, aug, expr_node, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;

			} else if (peek_token_type(parser) == TOKEN_LEFT_PARENTHESES) {
				// printf("\033[32mWe have function call\033[0m\n");
				advance_parser(parser);
				
				Node* args = parse_args(ctx, parser);
				stmt =  create_node(ctx, NODE_CALL, node, NULL, NULL, NULL, args, NULL);
				if (!stmt) {
					printf("\033[31mUnable to create NODE_CALL in parse_statement\033[0m\n");
					return NULL;
				}
				
				advance_parser(parser);
			
			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser);
				Node* expr = parse_logical_or(ctx, parser);
				if (!expr) return NULL;

				Node* def = create_node(ctx, NODE_AUG, node, NULL, NULL, NULL, NULL, NULL);
				if (!def) return NULL;

				stmt = create_node(ctx, NODE_ASSIGNMENT, def, expr, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;
				// printf("\033[32mAfter creating assignment node, current token type is %d\033[0m\n", peek_token_type(parser));

			} else if (peek_token_type(parser) == TOKEN_COLON) {
				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_INT_KEYWORD && peek_token_type(parser) != TOKEN_BOOL_KEYWORD &&
					peek_token_type(parser) != TOKEN_CHAR_KEYWORD) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_DATATYPE,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				Token token = peek_token(parser);
				TypeKind type = get_type(&token);
				struct Type* t = type_create(ctx, type, NULL);
				if (!t) return NULL;

				advance_parser(parser);
				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token tok = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_SEMICOLON,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}

				Node* decl = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
				if (!decl) return NULL;

				stmt = create_node(ctx, NODE_DECL, decl, NULL, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;

			} else if (peek_token_type(parser) == TOKEN_INCREMENT) {
				stmt = create_node(ctx, NODE_INCREMENT, node, NULL, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;
				advance_parser(parser);
			
			} else if (peek_token_type(parser) == TOKEN_DECREMENT) {
				stmt = create_node(ctx, NODE_DECREMENT, node, NULL, NULL, NULL, NULL, NULL);
				if (!stmt) return NULL;
				advance_parser(parser);
			}
			break;
		}
	}

	if (have_valid_statement(ctx, parser, stmt)) {
		switch (stmt->type) {
			case NODE_IF:
			case NODE_ELSE_IF:
			case NODE_ELSE:
			case NODE_FOR:
			case NODE_WHILE:
				break;

			default: 
				advance_parser(parser);
				break;
		}
		return stmt;
	}
	advance_parser(parser);
	return NULL;
}

Node* parse_block(CompilerContext* ctx, Parser* parser) {
	Node* block = create_node(ctx, NODE_BLOCK, NULL, NULL, NULL, NULL, NULL, NULL);
	if (!block) return NULL;
	
	Node* head = NULL;
	Node* current = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		Node* stmt = parse_statement(ctx, parser);

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
	return block;
}

Node* parse_parameters(CompilerContext* ctx, Parser* parser) {
	Node* head = NULL;
	Node* current= NULL;
	Node* node = NULL;
	Node* wrapped_param = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
			Error e = {
				.type = EXPECTED_IDENTIFIER,
				.message = message,
				.line = tok.line,
				.column = tok.column
			};
			log_error(ctx, e);
		} 

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			int length = strlen(tok.value.str);
			id = arena_allocate(ctx->ast_arena, length + 1);
			if (!id) {
				printf("Error: Unable to allocate space for parameter id\n");
				return NULL;
			} 		
			strncpy(id, tok.value.str, length);
			id[length] = '\0';
		}
		
		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_COLON) {
			Token tok = peek_token(parser);
			char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
			Error e = {
				.type = EXPECTED_COLON,
				.message = message,
				.line = tok.line,
				.column = tok.column
			};
			log_error(ctx, e);
		}

		advance_parser(parser);

		if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
			peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
			peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {

			Token tok = peek_token(parser);
			char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
			Error e = {
				.type = EXPECTED_DATATYPE,
				.message = message,
				.line = tok.line,
				.column = tok.column
			};
			log_error(ctx, e);
		}

		Token tok = peek_token(parser);
		TypeKind param_type = get_type(&tok);
		struct Type* t = type_create(ctx, param_type, NULL);
		if (!t) {
			printf("Unable to create type in 'parse_parameters\n");
			return NULL;
		}

		advance_parser(parser);

		node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
		if (!node) {
			printf("In 'parse_parameters' unable to create node with name '%s'.\n", id);
			return NULL;
		}
		
		wrapped_param = create_node(ctx, NODE_PARAM, NULL, node, NULL, NULL, NULL, NULL);
		if (!wrapped_param) {
			printf("Unable to create wrapped param node.\n");
			return NULL;
		}

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

		switch (peek_token_type(parser)) {
			case TOKEN_COMMA: advance_parser(parser); break;
			case TOKEN_RIGHT_PARENTHESES: break;
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

Node* parse_function(CompilerContext* ctx, Parser* parser) {	
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_IDENTIFIER,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);

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

			if (parentheses_count != 0) return NULL;

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

			if (brace_count != 0) return NULL;
		}
		advance_parser(parser);
		return NULL;
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		int length = strlen(tok.value.str);
		id = arena_allocate(ctx->ast_arena, length + 1);
		if (!id) {
			printf("In 'parse_function', error: unable to allocate space for function identifier '%s'.\n", tok.value.str);
			return NULL;
		}
		strncpy(id, tok.value.str, length);
		id[length] = '\0';
	}

	advance_parser(parser);
	
	Node* params = NULL;
	if (peek_token_type(parser) != TOKEN_LEFT_PARENTHESES) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_LEFT_PARENTHESES,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);

		token_t synchronizations[1] = {TOKEN_ID};
		synchronize(parser, synchronizations, 1);
		params = parse_parameters(ctx, parser);
	} else {
		advance_parser(parser);
		params = parse_parameters(ctx, parser);
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ARROW) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_ARROW,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);

		token_t synchronizations[5] = {TOKEN_INT_KEYWORD, TOKEN_CHAR_KEYWORD, TOKEN_BOOL_KEYWORD, TOKEN_STRUCT_KEYWORD, TOKEN_VOID_KEYWORD};
		synchronize(parser, synchronizations, 5);
	}
	
	advance_parser(parser);

	token_t type = peek_token_type(parser);
	struct Type* function_subtype = NULL;
	struct Type* function_maintype = NULL;

	if (!valid_function_return_type(type)) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_DATATYPE,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);

		token_t synchronizations[1] = {TOKEN_LEFT_BRACE};
		synchronize(parser, synchronizations, 1);
	} else {
		Token tok = peek_token(parser);
		TypeKind return_type = get_type(&tok);
		function_subtype = type_create(ctx, return_type, NULL);
		if (!function_subtype) return NULL;

		function_maintype = type_create(ctx, TYPE_FUNCTION, function_subtype);
		if (!function_maintype) return NULL;
		advance_parser(parser);
	} 

	Node* function_body = NULL;
	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token token = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_LEFT_BRACE,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);

		token_t synchronizations[7] = {TOKEN_LET_KEYWORD, TOKEN_ID, TOKEN_STRUCT_KEYWORD, TOKEN_ENUM_KEYWORD, TOKEN_FOR_KEYWORD, TOKEN_WHILE_KEYWORD, TOKEN_IF_KEYWORD};
		synchronize(parser, synchronizations, 7);
		
		function_body = parse_block(ctx, parser);
		if (!function_body) return NULL;

	} else {
		advance_parser(parser);
		function_body = parse_block(ctx, parser);
		if (!function_body) return NULL; 
	}

	Node* function_node = create_string_node(ctx, NODE_NAME, id, NULL, function_body, NULL, NULL, params, function_maintype);
	if (!function_node) return NULL;

	return function_node;
}

Node* parse_args(CompilerContext* ctx, Parser* parser) {
	Node* head = NULL;
	Node* current = NULL;
	Node* arg = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_PARENTHESES) {
		arg = parse_logical_or(ctx, parser);
		Node* wrapped_arg = create_node(ctx, NODE_ARG, NULL, arg, NULL, NULL, NULL, NULL);

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
			return NULL;
		}

		switch (peek_token_type(parser)) {
			case TOKEN_COMMA: advance_parser(parser); break;
			case TOKEN_RIGHT_PARENTHESES: break;
		}
	}

	if (current) {
		current->next = NULL;
	}
	return head;
}

Node* parse_array_list(CompilerContext* ctx, Parser* parser, int* element_count) {
	Node* head = NULL;
	Node* current = NULL;
	Node* array_element = NULL;

	advance_parser(parser);	

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		array_element = parse_logical_or(ctx, parser);

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
			return NULL;
		}

		(*element_count)++;

		if (peek_token_type(parser) == TOKEN_COMMA) {
			advance_parser(parser);
		}
	}

	if (current) {
		current->right = NULL;
	}

	advance_parser(parser);
	return head;
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

Node* parse(CompilerContext* ctx, Lexer* lexer) {
	ctx->phase = PHASE_PARSER;

	Parser parser =  {
		.tokens = lexer->tokens,
		.info = lexer->info
	};

	Node* head = NULL;
	Node* current = NULL;
	Node* node = NULL;

	token_t synchronizations[4] = {TOKEN_FUNCTION_KEYWORD, TOKEN_LET_KEYWORD, TOKEN_STRUCT_KEYWORD, TOKEN_ENUM_KEYWORD};
	size_t length = sizeof(synchronizations) / sizeof(synchronizations[0]);

	while (!at_token_eof(&parser)) {
		token_t current_type = peek_token_type(&parser);
		switch (current_type) {
			case TOKEN_FUNCTION_KEYWORD: {
				node = parse_function(ctx, &parser); 
				break;
			}
			
			case TOKEN_LET_KEYWORD: {
				node = parse_let(ctx, &parser); 
				break;
			}
			
			case TOKEN_STRUCT_KEYWORD: {
				node = parse_struct(ctx, &parser); 
				break;
			}

			case TOKEN_ENUM_KEYWORD: {
				node = parse_enum(ctx, &parser);
				break;
			}

			default: {
				printf("Unexpected token type in parse : %d\n", current_type);
				node = NULL;
				break;
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
		} else {
			synchronize(&parser, synchronizations, length);
		}
	}
	return head;
}

Node* parse_let(CompilerContext* ctx,Parser* parser) {
	Node* let_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_IDENTIFIER,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		int length = strlen(tok.value.str);
		id = arena_allocate(ctx->ast_arena, length + 1);
		if (!id) {
			printf("Error: Unable to allocate space for id in 'parse_let'\n");
			return NULL;
		}
		strncpy(id, tok.value.str, length);
		id[length] = '\0';
	}

	advance_parser(parser);
	
	if (peek_token_type(parser) != TOKEN_COLON) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_COLON,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_INT_KEYWORD && 
		peek_token_type(parser) != TOKEN_CHAR_KEYWORD &&
		peek_token_type(parser) != TOKEN_BOOL_KEYWORD) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_DATATYPE,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	{
		Token tok = peek_token(parser);
		TypeKind type = get_type(&tok);
		struct Type* t = type_create(ctx, type, NULL);
		if (!t) return NULL;
			
		advance_parser(parser);

		if (peek_token_type(parser) == TOKEN_LEFT_BRACKET) {
			advance_parser(parser);
			struct Type* array_type = type_create(ctx, TYPE_ARRAY, t);
			if (!array_type) return NULL;

			Node* expr_node = parse_logical_or(ctx, parser);
			if (!expr_node) return NULL;

			if (peek_token_type(parser) != TOKEN_RIGHT_BRACKET) {
				Token token = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_RIGHT_BRACKET,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}

			advance_parser(parser);

			if (peek_token_type(parser) == TOKEN_SEMICOLON) {
				let_node = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, NULL, array_type);
				if (!let_node) return NULL;

				advance_parser(parser);

			} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
				advance_parser(parser); 
				int element_count = 0;
				
				Node* assignee = create_string_node(ctx, NODE_NAME, id, expr_node, NULL, NULL, NULL, NULL, array_type);
				if (!assignee) return NULL;

				Node* elements = parse_array_list(ctx, parser, &element_count);
				if (!elements) return NULL;

				let_node = create_string_node(ctx, NODE_ASSIGNMENT, NULL, assignee, elements, NULL, NULL, NULL, NULL);   
				if (!let_node) return NULL;

				advance_parser(parser);

				if (peek_token_type(parser) != TOKEN_SEMICOLON) {
					Token token = peek_token(parser);
					char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
					Error e = {
						.type = EXPECTED_SEMICOLON,
						.message = message,
						.line = tok.line,
						.column = tok.column
					};
					log_error(ctx, e);
				}
				advance_parser(parser);
			} 
		} else if (peek_token_type(parser) == TOKEN_SEMICOLON) {
			let_node = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
			if (!let_node) return NULL;

			advance_parser(parser);
		} else if (peek_token_type(parser) == TOKEN_ASSIGNMENT) {
			advance_parser(parser);

			Node* assignee = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, t);
			if (!assignee) return NULL;

			Node* expr_node = parse_logical_or(ctx, parser);
			if (!expr_node) return NULL;
			
			let_node = create_string_node(ctx, NODE_ASSIGNMENT, NULL, assignee, expr_node, NULL, NULL, NULL, NULL);
			if (!let_node) return NULL;

			if (peek_token_type(parser) != TOKEN_SEMICOLON) {
				Token token = peek_token(parser);
				char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
				Error e = {
					.type = EXPECTED_SEMICOLON,
					.message = message,
					.line = tok.line,
					.column = tok.column
				};
				log_error(ctx, e);
			}
			advance_parser(parser);
		}
	}
		
	return let_node;
}

Node* parse_enum_body(CompilerContext* ctx, Parser* parser) {
	Node* head = NULL;
	Node* current = NULL;
	Node* stmt = NULL;

	while (peek_token_type(parser) != TOKEN_RIGHT_BRACE) {
		if (peek_token_type(parser) != TOKEN_ID) {
			Token tok = peek_token(parser);
			char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
			Error e = {
				.type = EXPECTED_IDENTIFIER,
				.message = message,
				.line = tok.line,
				.column = tok.column
			};
			log_error(ctx, e);
		}

		char* id = NULL;
		{
			Token tok = peek_token(parser);
			int length = strlen(tok.value.str);
			id = arena_allocate(ctx->ast_arena, length + 1);
			if (!id) return NULL;
			strncpy(id, tok.value.str, length);
			id[length] = '\0';
		}

		stmt = create_string_node(ctx, NODE_NAME, id, NULL, NULL, NULL, NULL, NULL, NULL);
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

		if (peek_token_type(parser) == TOKEN_COMMA) { 
			advance_parser(parser); 
		}
	}

	advance_parser(parser); // go over '}'
	return head;
}

// need to fix
Node* parse_enum(CompilerContext* ctx, Parser* parser) {
	Node* enum_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_IDENTIFIER,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		int length = strlen(tok.value.str);
		id = arena_allocate(ctx->ast_arena, length + 1);
		if (!id) return NULL;
		strncpy(id, tok.value.str, length);
		id[length] = '\0';
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_LEFT_BRACE,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	advance_parser(parser);

	Node* enum_body = parse_enum_body(ctx, parser);
	if (!enum_body) return NULL;

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_SEMICOLON,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	advance_parser(parser);

	enum_node = create_string_node(ctx, NODE_ENUM, id, NULL, enum_body, NULL, NULL, NULL, NULL);
	if (!enum_node) return NULL;
	return enum_node;
}

Node* parse_struct(CompilerContext* ctx, Parser* parser) {
	Node* struct_node = NULL;
	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_ID) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_IDENTIFIER,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	char* id = NULL;
	{
		Token tok = peek_token(parser);
		int length = strlen(tok.value.str);
		id = arena_allocate(ctx->ast_arena, length + 1);
		if (!id) return NULL;
		strncpy(id, tok.value.str, length);
		id[length] = '\0';
	}

	advance_parser(parser);

	if (peek_token_type(parser) != TOKEN_LEFT_BRACE) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_LEFT_BRACE,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	advance_parser(parser);

	Node* struct_body = parse_block(ctx, parser);
	if (!struct_body) return NULL;

	if (peek_token_type(parser) != TOKEN_SEMICOLON) {
		Token tok = peek_token(parser);
		char* message = error_prelude(ctx, parser->info->filename, tok.line, tok.column);
		Error e = {
			.type = EXPECTED_SEMICOLON,
			.message = message,
			.line = tok.line,
			.column = tok.column
		};
		log_error(ctx, e);
	}

	advance_parser(parser); 
	struct Type* t = type_create(ctx, TYPE_STRUCT, NULL);
	if (!t) return NULL;

	struct_node = create_string_node(ctx, NODE_STRUCT_DEF, id, NULL, struct_body, NULL, NULL, NULL, t);
	if (!struct_node) return NULL;

	return struct_node;
}