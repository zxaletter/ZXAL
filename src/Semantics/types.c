#include "auxiliaries.h"
#include "symbols.h"
#include "bumpallocator.h"

Node* params_copy(CompilerContext* ctx, Node* params) {
	if (!params) return NULL;
	printf("here in param copy\n");
	Node* head = NULL;
	Node* current = NULL;
	Node* current_wrapped_param = params;
	printf("Further down in params copy\n");
	while (current_wrapped_param) {
		printf("in params copy loop\n");
		Node* next_current_wrapped_param = current_wrapped_param->next;
		printf("now im here\n");
		char* identifier = current_wrapped_param->right->value.name;
		printf("Got '%s' in params copy\n", identifier);
		struct type* type = current_wrapped_param->right->t;
		printf("here in params_copy\n");
		Node* duplicate_param = create_string_node(ctx, NODE_NAME, identifier, NULL, NULL, NULL, NULL, type);
		if (duplicate_param) { 
			printf("Successfully created duplicate param\n");
		} else {
			printf("Received NULL duplicate param\n");
		}

		Node* duplicate_wrapped_param = create_node(ctx, NODE_PARAM, NULL, duplicate_param, NULL, NULL, NULL);
		if (duplicate_wrapped_param) {
			printf("Successfully created duplicate wrapped param\n");
		} else {
			printf("Received NULL duplicate wrapped param\n");
		}
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
	printf("About to return params\n");
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

struct type* type_create(CompilerContext* ctx, data_t kind, struct type* subtype, Node* params) {
	// struct type* type = malloc(sizeof(struct type));
	struct type* type = arena_allocate(ctx->type_arena, sizeof(struct type));
	if (!type) {
		perror("Unable to allocate space for type\n");
		return NULL;
	}

	type->kind = kind;
	type->type_free = false;
	type->subtype = subtype;
	type->params = params;
	type->is_a_copy = false;

	return type;
}

struct type* type_copy(CompilerContext* ctx, struct type* original_type) {
	if (!original_type) return NULL;
	printf("Here in type_copy\n");
	if (original_type->is_a_copy) {
		return original_type;
	}

	struct type* duplicate_type = arena_allocate(ctx->type_arena, sizeof(struct type));
	if (!duplicate_type) {
		perror("In 'type_copy', unable to allocate space for type\n");
		return NULL;
	}

	duplicate_type->is_a_copy = true;
	duplicate_type->kind = original_type->kind;
	duplicate_type->type_free = false;
	duplicate_type->subtype = NULL;
	duplicate_type->params = original_type->params;

	if (original_type->subtype) {
		duplicate_type->subtype = type_copy(ctx, original_type->subtype);
		if (!duplicate_type->subtype) {
			// free(duplicate_type);
			return NULL;
		}
	}

	// if (original_type->params) {
	// 	duplicate_type->params = params_copy(ctx, original_type->params);
	// 	if (!duplicate_type->params) {
	// 		// if (duplicate_type->subtype) free_type(duplicate_type->subtype);
	// 		// free(duplicate_type);
	// 		return NULL;
	// 	}
	// }
	// printf("bout to return type with kind %d\n", duplicate_type->kind);
	return duplicate_type;
}

struct type* typecheck_expression(CompilerContext* ctx, Node* node) {
	if (!node) return;

	struct type* lt = NULL;
	struct type* rt = NULL;
	struct type* result = NULL;

	switch (node->type) {
		case NODE_BOOL: {
			result = type_create(ctx, TYPE_BOOL, NULL, NULL);
			break;
		}

		case NODE_CHAR: {
			result = type_create(ctx, TYPE_CHAR, NULL, NULL);
			break;
		}

		case NODE_INTEGER: {
			result = type_create(ctx, TYPE_INTEGER, NULL, NULL);
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
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);
			printf("Back from\n");
			if (!lt || !rt) {
				printf("LEFT OR RIGHT TYPE IS NULL for %d\n", node->type);
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			}
			if (lt->kind == rt->kind) {
				if (lt->kind == TYPE_INTEGER ||
					lt->kind == TYPE_CHAR ||
					lt->kind == TYPE_BOOL) {

					result = type_create(ctx, TYPE_BOOL, NULL, NULL);
				} else {
					printf("May only compare int, char, and bool types\n");
					result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
				}
			}
			break;
		}

		case NODE_MODULO:
		case NODE_ADD_EQUAL:
		case NODE_SUB_EQUAL:
		case NODE_MUL_EQUAL:
		case NODE_DIV_EQUAL:
		case NODE_ADD:
		case NODE_SUB:
		case NODE_MUL:
		case NODE_DIV: {
			printf("\033[31mIn node case with type %d\033[0m\n", node->type);
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);

			if (!lt || !rt) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
				break;
			}

			switch (lt->kind) {
				case TYPE_INTEGER:
				case TYPE_BOOL: {
					if (rt->kind != TYPE_INTEGER && rt->kind != TYPE_BOOL) {
						result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
					} else {
						result = type_create(ctx, TYPE_INTEGER, NULL, NULL);
					}
					break;
				}
				default: {
					printf("Error: Arithmetic expression require integer or boolean types\n");
					printf("left type is %d and right type is %d\n", lt->kind, rt->kind);
					result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
					break;
				}
			}
			printf("\033[31mLeaving node case with type %d\033[0m\n", node->type);
			break;
		}

		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);

			if (!lt || !rt || lt->kind != TYPE_BOOL || rt->kind != TYPE_BOOL) {
				printf("May only use '&&' or '||' for boolean types\n");
				printf("lt kind is %d and right type kind is %d\n", lt->kind, rt->kind);
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(ctx, TYPE_BOOL, NULL, NULL);
			}
			
			break;
		}

		case NODE_UNARY_ADD:
		case NODE_UNARY_SUB: {
			rt = typecheck_expression(ctx, node->right);
			if (!rt || (rt && rt->kind != TYPE_INTEGER)) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(ctx, TYPE_INTEGER, NULL, NULL);
			}
			break;
		}

		case NODE_NOT: {
			// printf("IN NODE_NOT CASE\n");
			rt = typecheck_expression(ctx, node->right);

			if (!rt || rt->kind != TYPE_BOOL) {
				printf("May only apply '!' to boolean types -> rt kind is %d\n", rt->kind);
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(ctx, TYPE_BOOL, NULL, NULL);
			}

			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			lt = typecheck_expression(ctx, node->left);
			if (!lt || lt->kind != TYPE_INTEGER) {
				printf("Error: Increment/decrement requires integer type\n");
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			}
			result = type_create(ctx, TYPE_INTEGER, NULL, NULL);
			break;
		}

		case NODE_CALL: {
			if (!type_equals(node->left->t, node->left->symbol->type)) {
				printf("Attempting to call '%s' even though '%s' is not a function.\n",
					node->left->value.name ? node->left->value.name : "N/A", node->left->value.name ? node->left->value.name : "N/A");
				printf("Left node type is %d and left node symbol type is %d\n", node->left->t->kind, node->left->symbol->type->kind);
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			}
			result = type_create(ctx, node->left->t->subtype->kind, NULL, NULL);
			break; 

		}

		case NODE_NAME: {
			printf("\033[32mIn node name case processing '%s'\033[0m\n", node->value.name);
			if (node->symbol) {
				if (node->symbol->type) {
					if (node->symbol->type->kind == TYPE_ARRAY || node->symbol->type->kind == TYPE_FUNCTION) {
						result = type_create(ctx, node->symbol->type->kind, node->symbol->type->subtype, NULL);
					} else {
						printf("\033[32m'%s' has type %d\033[0m\n", node->value.name, node->t->kind);
						result = type_create(ctx, node->symbol->type->kind, NULL, NULL);
					}
				}
			} else {
				printf("\033[32mUnfortunately, '%s' does not have a symbol\033[0m\n", node->value.name);
			}		
			break;
		}

		case NODE_AUG:
		case NODE_DEF:
		case NODE_DECL: {
			result = typecheck_expression(ctx, node->left);
			break;
		}

		case NODE_SUBSCRIPT: {
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);
			
			if (!lt || lt->kind != TYPE_ARRAY) {
				if (!lt) printf("\033[31mleft type of node subscript is NULL\033[0m\n");
				printf("Left operand of subscript node must be an array\n");
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else if (!rt || rt->kind != TYPE_INTEGER) {
				printf("right operand of subscript must be an integer type\n");
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else {
				// printf("left type and right type in NODE_SUBSCRIPT case are both valid\n");
				if (!lt->subtype) {
					printf("left type has no subtype\n");
				}
				// printf("Left type has type %d\n", lt->kind);
				result = type_create(ctx, lt->subtype->kind, NULL, NULL);
				// printf("HELLo\n");
			}
			// if (result->kind != TYPE_UNKNOWN) {
			// 	printf("LEAVING NODE_SUBSCRIPT CASE with result type is %d\n", result->kind);
			// }
			// printf("HELLLLLLLO\n");
			break;
		}
	}

	// if (lt) free_type(lt);
	// if (rt) free_type(rt);
	return result;
}

