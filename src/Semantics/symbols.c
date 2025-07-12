#include "symbols.h"

static int scope_level = 0;
static size_t param_byte_offset = 0;
static size_t local_byte_offset = 0;
static size_t total_local_bytes = 0;
static bool start_at_param_bytes = false;

SymbolStack* symbol_stack = NULL;
ContextStack* context_stack = NULL;

void init_context_stack(CompilerContext* ctx) {
	context_stack = create_context(ctx);
	if (!context_stack) {
		perror("Failed to initialize global context stack\n");
		// free_symbol_stack();
		exit(EXIT_FAILURE);
	}
}

bool is_context_stack_empty() {
	return context_stack->top == -1;
}

ContextStack* create_context(CompilerContext* ctx) {
	ContextStack *context_stack = arena_allocate(ctx->symbol_arena, sizeof(ContextStack));
	if (!context_stack) {
		perror("Unable to allocate space for context stack\n");
		return NULL;
	}

	context_stack->top = -1;
	context_stack->size = 0;
	context_stack->capacity = CONTEXT_CAPACITY;
	context_stack->contexts = arena_allocate(ctx->symbol_arena, sizeof(context_t) * context_stack->capacity);
	if (!context_stack->contexts) {
		perror("Unable to allocate space for context types\n");
		return NULL;
	}

	return context_stack;
}

void push_context(CompilerContext* ctx, context_t new_context) {
	if (context_stack->size >= context_stack->capacity) {
		size_t prev_capacity = context_stack->capacity;
		
		context_stack->capacity *= 2;
		size_t new_capacity = context_stack->capacity;
		void* new_contexts = arena_reallocate(
			ctx->symbol_arena, 
			context_stack->contexts, 
			prev_capacity * sizeof(context_t), 
			new_capacity* sizeof(context_t)
		);
		
		if (!new_contexts) {
			perror("Unable to reallocate space for context types\n");
		}
		context_stack->contexts = new_contexts;
	}

	context_stack->contexts[++context_stack->top] = new_context;
	context_stack->size++;
}

void pop_context() {
	if (!is_context_stack_empty()) {
		context_stack->contexts[context_stack->top--];
		context_stack->size--;
	}
}

bool context_lookup(context_t context) {
	for (int i = context_stack->top; i >= 0; i--) {
		context_t type = context_stack->contexts[i];
		if (type == context) { 
			return true; 
		} 
	}
	return false;
}

void init_symbol_stack(CompilerContext* ctx) {
	symbol_stack = create_stack(ctx);
	if (!symbol_stack) {
		perror("Failed to initialize global symbol stack\n");
		exit(EXIT_FAILURE);
	}
} 

bool push_scope(CompilerContext* ctx) {
	if (symbol_stack->size >= symbol_stack->capacity) {
		size_t prev_capacity = symbol_stack->capacity;
		
		symbol_stack->capacity *= 2;
		size_t new_capacity = symbol_stack->capacity;
		void* new_tables = arena_reallocate(
			ctx->symbol_arena, 
			symbol_stack->tables, 
			prev_capacity * sizeof(SymbolTable*), 
			new_capacity * sizeof(SymbolTable*)
		);

		if (!new_tables) {
			perror("Unable to reallocate stack tables\n");
			return false;
		}
		symbol_stack->tables = new_tables;
	}

	symbol_stack->tables[++symbol_stack->top] = create_symbol_table(ctx);
	if (!symbol_stack->tables[symbol_stack->top]) {
		perror("Unable to create new symbol table\n");
		return false;
	}

	symbol_stack->size++;
	return true;
}

bool pop_scope() {
	if (!is_stack_empty()) {
		symbol_stack->top--;
		symbol_stack->size--;
		return true;
	}

	return false;
}

