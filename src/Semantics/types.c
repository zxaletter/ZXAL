#include "auxiliaries.h"
#include "symbols.h"
#include "memallocator.h"

Node* params_copy(Node* params) {
	if (!params) return NULL;

	Node* head = NULL;
	Node* current = NULL;
	Node* duplicate_param = NULL;
	Node* duplicate_wrapped_param = NULL;
	Node* current_wrapped_param = params;

	while (current_wrapped_param) {
		Node* next_current_wrapped_param = current_wrapped_param->next;
		
		char* identifier = current_wrapped_param->right->value.name ? current_wrapped_param->right->value.name : NULL;
		struct type* type = current_wrapped_param->right->t;
		duplicate_param = create_string_node(NODE_NAME, identifier, NULL, NULL, NULL, NULL, type);
		duplicate_wrapped_param = create_node(NODE_PARAM, NULL, duplicate_param, NULL, NULL, NULL);

		if (duplicate_wrapped_param) {
			if (!head) {
				head = duplicate_wrapped_param;
				current = duplicate_wrapped_param;
			} else {
				current->next = duplicate_wrapped_param;
				duplicate_wrapped_param->prev = current;
				current = duplicate_wrapped_param;
			}
		} else {
			printf("Failed to create duplicate wrapped param with name '%s'.\n", duplicate_param->value.name ? duplicate_param->value.name : "N/A");
			return NULL;
		}
		current_wrapped_param = next_current_wrapped_param;
	}

	return head;
}

bool type_equals(struct type* a, struct type* b) {
	if (!a || !b) return false;

	if (a->kind == b->kind) {
		switch (a->kind) {
			case TYPE_BOOL:
			case TYPE_CHAR:
			case TYPE_INTEGER: { return true; } 

			case TYPE_FUNCTION: {
				Node* func_param = a->params;
				Node* func_arg = b->params;

				while (func_param && func_arg) {
					Node* next_func_param = func_param->next;
					Node* next_func_arg = func_arg->next;
	
					if (!type_equals(func_param->right->t, func_arg->right->t)) {
						return false;
					}

					func_param = next_func_param;
					func_arg = next_func_arg;
				}
				
				return (!func_param && !func_arg) && type_equals(a->subtype, b->subtype);
			}
			case TYPE_ARRAY: {
				return type_equals(a->subtype, b->subtype);
			}

		}
	}
	return false;
}

struct type* type_create(Arena* arena, data_t kind, struct type* subtype, Node* params) {
	// struct type* type = malloc(sizeof(struct type));
	struct type* type = arena_allocate(arena, sizeof(struct type));
	if (!type) {
		perror("Unable to allocate space for type\n");
		return NULL;
	}

	type->kind = kind;
	type->type_free = false;
	type->subtype = NULL;
	type->params = NULL;

	if (subtype) {
		type->subtype = type_copy(subtype);
		if (!type->subtype) {
			perror("Unable to copy subtype\n");
			// free(type);
			return NULL;
		}
	}

	if (params) {
		type->params = params_copy(params);
		if (!type->params) {
			// if (type->subtype) free_type(type->subtype);
			// free(type);
			return NULL;
		}
	}

	return type;
}

struct type* type_copy(Arena* arena, struct type* original_type) {
	if (!original_type) return NULL;

	// struct type* duplicate_type = malloc(sizeof(struct type));
	struct type* duplicate_type = arena_allocate(arena, sizeof(struct type));
	if (!duplicate_type) {
		perror("In 'type_copy', unable to allocate space for type\n");
		return NULL;
	}

	duplicate_type->kind = original_type->kind;
	duplicate_type->type_free = false;
	duplicate_type->subtype = NULL;
	duplicate_type->params = NULL;

	if (original_type->subtype) {
		duplicate_type->subtype = type_copy(original_type->subtype);
		if (!duplicate_type->subtype) {
			// free(duplicate_type);
			return NULL;
		}
	}

	if (original_type->params) {
		duplicate_type->params = params_copy(original_type->params);
		if (!duplicate_type->params) {
			// if (duplicate_type->subtype) free_type(duplicate_type->subtype);
			// free(duplicate_type);
			return NULL;
		}
	}

	return duplicate_type;
}

struct type* typecheck_expression(Arena* arena,Node* node) {
	if (!node) return;

	printf("In typecheck expression\n");
	struct type* lt = NULL;
	struct type* rt = NULL;
	struct type* result = NULL;