void typecheck_statement(CompilerContext* ctx, Node* node) {
	if (!node) return;

	struct type* lt = NULL;
	struct type* rt = NULL;
	struct type* result = NULL;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			if (node->right && node->right->type == NODE_ARRAY_LIST) {
				lt = typecheck_expression(ctx, node->left);
				if (!lt) return;

				if (lt->subtype) {
					Node* array_list = node->right;
					Node* element = array_list->right;
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
				lt = typecheck_expression(ctx, node->left);
				rt = typecheck_expression(ctx, node->right);
				if (!rt) { printf("IN NODE_ASSIGNMENT-> right type is NULL\n"); }
				if (!lt || !rt || !type_equals(lt, rt)) {
					printf("Error: type mismatch in assignment\n");
					printf("Left type is %d and right type is \n", lt->kind);
					result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
				} else {
				}
			}
			printf("LEAVING NODE ASSIGNMENT CASE IN TYPECHECK STATEMENT\n");
			break;
		}

		case NODE_ELSE: {
			printf("IN TYPECHECK STATEMENT->NODE ELSE CASE\n");
			if (node->right) {
				typecheck_statement(ctx, node->right);
			}
			break;
		} 

		case NODE_RETURN: {
			printf("IN TYPECHECK STATEMENT->NODE RETURN CASE\n");
			rt = typecheck_expression(ctx, node->right);
			if (!rt || !type_equals(rt, node->t)) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL, NULL);
			} else {
				result = type_create(ctx, rt->kind, NULL, NULL);
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
					typecheck_statement(ctx, stmt);
					stmt = stmt_next;
				}
			}
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			printf("In typecheck statement-> NODE INCREMENT OR DECREMENT CASE\n");
			if (node->left) {
				typecheck_expression(ctx, node->left);
			}
			break;
		}

		case NODE_IF:
		case NODE_ELSE_IF:
		case NODE_WHILE: {
			printf("\033[31mENTERING CONTROL FLOW NODE WITH TYPE %d\033[0m\n", node->type);
			if (node->left) {
				printf("Here in %d statement\n", node->type);
				result = typecheck_expression(ctx, node->left);

				if (!result) return;

				if (result->kind != TYPE_BOOL) {
					printf("\033[31mCondition in NODE %d must be of boolean type\033[0m\n", node->type);
					return;
				}
			}

			if (node->right) {
				typecheck_statement(ctx, node->right);
			}
			printf("\033[31mLeaving Control flow node with TYPE %d\033[0m\n", node->type);
			break;
		} 
	
		case NODE_FOR: {
			printf("In typecheck statement-> IN NODE_FOR case\n");
			if (node->left) {
				printf("type of node->left in for is %d\n", node->left->type);
				if (node->left->left) { printf("type of left child of node->left is %d\n", node->left->left->type); }
				if (node->left->right) { printf("type of right child of node->left is %d\n", node->left->right->type); }
				typecheck_statement(ctx, node->left);
			}

			Node* condition = node->left->next;
			if (condition) {
				typecheck_expression(ctx, condition);
			}

			Node* update = condition->next;
			if (update) {
				typecheck_expression(ctx, update);
			}

			if (node->right) {
				typecheck_statement(ctx, node->right);
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

void typecheck_globals(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			if (node->left) {
				typecheck_expression(ctx, node->left);
			}

			if (node->right) {
				typecheck_expression(ctx, node->right);
			}

			break;
		}

		case NODE_NAME: {
			if (node->t) {
				if (node->t->kind == TYPE_FUNCTION) {
					printf("\033[31mABOUT TO TYPECHECK FUNCTION '%s'\033[0m\n", node->value.name);
					if (node->t->params) {
						Node* wrapped_param = node->t->params;
						while (wrapped_param) {
							Node* next_wrapped_param = wrapped_param->next;
							typecheck_params(wrapped_param);
							wrapped_param = next_wrapped_param;
						}
					}

					if (node->right) {
						typecheck_statement(ctx, node->right);
					}
					printf("\033[31mFINISHED TYPECHECKING FUNCTION '%s'\033[0m\n", node->value.name);
				}
			}
			break;
		}
	}
}

void typecheck_tree(CompilerContext* ctx, Node* root) {
	if (!ctx || !root) return;

	Node* node = root;
	while (node) {
		Node* next = node->next;
		typecheck_globals(ctx, node);
		node = next;
	}
}