Symbol* create_symbol(CompilerContext* ctx, symbol_t kind, char* name, struct type* t) {
	// Symbol* sym = malloc(sizeof(Symbol));
	Symbol* sym = arena_allocate(ctx->symbol_arena, sizeof(Symbol));
	if (!sym) {
		perror("Error: Unable to allocate space for symbol\n");
		return NULL;
	}

	sym->kind = kind;
	sym->symbol_free = false;
	sym->bind_symbol = false;
	sym->name = NULL;
	sym->type = NULL;
	sym->link = NULL;
	sym->next = NULL;
	sym->local_byte_offset = 0;
	sym->total_bytes = 0;
	sym->param_byte_offset = 0;
	sym->actual_bytes = 0;

	if (name) {
		// sym->name = strdup(name);
		sym->name = arena_allocate(ctx->symbol_arena, strlen(name) + 1);
		if (!sym->name) {
			perror("Unable to duplicate name\n");
			// free(sym);
			return NULL;
		} 
		strcpy(sym->name, name);
	} 

	if (t) {
		sym->type = t;
		if (!sym->type) {
			printf("Error: 'sym->type' is NULL after type_copy for symbol '%s'\n", sym->name ? sym->name : "NULL_NAME");
			// if (name) free(sym->name);
			// free(sym);
			return NULL;
		}		
	}
	return sym;
}

Symbol* create_array_symbol(CompilerContext* ctx, symbol_t kind, char* name, int element_count, struct type* type) {
	// Symbol* sym = malloc(sizeof(Symbol));
	Symbol* sym = arena_allocate(ctx->symbol_arena, sizeof(Symbol));
	if (!sym) {
		perror("Error: Unable to allocate space for symbol\n");
		return NULL;
	}

	sym->kind = kind;
	sym->symbol_free = false;
	sym->bind_symbol = false;
	sym->name = NULL;
	sym->type = NULL;
	sym->link = NULL;
	sym->next = NULL;
	sym->local_byte_offset = 0;
	sym->total_bytes = 0;
	sym->param_byte_offset = 0;
	sym->actual_bytes = 0;
	sym->array_size = element_count;
	
	if (name) {
		// sym->name = strdup(name);
		sym->name = arena_allocate(ctx->symbol_arena, strlen(name) + 1);
		if (!sym->name) {
			perror("Unable to duplicate name for sym member\n");
			// free(sym);
			return NULL;
		}
		strcpy(sym->name, name);
	} 

	if (type) {
		sym->type = type;
		// if (!sym->type) {
		// 	printf("Unable to copy type for array symbol '%s'\n", sym->name ? sym->name : "NULL_NAME");
		// 	// if (sym->name) free(sym->name);
		// 	// free(sym);
		// 	return NULL;
		// } 
	}
	return sym;
}

SymbolTable* create_symbol_table(CompilerContext* ctx) {
	SymbolTable* table = arena_allocate(ctx->symbol_arena, sizeof(SymbolTable));
	if (!table) {
		perror("Error: Unable to allocate space for symbol table\n");
		return NULL;
	}

	table->level = scope_level++;
	table->size = 0;
	table->capacity = TABLE_CAPACITY;
	table->symbols = arena_allocate(ctx->symbol_arena, sizeof(Symbol*) * table->capacity);
	if (!table->symbols) {
		perror("Error: Unable to allocate space for symbol table hash array\n");
		return NULL;
	}

	for (int i = 0; i < table->capacity; i++) {
		table->symbols[i] = NULL;
	}

	table->symboltable_free = false;
	return table;
}

SymbolStack* create_stack(CompilerContext* ctx) {
	SymbolStack* symbol_stack = arena_allocate(ctx->symbol_arena, sizeof(SymbolStack));
	if (!symbol_stack) {
		perror("Error: Unable to allocate space for stack\n");
		return NULL;
	}

	symbol_stack->top = -1;
	symbol_stack->size = 0;
	symbol_stack->capacity = STACK_CAPACITY;
	symbol_stack->tables = arena_allocate(ctx->symbol_arena, sizeof(SymbolTable*) * symbol_stack->capacity);
	if (!symbol_stack->tables) {
		perror("Unable to allocate space for stack tables\n");
		return NULL;
	}

	return symbol_stack;
}

bool is_stack_empty() {
	return symbol_stack->top == -1;
}