	switch (node->type) {
		case NODE_BOOL: {
			result = type_create(arena, TYPE_BOOL, NULL, NULL);
			break;
		}

		case NODE_CHAR: {
			result = type_create(arena, TYPE_CHAR, NULL, NULL);
			break;
		}

		case NODE_INTEGER: {
			result = type_create(arena, TYPE_INTEGER, NULL, NULL);
			break;
		}

		case NODE_EQUAL:
		case NODE_NOT_EQUAL:
		case NODE_LESS:
		case NODE_GREATER:
		case NODE_LESS_EQUAL:
		case NODE_GREATER_EQUAL: {
			printf("NODE HAS TYPE %d\n", node->type);
			printf("LEFT NODE HAS TYPE %d\n", node->left->type);
			printf("RIGHT NODE HAS TYPE %d\n", node->right->type);
			lt = typecheck_expression(node->left);
			rt = typecheck_expression(node->right);
			if (!lt) {printf("left type is NULL\n");}
			if (!rt) { printf("right type is NULL\n");}
			if (!lt || !rt || lt->kind != TYPE_INTEGER || rt->kind != TYPE_INTEGER) {
				printf("Comparison requires integer types\n");
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(arena, TYPE_BOOL, NULL, NULL);
			}
			break;
		}

		case NODE_ADD:
		case NODE_SUB:
		case NODE_MUL:
		case NODE_DIV: {
			lt = typecheck_expression(node->left);
			rt = typecheck_expression(node->right);

			if (!lt || !rt || lt->kind != TYPE_INTEGER || rt->kind != TYPE_INTEGER) {
				printf("Error: Arithmetic expression require integer types\n");
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} 
			else { 
				result = type_create(arena, TYPE_INTEGER, NULL, NULL); 
			} 
			break;
		}

		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {
			lt = typecheck_expression(node->left);
			rt = typecheck_expression(node->right);

			if (!lt || !rt || lt->kind != TYPE_BOOL || rt->kind != TYPE_BOOL) {
				printf("May only use '&&' or '||' for boolean types\n");
				printf("lt kind is %d and right type kind is %d\n", lt->kind, rt->kind);
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(arena, TYPE_BOOL, NULL, NULL);
			}
			if (result && result->kind == TYPE_BOOL) {
				printf("LEAVING NODE_LOGICAL_AND/NODE_LOGICAL_OR with type of result %d\n", result->kind);
			}
			break;
		}

		case NODE_NOT: {
			rt = typecheck_expression(node->right);

			if (!rt || rt->kind != TYPE_BOOL) {
				printf("May only apply '!' to boolean types -> rt kind is %d\n", rt->kind);
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(arena, TYPE_BOOL, NULL, NULL);
			}

			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			lt = typecheck_expression(node->left);
			if (!lt || lt->kind != TYPE_INTEGER) {
				printf("Error: Increment/decrement requires integer type\n");
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			}
			result = type_create(arena, TYPE_INTEGER, NULL, NULL);
			break;
		}

		case NODE_CALL: {
			if (!type_equals(node->left->t, node->left->symbol->type)) {
				printf("Attempting to call '%s' even though '%s' is not a function.\n",
					node->left->value.name ? node->left->value.name : "N/A", node->left->value.name ? node->left->value.name : "N/A");
				printf("Left node type is %d and left node symbol type is %d\n", node->left->t->kind, node->left->symbol->type->kind);
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			}
			result = type_copy(arena, node->left->t->subtype);
			if (result) {
				printf("IN NODE CALL and result has type %d\n", result->kind);
			}
			break; 

		}

		case NODE_NAME: {
			result = type_copy(arena, node->t);
			printf("heelo\n");
			if (result) {
				printf("LEAVING NODE_NAME case for '%s'\n", node->value.name ? node->value.name : "N/A");
				printf("Result has type %d\n", result->kind);
			}
			break;
		}

		case NODE_AUG:
		case NODE_DEF:
		case NODE_DECL: {
			result = typecheck_expression(node->left);
			break;
		}

		case NODE_SUBSCRIPT: {
			printf("IN NODE_SUBSCRIPT CASE\n");
			lt = typecheck_expression(node->left);
			rt = typecheck_expression(node->right);

			if (!lt || lt->kind != TYPE_ARRAY) {
				printf("Left operand of subscript must be an array\n");
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else if (!rt || rt->kind != TYPE_INTEGER) {
				printf("right operand of subscript must be an integer type\n");
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_copy(arena, lt->subtype);
			}
			if (result->kind != TYPE_UNKNOWN) {
				printf("LEAVING NODE_SUBSCRIPT CASE with result type is %d\n", result->kind);
			}
			break;
		}
	}

	// if (lt) free_type(lt);
	// if (rt) free_type(rt);
	return result;
}

void typecheck_statement(Node* node) {
	if (!node) return;

	struct type* lt = NULL;
	struct type* rt = NULL;
	struct type* result = NULL;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			printf("IN TYPECHECK STATEMENT->NODE ASSIGNMENT CASE\n");
			if (node->right->type == NODE_ARRAY_LIST) {
				printf("in regular case\n");
				lt = typecheck_expression(node->left);
				if (!lt) return;

				if (lt->subtype) {
					Node* array_elements = node->right;
					Node* element = array_elements->right;
					while (element) {
						struct type* element_type = element->t;
						if (!type_equals(lt->subtype, element_type)) {
							printf("Array element and subtype not equal. Subtype kind is %d\n", lt->subtype->kind);
							return;
						}
						element = element->next;
					}
				}
			} else {
				printf("In else case\n");
				lt = typecheck_expression(node->left);
				if (!lt) { printf("IN NODE_ASSIGNMENT-> left type is NULL\n"); }
				rt = typecheck_expression(node->right);
				if (!rt) { printf("IN NODE_ASSIGNMENT-> right type is NULL\n"); }
				if (!lt || !rt || !type_equals(lt, rt)) {
					printf("Error: type mismatch in assignment\n");
					printf("Left type is %d and right type is \n", lt->kind);
					result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
				} else {
					result = type_copy(arena, lt);
				}
			}
			printf("LEAVING NODE ASSIGNMENT CASE IN TYPECHECK STATEMENT\n");
			break;
		}

