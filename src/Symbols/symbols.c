#include "symbols.h"

static int scope_level = 0;
static size_t param_byte_offset = 0;
static size_t local_byte_offset = 0;
static size_t total_local_bytes = 0;
static bool start_at_param_bytes = false;
Stack* stack;

void init_symbol_stack() {
	stack = create_stack();
	if (!stack) {
		perror("Failed to initialize global symbol stack\n");
		exit(EXIT_FAILURE);
	}
} 

bool push_scope() {
	if (stack->size >= stack->capacity) {
		stack->capacity *= 2;
		stack->tables = realloc(stack->tables, stack->capacity);
		if (!stack->tables) {
			perror("Unable to reallocate stack tables\n");
			return false;
		}
	}

	stack->tables[++stack->top] = create_symbol_table();
	if (!stack->tables[stack->top]) {
		perror("Unable to create new symbol table\n");
		return false;
	}

	stack->size++;
	return true;
}

bool pop_scope() {
	if (!is_stack_empty()) {
		stack->tables[stack->top--];
		stack->size--;
		return true;
	}

	return false;
}

struct type* type_copy(struct type* original_type) {
	struct type* duplicate_type = malloc(sizeof(struct type));
	if (!duplicate_type) {
		perror("Unable to allocate space for type\n");
		return NULL;
	}

	printf("Type we're copying is %d\n", original_type->kind);

	duplicate_type->kind = original_type->kind;
	duplicate_type->type_free = false;
	if (original_type->subtype) {
		duplicate_type->subtype = type_copy(original_type->subtype);
		if (!duplicate_type->subtype) {
			perror("Unable to copy subtype\n");
			free(duplicate_type);
			return NULL;
		}
	} else {
		duplicate_type->subtype = NULL;
	}
	printf("About to return\n");
	return duplicate_type;
}

Symbol* create_symbol(symbol_t kind, char* name, struct type* t) {
	Symbol* sym = malloc(sizeof(Symbol));
	if (!sym) {
		perror("Error: Unable to allocate space for symbol\n");
		return NULL;
	}

	sym->kind = kind;
	if (name) {
		sym->name = strdup(name);
		if (!sym->name) {
			perror("Unable to duplicate name\n");
			free(sym);
			return NULL;
		} else {
			printf("Got name\n");
		}
	} else {
		printf("name is NULL\n");
		sym->name = NULL;
	}

	printf("oh here\n");
	if (t) {
		sym->type = type_copy(t);
		if (!sym->type) {
			printf("'sym->type' is NULL\n");
			free(sym->name);
			free(sym);
			return NULL;
		}		
	} else {
		printf("'%s' has type NULL\n", sym->name);
		sym->type = NULL;
	}
	printf("after type\n");
	printf("here\n");

	sym->symbol_free = false;
	sym->bind_symbol = false;
	sym->array_size = 0;
	printf("About to create symbol with name '%s'\n", name);

	return sym;
}

SymbolTable* create_symbol_table() {
	SymbolTable* table = malloc(sizeof(SymbolTable));
	if (!table) {
		perror("Error: Unable to allocate space for symbol table\n");
		return NULL;
	}

	table->level = scope_level++;
	table->size = 0;
	table->capacity = TABLE_CAPACITY;
	table->symbols = calloc(table->capacity, sizeof(Symbol*));
	
	table->symboltable_free = false;
	return table;
}

Stack* create_stack() {
	Stack* stack = malloc(sizeof(Stack));
	if (!stack) {
		perror("Error: Unable to allocate space for stack\n");
		return NULL;
	}

	stack->top = -1;
	stack->size = 0;
	stack->capacity = STACK_CAPACITY;
	stack->tables = malloc(sizeof(SymbolTable*) * stack->capacity);
	if (!stack->tables) {
		perror("Unable to allocate space for stack tables\n");
		free(stack);
		return NULL;
	}

	return stack;
}

bool is_stack_empty() {
	return stack->top == -1;
}

bool scope_bind(Symbol* symbol, int hash_key) {
	if (!symbol) return false;

	SymbolTable* table = stack->tables[stack->top];
	if (table) {
		printf("got table\n");
	}
	
	if (table->symbols) {
		if (table->symbols[hash_key]) {
			symbol->link = table->symbols[hash_key];
			table->symbols[hash_key] = symbol;
			printf("Just bound '%s' to scope level %d\n", symbol->name, stack->top);
			return true;			
		} else {
			table->symbols[hash_key] = symbol;
			printf("Just bound '%s' to scope level %d\n", symbol->name, stack->top);
			return true;
		}
	} 
	return false;
}