bool scope_bind(CompilerContext* ctx, Symbol* symbol, int hash_key) {
	if (!symbol || (symbol && !symbol->name)) {
		printf("Error: Attempted to bind NULL symbol or symbol with NULL name.\n");
		return false;
	}

	SymbolTable* table = symbol_stack->tables[symbol_stack->top];
	if (!table) {
		printf("Error: Current symbol table is NULL in scope_bind.\n");
		return false;
	}

	if (table->size >= table->capacity) {
		int prev_capacity  = table->capacity;

		Symbol** temp_symbols = arena_allocate(ctx->symbol_arena, sizeof(Symbol*) * prev_capacity);
		if (!temp_symbols) return false;

		for (int i = 0; i < prev_capacity; i++) {
			temp_symbols[i] = table->symbols[i];
		}

		table->capacity *= 2;
		void** new_symbols = arena_reallocate(
			ctx->symbol_arena, 
			table->symbols, 
			prev_capacity * sizeof(Symbol*), 
			table->capacity * sizeof(Symbol*)
		);

		if (!new_symbols) return false;

		table->symbols = new_symbols;
		table->size = 0;
		for (int i = 0; i < table->capacity; i++) {
			table->symbols[i] = NULL;
		}

		for (int i = 0; i < prev_capacity; i++) {
			Symbol* current_sym = temp_symbols[i];
			while (current_sym) {
				Symbol* next_link = current_sym->link;
				
				int updated_hash_key = hash(table->capacity, current_sym->name);
				if (updated_hash_key == -1) return false;

				current_sym->link = table->symbols[updated_hash_key];
				table->symbols[updated_hash_key] = current_sym;

				current_sym = next_link;
			}
		}

		hash_key = hash(table->capacity, symbol->name);
	}

	if (table->symbols[hash_key]) {
		symbol->link = table->symbols[hash_key];
		table->symbols[hash_key] = symbol;

	} else {
		table->symbols[hash_key] = symbol;
		symbol->link = NULL;
		table->size++;
	}
	return true;
}

int hash(int table_capacity, char* name) {
	if (!name) return -1;
	int hash = 0;
	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % table_capacity;
}

Symbol* scope_lookup(Symbol* symbol, int hash_key) {
	if (!symbol || !symbol->name) return NULL;

	for (int i = symbol_stack->top; i >= 0; i--) {
		SymbolTable* table = symbol_stack->tables[i];
		if (!table || !table->symbols) continue;

		Symbol* sym = table->symbols[hash_key];
		Symbol* current = sym;
		while (current) { 
			if (current->name  && strcmp(current->name, symbol->name) == 0) {
				printf("Found symbol '%s' in scope level %d.\n", current->name, table->level);
				return current;
			}
			current = current->link;
		}			
	}
	printf("Symbol '%s' not found in any scope.\n", symbol->name);
	return NULL;
}

Symbol* scope_lookup_current(Symbol* symbol, int hash_key) {
	if (!symbol || !symbol->name) return NULL;

	SymbolTable* current_table = symbol_stack->tables[symbol_stack->top];
	if (!current_table || !current_table->symbols) {
		printf("Current table or its symbols array is NULL in scope_lookup_current\n");
		return NULL;
	}
	
	Symbol* sym = current_table->symbols[hash_key];
	Symbol* current = sym;
	while (current) {
		if (current->name && strcmp(current->name, symbol->name) == 0) {
			// printf("Found '%s' in current scope.\n", current->name);
			return current;
		}
		current = current->link;
	}
	printf("Symbol '%s' not found in current scope.\n", symbol->name);
	return NULL;
}