		case NODE_ELSE: {
			printf("IN TYPECHECK STATEMENT->NODE ELSE CASE\n");
			if (node->right) {
				typecheck_statement(node->right);
			}
			break;
		} 

		case NODE_RETURN: {
			printf("IN TYPECHECK STATEMENT->NODE RETURN CASE\n");
			rt = typecheck_expression(node->right);
			if (!rt || !type_equals(arena, rt, node->t)) {
				result = type_create(arena, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_copy(arena, rt);
			}
			break;
		}

		case NODE_CONTINUE:
		case NODE_BREAK: { break; }

		case NODE_BLOCK: {
			printf("IN TYPECHECK STATEMENT-> NODE BLOCK CASE\n");
			if (node->right) {
				Node* stmt = node->right;
				while (stmt) {
					Node* stmt_next = stmt->next;
					typecheck_statement(stmt);
					stmt = stmt_next;
				}
			}
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			printf("In typecheck statement-> NODE INCREMENT OR DECREMENT CASE\n");
			if (node->left) {
				typecheck_expression(node->left);
			}
			break;
		}

		case NODE_IF:
		case NODE_ELSE_IF:
		case NODE_WHILE: {
			if (node->left) {
				printf("Here in while/else if/if\n");
				result = typecheck_expression(node->left);

				if (!result || result->kind != TYPE_BOOL) {
					printf("Condition in while/else if/ if must be boolean type\n");
					exit(EXIT_FAILURE);
				}
			}
			if (result && result->kind == TYPE_BOOL) {
				printf("LEAVING condition in while/else if/if\n");
			}
			printf("MID\n");
			if (node->right) {
				typecheck_statement(node->right);
			}
			printf("End of while/else if/if\n");
			break;
		} 
	
		case NODE_FOR: {
			printf("In typecheck statement-> IN NODE_FOR case\n");
			if (node->left) {
				printf("type of node->left in for is %d\n", node->left->type);
				if (node->left->left) { printf("type of left child of node->left is %d\n", node->left->left->type); }
				if (node->left->right) { printf("type of right child of node->left is %d\n", node->left->right->type); }
				typecheck_statement(node->left);
			}

			Node* condition = node->left->next;
			if (condition) {
				typecheck_expression(condition);
			}

			Node* update = condition->next;
			if (update) {
				typecheck_expression(update);
			}

			if (node->right) {
				typecheck_statement(node->right);
			}
			break;
		}
	}
	// if (lt) free_type(lt);
	// if (rt) free_type(rt);
	// if (result) free_type(result);
}

void typecheck_params(Node* params) {
	if (!params) return;

	Node* actual_param = params->right;
	if (!actual_param) { return; }
	if (!type_equals(actual_param->t, actual_param->symbol->type)) { 
		return; 
	} else {
		printf("Types for '%s' are equal\n", actual_param->value.name);
	}
}

void typecheck_globals(Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			if (node->left) {
				typecheck_expression(node->left);
			}

			if (node->right) {
				typecheck_expression(node->right);
			}

			break;
		}

		case NODE_NAME: {
			if (node->t) {
				if (node->t->kind == TYPE_FUNCTION) {
					printf("About to typecheck function '%s'\n", node->value.name);
					if (node->left) {
						Node* param = node->left;
						while (param) {
							Node* next_param = param->next;
							printf("About to typecheck param: '%s'\n", param->right->value.name ? param->right->value.name : 
								"N/A");
							typecheck_params(param);
							param = next_param;
						}
					}

					if (node->right) {
						typecheck_statement(node->right);
					}
				}
			}
			break;
		}
	}
}

void typecheck_tree(Node* root) {
	if (!root) return;


	Node* node = root;
	while (node) {
		Node* next = node->next;
		typecheck_globals(node);
		node = next;
	}
}