int hash(char* name) {
	unsigned int hash = 0;
	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % TABLE_CAPACITY;
}

Symbol* scope_lookup(Symbol* symbol, int hash_key) {
	if (!symbol) return NULL;

	for (int i = stack->top - 1; i >= 0; i--) {
		SymbolTable* table = stack->tables[i];

		Symbol* sym = table->symbols[hash_key];
		while (sym) {
			Node* next = sym->link; 
			if (strcmp(sym->name, symbol->name) == 0) {
				return sym;
			}
			sym = next;
		}			
	}
	
	return NULL;
}

Symbol* scope_lookup_current(Symbol* symbol, int hash_key) {
	if (!symbol) return NULL;

	printf("in scope lookup current -> Current scope level is %d\n", stack->top);
	SymbolTable* current_table = stack->tables[stack->top];
	if (!current_table) {
		printf("Current table is NULL\n");
		return NULL;
	}
	printf("About to traverse link\n");
	
	Symbol* sym = current_table->symbols[hash_key];
	Symbol* current = sym;
	while (current) {
		Symbol* next = sym->link;
		if (strcmp(current->name, symbol->name) == 0) {
			printf("Found '%s'\n", current->name);
			return current;
		}
		current = next;
	}

	return NULL;
}

size_t get_num_bytes(Symbol* symbol) {
	if (!symbol) return 0;

	size_t size = 0;
	printf("in get num bytes\n");
	if (symbol->type) {
		switch (symbol->type->kind) {
			case TYPE_INTEGER: {
				printf("in int case\n");
				size = sizeof(int);
				return size;
			}

			case TYPE_CHAR:
			case TYPE_BOOL: {
				printf("in bool or char case\n");
				size = sizeof(char);
				return size;
			}

			case TYPE_ARRAY: {
				printf("in array case\n");
				if (symbol->type->subtype) {
					size_t array_size = 0;
					if (symbol->type->subtype->kind == TYPE_INTEGER) {
						array_size = symbol->array_size;
						size = array_size * sizeof(int);
					} else {
						size = array_size * sizeof(char); 
					}
				}
				return size;
			}
		}
	}
	printf("im here\n");
	return size;
}

void resolve_params(Node* params) {
	if (!stack || !params) return;

	int param_hash_key;
	data_t type = params->right->t->kind;
	
	switch (type) {
		case TYPE_INTEGER: {
			param_hash_key = hash(params->right->value.name);
			printf("Parameter hash key: '%d'\n", param_hash_key);
			params->right->symbol = create_symbol(SYMBOL_PARAM, params->right->value.name, params->right->t);

			if (scope_bind(params->right->symbol, param_hash_key)) {
				printf("Bound symbol '%s' to scope level ('%d')\n", params->right->symbol->name, stack->top);
			}
			printf("In param int case now\n");
			size_t size = sizeof(int);

			if (param_byte_offset == 0) {
				param_byte_offset = size;
			} else {
				param_byte_offset += size;
			}
			printf("made it here\n");
			params->right->symbol->param_byte_offset = param_byte_offset;
			params->right->symbol->actual_bytes = size;
			printf("\033[31mParameter: '%s' -> Offset: %d\033[0m\n", params->right->symbol->name, params->right->symbol->param_byte_offset);
			break;
		}

		case TYPE_BOOL:
		case TYPE_CHAR: {
			param_hash_key = hash(params->right->value.name);

			params->right->symbol = create_symbol(SYMBOL_PARAM, params->right->value.name, params->right->t);
			scope_bind(params->right->symbol, param_hash_key);

			size_t size = sizeof(char);

			if (param_byte_offset == 0) {
				param_byte_offset = size;
			} else {
				param_byte_offset += size;
			}

			params->symbol->param_byte_offset = param_byte_offset;
			params->symbol->actual_bytes = size;
			break;
		}
	}

}

Symbol* create_array_symbol(symbol_t kind, char* name, int element_count, struct type* type) {
	Symbol* sym = malloc(sizeof(Symbol));
	if (!sym) {
		perror("Error: Unable to allocate space for symbol\n");
		return NULL;
	}
	printf("IN CREATE ARRAY SYMBOL\n");
	sym->kind = kind;
	sym->array_size = element_count;
	
	printf("Number of elements is: %d\n", sym->array_size);
	printf("here\n");

	if (name) {
		printf("About to duplicate name\n");
		sym->name = strdup(name);
		printf("Duplicate\n");
		if (!sym->name) {
			perror("Unable to duplicate name for sym member\n");
			free(sym);
			return NULL;
		} else {
			printf("successful duplication of name\n");
		}
	} else {
		sym->name = NULL;
	}
	printf("About to check type\n");

	if (type) {
		sym->type = copy_type(type);
		printf("Failure\n");
		if (!sym->type) {
			free(sym->name);
			free(sym);
			return NULL;
		} else {
			printf("Successfully copied type\n");
		}
	} else {
		sym->type = NULL;
	}
	printf("now\n");

	sym->symbol_free = false;
	sym->bind_symbol = false;
	printf("yu\n");
	printf("uy\n");
	return sym;
}