size_t get_num_bytes(Symbol* symbol) {
	if (!symbol || !symbol->type) return 0;

	size_t size = 0;
	switch (symbol->type->kind) {
		case TYPE_INTEGER: {
			size = sizeof(int);
			return size;
		}

		case TYPE_CHAR:
		case TYPE_BOOL: {
			size = sizeof(char);
			return size;
		}

		case TYPE_ARRAY: {
			if (symbol->type->subtype) {
				size_t element_size = 0;
				switch (symbol->type->subtype->kind) {
					case TYPE_INTEGER: element_size = sizeof(int); break;
					case TYPE_CHAR:
					case TYPE_BOOL: element_size = sizeof(char); break;
					default: 
						printf("Warning: Unknown array element type %d for symbol '%s'. Returning 0 bytes.\n", symbol->type->subtype->kind, symbol->name ? symbol->name : "N/A");
						return 0;
				}
				size = symbol->array_size * element_size;
			} else {
				printf("Warning: Array symbol '%s' has NULL subtype. Returning 0 bytes.\n", symbol->name ? symbol->name : "N/A");
				return 0;
			}
			return size;
		}
		case TYPE_FUNCTION:
		case TYPE_VOID:
		case TYPE_POINTER:
		case TYPE_STRUCT:
		case TYPE_UNKNOWN:
		default:
			printf("Warning: get_num_bytes: Unhandled or UNKNOWN type kind %d for symbol '%s'. Returning 0 bytes.\n", symbol->type->kind, symbol->name ? symbol->name : "N/A");
			return 0;
	}
	
	return size;
}


data_t get_kind(struct type* t) {
	if (t) {
		return t->kind;
	}
	return TYPE_UNKNOWN;
}

