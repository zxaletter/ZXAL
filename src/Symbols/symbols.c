#include "symbols.h"

static int scope_level = 0;
static size_t param_byte_offset = 0;
static size_t local_byte_offset = 0;
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

Symbol* create_symbol(symbol_t kind, char* name, struct type* type) {
	Symbol* sym = malloc(sizeof(Symbol));
	if (!sym) {
		perror("Error: Unable to allocate space for symbol\n");
		return NULL;
	}

	sym->kind = kind;
	sym->name = strdup(name);
	if (!sym->name) {
		perror("Unable to duplicate name for sym member\n");
		free(sym);
		return NULL;
	}

	sym->type = type;
	sym->symbol_free = false;

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
	table->symbols = malloc(sizeof(Symbol*) * table->capacity);
	if (!table->symbols) {
		perror("Unable to allocate space for symbols\n");
		free(table);
		return NULL;
	}
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
	if (!stack || !symbol) return false;

	SymbolTable* table = stack->tables[stack->top];
	if (table) {
		printf("got table\n");
	}

	if (table->symbols[hash_key]) {
		printf("no im here\n");
		symbol->link = table->symbols[hash_key];
		table->symbols[hash_key] = symbol;
		return true;
	} else {
		printf("im in here\n");
		table->symbols[hash_key] = symbol;
		printf("in here\n");
		return true;
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

bool scope_lookup(Symbol* symbol, int hash_key) {
	if (!symbol) return false;

	if (!scope_lookup_current(symbol, hash_key)) {
		for (int i = stack->top - 1; i >= 0; i--) {
			SymbolTable* table = stack->tables[i];
			Symbol* sym = table->symbols;
			while (sym) {
				Symbol* next = sym->next;
				if (sym->link) {
					Symbol* current_link = sym->link;
					while (current_link) {
						Symbol* current_link_next = current_link->next;
						if (strcmp(current_link->name, symbol->name) == 0) {
							return true;
						}
						current_link = current_link_next;
					}
				}

				if (strcmp(sym->name, symbol->name) == 0) {
					return true;
				}

				sym = next;
			}
		}
	}
	return false;
}

bool scope_lookup_current(Symbol* symbol, int hash_key) {
	if (!symbol) return false;

	Symbol* current_sym = stack->tables[stack->top]->symbols[hash_key];
	while (current_sym) {
		Symbol* next = current_sym->next;
		if (strcmp(current_sym->name, symbol->name) == 0) {
			printf("Redeclaration Error for '%s'\n", symbol->name);
			return false;
		}
		current_sym = next;
	}

	return false;
}

void resolve_params(Node* params) {
	if (!stack || !params) return;
	
	push_scope();

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
			break;
		}
	}

	pop_scope();
}