data_t get_kind(struct type* t) {
	if (t->kind) {
		switch (t->kind) {
			case TYPE_INTEGER: return TYPE_INTEGER;
			case TYPE_CHAR: return TYPE_CHAR;
			case TYPE_BOOL: return TYPE_BOOL;
			case TYPE_VOID: return TYPE_VOID;
			case TYPE_ARRAY: return TYPE_ARRAY;
			case TYPE_POINTER: return TYPE_POINTER;
			default: return TYPE_UNKNOWN;
		}
	}
}

static bool in_return = false;
void resolve_expression(Node* node) {
	static bool op_child = false;
	static bool in_aug = false;
	static bool in_call = false;

	if (!node) return;

	size_t size;
	int hash_key;
	printf("Now im here\n");
	if (node->type) {
		printf("about to go to switch stmt\n");
		switch (node->type) {
			printf("In switch\n");
			// will do NODE_ADDR after adding support for pointers

			case NODE_INTEGER: { break; }

			case NODE_LOGICAL_AND:
			case NODE_LOGICAL_OR:
			case NODE_NOT_EQUAL:
			case NODE_EQUAL:
			case NODE_GREATER_EQUAL:
			case NODE_LESS_EQUAL:
			case NODE_GREATER:
			case NODE_LESS:
			case NODE_DIV_EQUAL:
			case NODE_MUL_EQUAL:
			case NODE_SUB_EQUAL:
			case NODE_ADD_EQUAL:
			case NODE_DIV:
			case NODE_MUL:
			case NODE_SUB:
			case NODE_ADD: {
				op_child = true;
				if (node->left) {
					resolve_expression(node->left);
				}

				if (node->right) {
					printf("About to resolve right child of op\n");
					resolve_expression(node->right);
				}
				op_child = false;
				break;
			}

			case NODE_NOT: {
				if (node->right) {
					resolve_expression(node->right);
				}
				break;
			}

			case NODE_DECREMENT:
			case NODE_INCREMENT: {
				op_child = true;
				if (node->left) {
					printf("in NODE_INCREMENT OR DECREMENT\n");
					resolve_expression(node->left);
					printf("LEAVING NODE INCREMENT OR DECREMENT\n");
				}
				op_child = false;
				break;
				
			}

			case NODE_CALL: {
				in_call = true;
				if (node->left) {
					printf("resolving calling function name\n");
					resolve_expression(node->left);
					printf("resolved calling function name\n");
				}

				if (node->right) {
					printf("About to resolve arguments\n");
					Node* arg = node->right;
					while (arg) {
						Node* next = arg->next;
						resolve_expression(arg);
						arg = next;	
					}
				}
				in_call = false;
				break;
			}

			case NODE_ARG: {
				printf("In node arg\n");
				if (node->right) {
					resolve_expression(node->right);
				}

				break;
			}

			case NODE_NAME: {
				printf("In node name case in RESOLVE EXPRESSION\n");
				hash_key = hash(node->value.name);
				if (node->t) {
					data_t kind = get_kind(node->t);
					printf("array\n");
					if (kind == TYPE_ARRAY) {
						printf("hellon\n");
						if (node->value.name) {
							printf("Name is '%s'\n", node->value.name);
							printf("Element count is: %d\n", node->right->value.val);
						}
						int count = node->right->value.val;
						node->symbol = create_array_symbol(SYMBOL_LOCAL, node->value.name, count, node->t);
						printf("sdasdas\n");
					} else {
						printf("heeee\n");
						node->symbol = create_symbol(SYMBOL_LOCAL, node->value.name, node->t);
					}
				}
				printf("got here\n");
				if (in_return || op_child || in_aug || in_call) {
					Symbol* temp_symbol = create_symbol(SYMBOL_LOCAL, node->value.name, NULL);
					if (!temp_symbol) {
						printf("temp symbol is NULL\n");
						return;
					}
					printf("Current scope level: %d\n", stack->top);
					Symbol* sym = scope_lookup_current(temp_symbol, hash_key);
					if (sym) {
						printf("sym name is '%s'\n", sym->name);
					}
					printf("here\n");
					if (!sym) {
						printf("In NODE_NAME: Did not find symbol in current scope, looking through remaining scopes\n");
						sym = scope_lookup(temp_symbol, hash_key);
						if (!sym) {
							printf("Did not find symbol in the remaining scopes\n");
							return;
						} else {
							printf("Found symbol '%s' in remaining scopes\n", sym->name);
							free_symbol(temp_symbol);
							node->symbol = sym;
							return;
						}
					} else {
						printf("no im here\n");
						printf("here i am\n");
						free_symbol(temp_symbol);
						node->symbol = sym;
						return;
					}
				} else {
					size = get_num_bytes(node->symbol);
					if (total_local_bytes == 0) {
						total_local_bytes = size;
					} else {
						total_local_bytes += size;
					}
					printf("Total local bytes: %zu\n", total_local_bytes);
					if (!start_at_param_bytes && param_byte_offset > 0) {
						local_byte_offset = param_byte_offset;
						local_byte_offset += size;
						start_at_param_bytes = true;
					} else {
						local_byte_offset += size;
					}
					printf("hello\n");
					node->symbol->local_byte_offset = local_byte_offset;
					printf("heras\n");
					node->symbol->actual_bytes = size;
					printf("here\n");
				}

				printf("\033[31mSymbol: '%s' -> Size: %zu\033[0m\n", node->symbol->name, node->symbol->actual_bytes);
				printf("we here\n");
				break;
			}

			case NODE_DECL: {
				if (node->left) {
					int hash_key = hash(node->left->value.name);
					resolve_expression(node->left);
					if (node->left->symbol) {
						Symbol* sym = scope_lookup_current(node->left->symbol, hash_key);
						if (!sym) {
							printf("No redeclaration!\n");	
							node->left->symbol = sym;
							scope_bind(node->left->symbol, hash_key);
						} else {
							printf("Redeclaration!\n");
							return;
						}		
					}
				}
				break;
			}

			case NODE_DEF: {
				printf("in node def for '%s'\n", node->left->value.name);
				if (node->left) {
					int hash_key = hash(node->left->value.name);
					resolve_expression(node->left);
					printf("back from call\n");
					if (node->left->symbol) {
						Symbol* sym = scope_lookup_current(node->left->symbol, hash_key);
						if (sym) {
							printf("Two definitions with same name in the same scope\n");
							return;
						} else {
							if (!scope_bind(node->left->symbol, hash_key)) {
								printf("unable to bind symbol to scope level %d\n", stack->top);
								return;
							}

						}
					}
				}
				break;
			}

			case NODE_AUG: {
				in_aug = true;
				printf("IN NODE AUG\n");
				if (node->left) {
					printf("we have left\n");					
					resolve_expression(node->left);
				}
				printf("got to end\n");
				in_aug = false;
				break;
			}

			case NODE_SUBSCRIPT: {
				printf("we have subscript\n");
				if (node->left) {
					resolve_expression(node->left);	
				}

				if (node->right) {
					resolve_expression(node->right);
				}

				break;
			}
		}
	}
	printf("leaving resolve expression\n");
}