void resolve_expression(CompilerContext* ctx, Node* node) {
	if (!node || (node && !node->type)) return;

	size_t size;
	int hash_key;

	switch (node->type) {
		case NODE_CHAR: 
		case NODE_BOOL:
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
		case NODE_MODULO:
		case NODE_DIV:
		case NODE_MUL:
		case NODE_SUB:
		case NODE_ADD: {
			push_context(ctx, CONTEXT_OP);
			if (node->left) {
				resolve_expression(ctx, node->left);
			}

			if (node->right) {
				resolve_expression(ctx, node->right);
			}
			pop_context();
			break;
		}

		case NODE_UNARY_ADD:
		case NODE_UNARY_SUB:
		case NODE_NOT: {
			push_context(ctx, CONTEXT_OP);
			if (node->right) {
				resolve_expression(ctx, node->right);
			}
			pop_context();
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			push_context(ctx, CONTEXT_OP);
			if (node->left) {
				resolve_expression(ctx, node->left);
			}
			pop_context();
			break;
				
		}

		case NODE_ARRAY_LIST: {
			if (node->right) {
				Node* element = node->right;
				while (element) {
					resolve_expression(ctx, element);
					element = element->next;
				}
			}
			break;
		}

		case NODE_CALL: {
			push_context(ctx, CONTEXT_CALL);
			if (node->left) {
				resolve_expression(ctx, node->left);
			}

			if (node->right) {
				Node* arg = node->right;
				while (arg) {
					Node* next = arg->next;
					resolve_expression(ctx, arg);
					arg = next;	
				}
			}
			pop_context();
			break;
		}

		case NODE_ARG: {
			if (node->right) {
				resolve_expression(ctx, node->right);
			} 
			break;
		}

		case NODE_NAME: {
				// printf("\033[31mIN node name processing '%s'\033[0m\n", node->value.name ? node->value.name : "NULL_NAME_NODE");
				if (!node->value.name) {
					printf("Error: NODE_NAME has NULL name. Cannot resolve\n");
					return;
				}
				SymbolTable* current_table = symbol_stack->tables[symbol_stack->top];
				if (!current_table) return;
				
				hash_key = hash(current_table->capacity, node->value.name);
				Symbol* sym = NULL;
				if (node->t && !node->symbol) {
					data_t kind = get_kind(node->t);
					if (kind == TYPE_ARRAY) {
						int count = 0;
						if (node->right && node->right->type == NODE_INTEGER) {
							count = node->right->value.val;
						}
						sym = create_array_symbol(ctx, SYMBOL_LOCAL, node->value.name, count, node->t);

						node->symbol = sym;
					} else {
						sym = create_symbol(ctx, SYMBOL_LOCAL, node->value.name, node->t);
						node->symbol = sym;
					}

					if (!node->symbol) {
						printf("Error: Failed to create symbol for NOME_NAME '%s' (initial creation).\n", node->value.name);
						return;
					}

					if (!scope_bind(ctx, node->symbol, hash_key)) {
						printf("Error: Failed to bind newly created symbol '%s' during initial resolution.\n", node->value.name);
						// free_symbol(sym);
						return;
					}

					printf("Node '%s' symbol created and bound (initial decl/def).\n", node->value.name);

					if (node->symbol->kind == SYMBOL_LOCAL) {
						size = get_num_bytes(node->symbol);
						if (size > 0) {
							if (total_local_bytes == 0) { total_local_bytes = size; }
							else { total_local_bytes += size; }
							
							printf("Updated total local bytes for '%s': %zu\n", node->value.name, total_local_bytes);

							if (!start_at_param_bytes && param_byte_offset > 0) {
								local_byte_offset = param_byte_offset;
								local_byte_offset += size;
								start_at_param_bytes = true;
							} else { local_byte_offset += size; }

							node->symbol->local_byte_offset = local_byte_offset;
							printf("\033[31mLocal byte offset for '%s' is %zu\033[0m\n", node->symbol->name, node->symbol->local_byte_offset);
							node->symbol->actual_bytes = size;
						}
					}
					return;
				}

				if (context_lookup(CONTEXT_OP) || context_lookup(CONTEXT_IF) ||
				    context_lookup(CONTEXT_LOOP) || context_lookup(CONTEXT_RETURN) || 
				    context_lookup(CONTEXT_AUG) || context_lookup(CONTEXT_SUBSCRIPT) ||context_lookup(CONTEXT_CALL) ||
				    context_lookup(CONTEXT_ASSIGNMENT)) {

					
					Symbol* temp_symbol = create_symbol(ctx, SYMBOL_LOCAL, node->value.name, NULL);
					if (!temp_symbol) {
						return;
					}

					Symbol* sym = scope_lookup_current(temp_symbol, hash_key);
					if (!sym) {
						printf("In NODE_NAME: Did not find symbol in current scope, looking through remaining scopes\n");

						sym = scope_lookup(temp_symbol, hash_key);
						if (!sym) {
							printf("Did not find symbol in the remaining scopes\n");
							return;
						} else {
							printf("Found symbol '%s' in remaining scopes\n", sym->name);
							node->symbol = sym;
							node->t = sym->type;
							
							return;
						}
					} else {
						node->symbol = sym;
						node->t = sym->type;
					
						return;
					}
				} 

				break;
		}

		case NODE_DEF:
		case NODE_DECL: {
				if (node->left) {
					resolve_expression(ctx, node->left);
				}
				break;
		}

		case NODE_AUG: {
				push_context(ctx, CONTEXT_AUG);
				if (node->left) {
					resolve_expression(ctx, node->left);
					node->symbol = node->left->symbol;
				}
				pop_context();
				break;
		}

		case NODE_SUBSCRIPT: {
			push_context(ctx, CONTEXT_SUBSCRIPT);
			if (node->left) {
				resolve_expression(ctx, node->left);	
			}

			if (node->right) {
				resolve_expression(ctx, node->right);
			}

			if (node->left && node->left->symbol && node->left->symbol->type) {
				if (node->left->symbol->type->kind == TYPE_ARRAY) {
					node->symbol = node->left->symbol;
					node->t = node->left->symbol->type->subtype;
				} 
			}
			pop_context();
			break;
		}
	}
}

