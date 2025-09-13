#include "typechecker.h"
#include "compilercontext.h"
#include "Parser/node.h"
#include "symbols.h"
#include "types.h"
#include "errors.h"
#include "assert.h"

TypeCheckerSymbolStack typechecker_symbol_stack;

bool init_typechecker_symbol_stack(CompilerContext* ctx) {
	typechecker_symbol_stack = create_typechecker_symbol_stack(ctx);
	if (!typechecker_symbol_stack.functions) return false;
	return true;
}

TypeCheckerSymbolStack create_typechecker_symbol_stack(CompilerContext* ctx) {
	Symbol** functions = arena_allocate(
		ctx->type_arena, 
		sizeof(Symbol*) * INIT_STACK_CAPACITY
	);

	if (!functions) {
		TypeCheckerSymbolStack dummy_stack = {
			.top = -1,
			.capacity = 0,
			.functions = NULL
		};
		return dummy_stack;
	}

	TypeCheckerSymbolStack stack = {
		.top = -1,
		.capacity = INIT_STACK_CAPACITY,
		.functions = functions
	};
	return stack;
}

bool add_func_symbol(CompilerContext* ctx, Symbol* symbol) {
	if (typechecker_symbol_stack.top >= typechecker_symbol_stack.capacity) {
		int prev_capacity = typechecker_symbol_stack.capacity;

		typechecker_symbol_stack.capacity *= 2;
		int new_capacity = typechecker_symbol_stack.capacity;
		void** new_functions = arena_reallocate(
			ctx->type_arena,
			typechecker_symbol_stack.functions,
			prev_capacity,
			new_capacity
		);

		if (!new_functions) return false;

		typechecker_symbol_stack.functions = new_functions;
	}
	typechecker_symbol_stack.functions[++typechecker_symbol_stack.top] = symbol;
	return true;
}

void pop_func_symbol() {
	if (!is_typecheck_stack_empty()) {
		typechecker_symbol_stack.top--;
	}
}

bool is_typecheck_stack_empty() {
	return typechecker_symbol_stack.top == -1;
}

Symbol* peek_typechecker_symbol_stack() {
	return typechecker_symbol_stack.functions[typechecker_symbol_stack.top];
}

struct Type* typecheck_expression(CompilerContext* ctx, Node* node) {
	if (!node) return;

	struct Type* lt = NULL;
	struct Type* rt = NULL;
	struct Type* result = NULL;