void resolve_statement(Node* node) {
	if (!node) return;

	printf("in resolve statement\n");
	switch (node->type) {
		case NODE_WHILE:
		case NODE_IF:
		case NODE_ELSE_IF: {
			push_scope();
			if (node->left) {
				resolve_expression(node->left);
			}

			if (node->right) {
				resolve_statement(node->right);
			} 
			
			pop_scope();
			break;
		}

		case NODE_ELSE: {
			push_scope();
			if (node->right) {
				resolve_statement(node->right);
			}
			pop_scope();
			break;
		}

		case NODE_FOR: {
			push_scope();

			if (node->left) {
				printf("\033[31mResolving initializer\033[0m\n");
				resolve_statement(node->left);
				printf("\033[32mFinished Resolving initializer\033[0m\n");
			}

			Node* condition = node->left->next;
			if (condition) {
				printf("\033[32mType of condition is: %d\033[0m\n", condition->type);
				resolve_expression(condition);
			}

			Node* update = condition->next;
			if (update) {
				printf("\033[32mType on update node is: %d\033[0m\n", update->type);
				resolve_expression(update);
			}

			if (node->right) {
				resolve_statement(node->right);
			}

			pop_scope();
			break;
		}
		
		case NODE_RETURN: {
			in_return = true;
			printf("\033[31mIN RETURN STATEMENT\033[0m\n");
			if (node->right) {
				resolve_expression(node->right);
			}
			printf("back in node return\n");
			in_return = false;
			break;
		}

		case NODE_ASSIGNMENT: {
			if (node->left) {
				if (node->type) {
					printf("\033[1;31mAssignment node has type: %d\033[0m\n", node->type);
				}
				if (node->left->type) {
					printf("\033[1;31mLeft node of assignment node has type: %d\033[0m\n", node->left->type);
				}
				resolve_expression(node->left);
				printf("resolved left node\n");
			}

			if (node->right) {
				printf("Attempting to resolve right node for assignment\n");
				resolve_expression(node->right);
				printf("resolved right node\n");
			}

			break;
		}

		case NODE_BLOCK: {
			if (node->right) {
				Node* stmt = node->right;
				while (stmt) {
					Node* stmt_next = stmt->next;
					resolve_statement(stmt);
					stmt = stmt_next;
				}
			}

			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			if (node->left) {
				resolve_expression(node->left);
			}
			break;
		}
		case NODE_BREAK:
		case NODE_CONTINUE: { break; } 

	}
	printf("leaving resolve statement\n");
}