void resolve_statement(CompilerContext* ctx, Node* node) {
	if (!node || (node && !node->type)) return;

	switch (node->type) {
		case NODE_IF:
		case NODE_ELSE_IF: {
			push_context(ctx, CONTEXT_IF);
			push_scope(ctx);
			if (node->left) {
				resolve_expression(ctx, node->left);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			} 
			
			pop_scope();
			pop_context();
			break;
		}

		case NODE_ELSE: {
			push_scope(ctx);
			if (node->right) {
				resolve_statement(ctx, node->right);
			}
			pop_scope();
			break;
		}

		case NODE_WHILE: {
			printf("\033[32mIN NODE_WHILE CASE IN RESOLVE STATEMENT\033[0m\n");
			push_scope(ctx);
			push_context(ctx, CONTEXT_LOOP);

			if (node->left) {
				resolve_expression(ctx, node->left);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			}

			pop_scope();
			pop_context();
			break;
		}
		case NODE_FOR: {
			push_scope(ctx);
			push_context(ctx, CONTEXT_LOOP);

			if (node->left) {
				printf("\033[31mResolving initializer in for loop\033[0m\n");
				resolve_statement(ctx, node->left);
				printf("\033[32mFinished Resolving initializer in for loop\033[0m\n");
			}

			Node* condition = node->left ? node->left->next : NULL;
			if (condition) {
				printf("\033[32mType of condition is: %d\033[0m\n", condition->type);
				resolve_expression(ctx, condition);
			}

			Node* update = condition ? condition->next : NULL;
			if (update) {
				printf("\033[32mType on update node is: %d\033[0m\n", update->type);
				resolve_expression(ctx, update);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			}

			pop_scope();
			pop_context();
			break;
		}
		
		case NODE_RETURN: {
			// printf("\033[31mIN RETURN STATEMENT\033[0m\n");
			if (!node->right) {
				if (!context_lookup(CONTEXT_VOID_FUNCTION)) {
					return;
				}
			} else {
				push_context(ctx, CONTEXT_RETURN);
				if (!context_lookup(CONTEXT_NONVOID_FUNCTION)) {
					return;
				}

				resolve_expression(ctx, node->right);
				pop_context();
			}
			break;
		}

		case NODE_ASSIGNMENT: {
			if (node->left) {
				resolve_expression(ctx, node->left);
			}
			if (node->right) {
				push_context(ctx, CONTEXT_ASSIGNMENT);
				printf("\033[33mRight node of assignment has type %d\033[0m\n", node->right->type);
				resolve_expression(ctx, node->right);
				pop_context();
			}

			break;
		}

		case NODE_BLOCK: {
			if (node->right) {
				Node* stmt = node->right;
				while (stmt) {
					Node* stmt_next = stmt->next;
					resolve_statement(ctx, stmt);
					stmt = stmt_next;
				}
			}
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			if (node->left) {
				push_context(ctx, CONTEXT_OP);
				resolve_expression(ctx, node->left);
				pop_context();
			}
			break;
		}

		case NODE_BREAK: {
			if (!context_lookup(CONTEXT_LOOP) && !context_lookup(CONTEXT_SWITCH)) {
				exit(EXIT_FAILURE);
			}
			break;
		}
		case NODE_CONTINUE: { 
			if (!context_lookup(CONTEXT_LOOP)) {
				exit(EXIT_FAILURE);
			}
			break; 
		} 
		case NODE_CALL: {
			resolve_expression(ctx, node);
			break;
		}

	}
}

void resolve_params(CompilerContext* ctx, Node* wrapped_param) {
	if (!symbol_stack || !wrapped_param || 
		!wrapped_param->right || !wrapped_param->right->t || 
		!wrapped_param->right->value.name) {
		return;
	}

	SymbolTable* current_table = symbol_stack->tables[symbol_stack->top];
	if (!current_table) return;

	int param_hash_key = hash(current_table->capacity, wrapped_param->right->value.name);
	
	wrapped_param->right->symbol = create_symbol(ctx, SYMBOL_PARAM, wrapped_param->right->value.name, wrapped_param->right->t);
	if (!wrapped_param->right->symbol) {
		printf("Error: Failed to create symbol for parameters '%s'.\n", wrapped_param->right->value.name);
		return;
	}

	Symbol* existing_param = scope_lookup_current(wrapped_param->right->symbol, param_hash_key);
	if (existing_param) {
		printf("\033[31mError: Redefinition of parameter '%s' in funciton scope. Original defined at scope level %d.\033[0m\n",
			wrapped_param->right->symbol->name, scope_level);
		// free_symbol(wrapped_param->right->symbol);
		wrapped_param->right->symbol = NULL;
		return;
	}

	if (!scope_bind(ctx, wrapped_param->right->symbol, param_hash_key)) {
		// free_symbol(wrapped_param->right->symbol);
		return;
	}
	size_t size = get_num_bytes(wrapped_param->right->symbol);
	if (size > 0) {
		if (param_byte_offset == 0) { param_byte_offset = size; }
		else { param_byte_offset += size; }
		wrapped_param->right->symbol->param_byte_offset = param_byte_offset;
		wrapped_param->right->symbol->actual_bytes = size;
		printf("\033[31mParameter: '%s' -> Offset: %zu -> Actual Bytes: %zu\033[0m\n",
			wrapped_param->right->symbol->name, wrapped_param->right->symbol->param_byte_offset, wrapped_param->right->symbol->actual_bytes);

	}
}