void resolve_expression(Node* node) {
	if (!node) return;

	int hash_key;

	switch (node->type) {
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
			if (node->left) {
				resolve_expression(node->left);
			}

			if (node->right) {
				resolve_expression(node->right);
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

		case NODE_CALL: {
			if (node->left) {
				resolve_expression(node->left);
			}

			if (node->right) {
				Node* arg = node->right;
				while (arg) {
					Node* next = arg->next;
					resolve_expression(arg);
					arg = next;	
				}
			}
			break;
		}

		case NODE_ARG: {
			if (node->right) {
				int hash_key = hash(node->right->value.name);
				if (node->right->t) {
					node->right->symbol = create_symbol(SYMBOL_LOCAL, node->right->value.name, node->right->t);
				}
				scope_bind(node->right->symbol, hash_key);
			}

			break;
		}

		case NODE_NAME: {
			int hash_key = hash(node->value.name);
			node->symbol = create_symbol(SYMBOL_LOCAL, node->value.name, node->t);

			if (scope_lookup_current(node->symbol, hash_key)) {
				printf("Redeclaration error\n");
				return;
			}

			if (!scope_lookup(node->symbol, hash_key)) {
				printf("Undefined varialbe\n");
				return;
			}
			if (scope_bind(node->symbol, hash_key)) {
				printf("Bound symbol '%s' to scope level ('%d')\n", node->symbol->name, stack->top);
			}

			size_t size = 0;
			if (node->t->kind == TYPE_INTEGER) {
				size = sizeof(int);
			} else if (node->t->kind == TYPE_CHAR || node->t->kind == TYPE_BOOL) {
				size = sizeof(char);
			} else if (node->t->kind == TYPE_ARRAY) {
				if (node->left && node->t->subtype && node->t->subtype->kind == TYPE_INTEGER) {
					size_t array_size = node->left->value.val;
					size = array_size * sizeof(int);
				} else {
					size_t array_size = node->left->value.val;
					size = array_size * sizeof(char);
				}
			}

			if (!start_at_param_bytes && param_byte_offset > 0) {
				local_byte_offset = param_byte_offset;
				local_byte_offset += size;
				start_at_param_bytes = true;
			} else {
				local_byte_offset += size;
			}

			node->symbol->local_byte_offset = local_byte_offset;
			break;
		}

		case NODE_SUBSCRIPT: {
			if (node->left) {
				int hash_key = hash(node->left->value.name);
				node->left->symbol = create_symbol(SYMBOL_LOCAL, node->left->value.name, hash_key);
				scope_bind(node->left->symbol, hash_key);
			}

			if (node->right && node->right->type == NODE_NAME) {
				if (node->right->t && node->right->t->kind == TYPE_INTEGER) {
					int index_key = hash(node->right->value.name);
					node->right->symbol = create_symbol(SYMBOL_LOCAL, node->right->value.name, index_key);
					scope_bind(node->right->symbol, index_key);					
				}
			}

			break;
		}


	}

}

void resolve_statement(Node* node) {
	if (!node) return;

	switch (node->type) {
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
				resolve_statement(node->left);
			}

			Node* condition = node->left->next;
			if (condition) {
				resolve_expression(condition);
			}

			Node* update = condition->next;
			if (update) {
				resolve_expression(update);
			}

			if (node->right) {
				resolve_statement(node->right);
			}

			pop_scope();
			break;
		}

		case NODE_WHILE: {
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
		
		case NODE_RETURN: {
			if (node->right) {
				resolve_expression(node->right);
			}
			break;
		}

		case NODE_ASSIGNMENT: {
			if (node->left) {
				resolve_expression(node->left);
			}

			if (node->right) {
				resolve_expression(node->right);
			}

			break;
		}

		case NODE_BREAK:
		case NODE_CONTINUE: { break; } 

	}
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
				printf("Hash key is '%d'\n", hash_key);

				node->symbol = create_symbol(SYMBOL_GLOBAL, node->value.name, node->t);
				if (scope_bind(node->symbol, hash_key)) {
					printf("Bound symbol '%s' to scope level ('%d')\n", node->symbol->name, stack->top);
				}

				size_t total_param_bytes = 0;

				if (node->left) {
					Node* param = node->left;
					while (param) {
						Node* next = param->next;
						resolve_params(param);
						total_param_bytes += param->right->symbol->param_byte_offset;
						param = next;
					}
				}

				size_t total_local_bytes = 0;

				if (node->right) {
					Node* stmt = node->right;
					push_scope();
					while (stmt) {
						Node* next = stmt->next;
						resolve_statement(stmt);
						total_local_bytes += stmt->symbol->local_byte_offset;
						stmt = next;
					}
					pop_scope();
				}

				node->symbol->total_byte_offset = total_param_bytes + total_local_bytes;
				printf("Total local bytes for '%s' before alignment: %d\n", node->symbol->name, node->symbol->total_byte_offset);

				if ((node->symbol->total_byte_offset % 64) != 0) {
					node->symbol->total_byte_offset += 64 - (node->symbol->total_byte_offset % 64);
				}

				printf("Total local bytes for '%s' after alignment: %d\n", node->symbol->name, node->symbol->total_byte_offset);


				param_byte_offset = 0;
				local_byte_offset = 0;
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