void resolve_globals(Node* node) {
	int hash_key;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			hash_key = hash(node->left->value.name);
			printf("Hash key is '%d'\n", hash_key);
			Symbol* sym = create_symbol(SYMBOL_GLOBAL, node->left->value.name, node->left->t);		
			if (scope_lookup_current(sym, hash_key)) {
				printf("Reclaration error\n");
				return;
			} else {
				node->left->symbol = sym;
				if (scope_bind(node->left->symbol, hash_key)) {
					printf("Bound symbol '%s' to scope level ('%d')\n", node->left->symbol->name, stack->top);
				}				
			}

			break;
		}

		case NODE_NAME: {
			if (node->t->kind == TYPE_FUNCTION) {
				hash_key = hash(node->value.name);

				node->symbol = create_symbol(SYMBOL_GLOBAL, node->value.name, node->t);
				if (scope_bind(node->symbol, hash_key)) {
					printf("Bound symbol '%s' to scope level ('%d')\n", node->symbol->name, stack->top);
				}

				size_t total_param_bytes = 0;
				push_scope();

				if (node->left) {
					Node* param = node->left;
					while (param) {
						Node* next = param->next;
						resolve_params(param);
						total_param_bytes += param->right->symbol->actual_bytes;
						param = next;
					}
				}
				printf("\033[33mTotal param bytes: %zu\033[0m\n", total_param_bytes);

				if (node->right) {
					Node* block = node->right;
					Node* stmt = block->right;
					while (stmt) {
						Node* next = stmt->next;
						resolve_statement(stmt);
						stmt = next;
					}
				}

				pop_scope();

				printf("Total local bytes: %d\n", total_local_bytes);
				node->symbol->total_bytes = total_param_bytes + total_local_bytes;
				printf("Total local bytes for '%s' before alignment: %d\n", node->symbol->name, node->symbol->total_bytes);

				if ((node->symbol->total_bytes % 32) != 0) {
					node->symbol->total_bytes += 32 - (node->symbol->total_bytes % 32);
				}

				printf("Total local bytes for '%s' after alignment: %d\n", node->symbol->name, node->symbol->total_bytes);


				param_byte_offset = 0;
				total_local_bytes = 0;
				start_at_param_bytes = false;
			} 
			break;
		}
	}
}

void resolve_tree(Node* root) {
	if (!root) return;

	init_symbol_stack();

	push_scope();
	
	Node* current = root;
	while (current) {
		Node* next = current->next;
		resolve_globals(current);
		current = next;
	}

	pop_scope();

	free_stack(stack);
}

void free_symbol_type(struct type* type) {
	if (!type) return;
	if (type->subtype) free_symbol_type(type->subtype);
	free(type);
}

void free_symbol(Symbol* symbol) {
	if (!symbol || symbol->symbol_free) return;

	if (symbol->name) free(symbol->name);
	if (symbol->type) free_symbol_type(symbol->type);
	free(symbol);
}

void free_table(SymbolTable* table) {
	if (!table || table->symboltable_free) return;

	table->symboltable_free = true;
	for (int i = 0; i < table->size; i++) {
		free_symbol(table->symbols[i]);
	}
	free(table);
}

void free_stack(Stack* stack) {
	if (!stack) return;

	for (int i = stack->top; i >= 0; i--) {
		free_table(stack->tables[i]);
	}

	free(stack);
}