	switch (node->type) {
		case NODE_BOOL: {
			result = type_create(ctx, TYPE_BOOL, NULL);
			break;
		}

		case NODE_CHAR: {
			result = type_create(ctx, TYPE_CHAR, NULL);
			break;
		}

		case NODE_INTEGER: {
			result = type_create(ctx, TYPE_INTEGER, NULL);
			break;
		}

		case NODE_EQUAL:
		case NODE_NOT_EQUAL:
		case NODE_LESS:
		case NODE_GREATER:
		case NODE_LESS_EQUAL:
		case NODE_GREATER_EQUAL: {
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);

			if (!lt || !rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			switch (lt->kind) {
				case TYPE_INTEGER:
				case TYPE_CHAR: {
					if (rt && (rt->kind == TYPE_INTEGER ||
							   rt->kind == TYPE_CHAR)) {
						result = type_create(ctx, TYPE_BOOL,  NULL);
					}
					break;
				}
				default: 
					result = type_create(ctx, TYPE_UNKNOWN, NULL);
					break;
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
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);

			if (!lt || !rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (lt && lt->kind != TYPE_INTEGER ) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
 			} else if (rt && rt->kind != TYPE_INTEGER) {
 				result = type_create(ctx, TYPE_UNKNOWN, NULL);
 			} else {
 				result = type_create(ctx, TYPE_INTEGER, NULL);
 			}
			break;
		}

		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {
			lt = typecheck_expression(ctx, node->left);
			rt = typecheck_expression(ctx, node->right);

			if (!lt || !rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (lt && lt->kind != TYPE_BOOL) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else if (rt && rt->kind != TYPE_BOOL) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else {
				result = type_create(ctx, TYPE_BOOL, NULL);
			}
			break;
		}

		case NODE_UNARY_ADD:
		case NODE_UNARY_SUB: {
			rt = typecheck_expression(ctx, node->right);
			if (!rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (rt->kind != TYPE_INTEGER) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else {
				result = type_create(ctx, TYPE_INTEGER, NULL);
			}
			break;
		}

		case NODE_NOT: {
			rt = typecheck_expression(ctx, node->right);
			if (!rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (rt->kind != TYPE_BOOL) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else {
				result = type_create(ctx, TYPE_BOOL, NULL);
			}

			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			lt = typecheck_expression(ctx, node->left);
			if (!lt) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (lt && lt->kind != TYPE_INTEGER) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else {
				result = type_create(ctx, TYPE_INTEGER, NULL);
			}
			break;
		}

		case NODE_CALL: {
			Node* wrapped_arg = node->params;

			Symbol* sym = node->left->symbol;
			Node* wrapped_param = sym->params;

			while (wrapped_arg && wrapped_param) {
				Node* next_arg = wrapped_arg->next;
				Node* next_param = wrapped_param->next;

				Type* arg_type = typecheck_expression(ctx, wrapped_arg->right);
				Type* param_type = ((Symbol*)wrapped_param->right->symbol)->type;
				if (!type_equals(arg_type, param_type)) {
					return type_create(ctx, TYPE_UNKNOWN, NULL);
				}

				wrapped_arg = next_arg;
				wrapped_param = next_param;
			}

			if (wrapped_arg || wrapped_param) {
				printf("mismatched number of args -> params\n");
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			Type* func_return_type = NULL;
			if (sym->type && sym->type->subtype) {
				func_return_type = sym->type->subtype;
				result = type_create(ctx, func_return_type->kind, NULL);
			} else {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			break; 

		}

		case NODE_NAME: {
			void* sym = node->symbol;
			if ((Symbol*)sym) {
				Type* t = ((Symbol*)sym)->type;
				if (t && (t->kind == TYPE_ARRAY || t->kind == TYPE_FUNCTION)) {
					result = type_create(
						ctx,
						t->kind,
						t->subtype
					);

				} else {
					result = type_create(
						ctx,
						t ? t->kind : TYPE_UNKNOWN,
						NULL
					);
				}
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
			
			if (!lt || !rt) {
				return type_create(ctx, TYPE_UNKNOWN, NULL);
			}

			if (lt->kind != TYPE_ARRAY) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else if (rt->kind != TYPE_INTEGER) {
				result = type_create(ctx, TYPE_UNKNOWN, NULL);
			} else {
				result = type_create(ctx, lt->subtype->kind, NULL);
			}

			break;
		}
	}

	return result;
}

void typecheck_statement(CompilerContext* ctx, Node* node) {
	if (!node) return;

	struct Type* lt = NULL;
	struct Type* rt = NULL;
	struct Type* result = NULL;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			if (node->right && node->right->type == NODE_ARRAY_LIST) {
				lt = typecheck_expression(ctx, node->left);
				if (!lt) return;

				if (lt->subtype) {
					Node* array_list = node->right;
					Node* element = array_list->right;
					while (element) {
						struct Type* element_type = element->t;
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
				if (!lt || !rt) {
					return type_create(ctx, TYPE_UNKNOWN, NULL);
				}

				if (!type_equals(lt, rt)) {
					return type_create(ctx, TYPE_UNKNOWN, NULL);
				}			
			}
			break;
		}

		case NODE_ELSE: {
			if (node->right) {
				typecheck_statement(ctx, node->right);
			}
			break;
		} 

		case NODE_RETURN: {
			if (node->right) {
				Symbol* func_symbol = peek_typechecker_symbol_stack();
				assert(func_symbol);

				Type* func_return_type = NULL;
				if (func_symbol->type) {
					func_return_type = func_symbol->type->subtype;
				}

				rt = typecheck_expression(ctx, node->right);
				if (!type_equals(rt, func_return_type)) {
					result = type_create(ctx, TYPE_UNKNOWN, NULL);
					assert(false);
				} else {
					result = type_create(ctx, func_return_type->kind, NULL);
				}
			}

			break;
		}

		case NODE_CONTINUE:
		case NODE_BREAK:
			break;

		case NODE_BLOCK: {
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
			if (node->left) {
				typecheck_expression(ctx, node->left);
			}
			break;
		}

		case NODE_IF:
		case NODE_ELSE_IF:
		case NODE_WHILE: {
			if (node->left) {
				result = typecheck_expression(ctx, node->left);
				if (!result || (result && result->kind != TYPE_BOOL)) return;
			}

			if (node->right) {
				typecheck_statement(ctx, node->right);
			}
			break;
		} 
	
		case NODE_FOR: {
			Node* initializer = node->left;

			if (initializer) {
				typecheck_statement(ctx, initializer);
			}

			Node* condition = initializer ? initializer->next : NULL;
			if (condition) {
				typecheck_expression(ctx, condition);
			}

			Node* update = condition ? condition->next : NULL;
			if (update) {
				typecheck_expression(ctx, update);
			}

			if (node->right) {
				typecheck_statement(ctx, node->right);
			}
			break;
		}
	}
}

void typecheck_params(Node* params) {
	if (!params) return;

	Node* actual_param = params->right;
	if (!actual_param) { return; }
	if (!type_equals(actual_param->t, ((Symbol*)actual_param->symbol)->type)) { 
		return; 
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
			if ((Type*)node->t) {
				if (((Type*)node->t)->kind == TYPE_FUNCTION) {
					add_func_symbol(ctx, (Symbol*)node->symbol);
					// printf("\033[32mAbout to typecheck function '%s'\033[0m\n", node->value.name);
					if (node->params) {
						Node* wrapped_param = node->params;
						while (wrapped_param) {
							Node* next_wrapped_param = wrapped_param->next;
							typecheck_params(wrapped_param);
							wrapped_param = next_wrapped_param;
						}
					}

					if (node->right) {
						typecheck_statement(ctx, node->right);
					}
					// printf("\033[32mFinished typechecking function'%s'\033[0m\n", node->value.name);
					pop_func_symbol();
				}
			}
			break;
		}
	}
}

void typecheck_tree(CompilerContext* ctx, Node* root) {
	if (!ctx || !root) return;

	if (!init_typechecker_symbol_stack(ctx)) return;
	

	Node* node = root;
	while (node) {
		Node* next = node->next;
		typecheck_globals(ctx, node);
		node = next;
	}
}