void resolve_globals(CompilerContext* ctx, Node* node) {
	SymbolTable* current_table = symbol_stack->tables[symbol_stack->top];
	if (!current_table) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {

			int hash_key = hash(current_table->capacity, node->left->value.name);
			Symbol* sym = create_symbol(ctx, SYMBOL_GLOBAL, node->left->value.name, node->left->t);		
			if (scope_lookup_current(sym, hash_key)) {
				return;
			} else {
				node->left->symbol = sym;
				if (scope_bind(ctx, node->left->symbol, hash_key)) {
					printf("Bound symbol '%s' to scope level ('%d')\n", node->left->symbol->name, symbol_stack->top);
				}				
			}

			break;
		}

		case NODE_NAME: {
			if (node->t) {
				if (node->t->kind == TYPE_FUNCTION) {
					param_byte_offset = 0;
					local_byte_offset = 0;
					total_local_bytes = 0;
					start_at_param_bytes = false;

					if (node->t->subtype && node->t->subtype->kind == TYPE_VOID) { 
						push_context(ctx, CONTEXT_VOID_FUNCTION);
					} else { 
						push_context(ctx, CONTEXT_NONVOID_FUNCTION); 
					}

					int hash_key = hash(current_table->capacity, node->value.name);
					node->symbol = create_symbol(ctx, SYMBOL_GLOBAL, node->value.name, node->t);
					if (scope_bind(ctx, node->symbol, hash_key)) {
						printf("\033[31mBound symbol '%s' to scope level ('%d') with TYPE: '%d' and RETURN TYPE: '%d'\033[0m\n", node->symbol->name, symbol_stack->top, node->symbol->type->kind, node->symbol->type->subtype->kind);
					} 

					size_t total_param_bytes = 0;
					push_scope(ctx);

					if (node->t->params) {
						Node* wrapped_param = node->t->params;
						while (wrapped_param) {
							Node* next_wrapped_param = wrapped_param->next;
							resolve_params(ctx, wrapped_param);
							total_param_bytes += wrapped_param->right->symbol->actual_bytes;
							wrapped_param = next_wrapped_param;
						}
					}
					printf("\033[33mTotal param bytes: %zu\033[0m\n", total_param_bytes);

					if (node->right) {
						Node* block = node->right;
						Node* stmt = block->right;
						while (stmt) {
							Node* next = stmt->next;
							resolve_statement(ctx, stmt);
							stmt = next;
						}
					}

					pop_scope();

					node->symbol->total_bytes = total_param_bytes + total_local_bytes;
					printf("Total local bytes for '%s' before alignment: %d\n", node->symbol->name, node->symbol->total_bytes);

					if ((node->symbol->total_bytes % 16) != 0) {
						node->symbol->total_bytes += 16 - (node->symbol->total_bytes % 16);
					}

					printf("Total local bytes for '%s' after alignment: %d\n", node->symbol->name, node->symbol->total_bytes);


					
					pop_context();
				}
			} 
			break;
		}
	}
}

void resolve_tree(CompilerContext* ctx, Node* root) {
	if (!ctx || !root) return;

	init_symbol_stack(ctx);
	init_context_stack(ctx);

	push_scope(ctx);
	
	Node* current = root;
	while (current) {
		Node* next = current->next;
		resolve_globals(ctx, current);
		current = next;
	}

	pop_scope();
}