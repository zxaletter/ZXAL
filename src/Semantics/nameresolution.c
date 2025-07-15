#include "nameresolution.h"
#include "compilercontext.h"
#include "symbols.h"

static size_t total_param_bytes = 0;
static size_t param_byte_offset = 0;
static size_t local_byte_offset = 0;
static size_t total_local_bytes = 0;
static bool start_at_param_bytes = false;

ContextStack* context_stack = NULL;

void init_context_stack(CompilerContext* ctx) {
	context_stack = create_context(ctx);
	if (!context_stack) {
		perror("Failed to initialize global context stack\n");
		return;	
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

context_t peek_context() {
	return context_stack->contexts[context_stack->top];
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

bool push_scope(CompilerContext* ctx) {
	if (ctx->symbol_stack->top >= ctx->symbol_stack->capacity) {
		size_t prev_capacity = ctx->symbol_stack->capacity;
		
		ctx->symbol_stack->capacity *= 2;
		size_t new_capacity = ctx->symbol_stack->capacity;
		void** new_tables = arena_reallocate(
			ctx->symbol_arena, 
			ctx->symbol_stack->tables, 
			prev_capacity * sizeof(SymbolTable*), 
			new_capacity * sizeof(SymbolTable*)
		);

		if (!new_tables) {
			perror("Unable to reallocate stack tables\n");
			return false;
		}
		ctx->symbol_stack->tables = new_tables;
	}

	ctx->symbol_stack->tables[++ctx->symbol_stack->top] = create_symbol_table(ctx);
	if (!ctx->symbol_stack->tables[ctx->symbol_stack->top]) {
		perror("Unable to create new symbol table\n");
		return false;
	} else {
		SymbolTable* current = ctx->symbol_stack->tables[ctx->symbol_stack->top];
		printf("Created new table at level %d with address %p\n", ctx->symbol_stack->top, (void*)current);
	}
	return true;
}

void pop_scope(CompilerContext* ctx) {
	if (!is_stack_empty(ctx)) {
		ctx->symbol_stack->top--;
	}
}

bool is_stack_empty(CompilerContext* ctx) {
	return ctx->symbol_stack->top == -1;
}

int hash(int table_capacity, char* name) {
	if (!name) return -1;
	int hash = 0;
	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % table_capacity;
}

Symbol* lookup_symbol_in_all_scopes(CompilerContext* ctx, Symbol* symbol) {
	if (!symbol) return NULL;

	for (int i = ctx->symbol_stack->top; i >= 0; i--) {
		SymbolTable* current_table = ctx->symbol_stack->tables[i];
		if (!current_table) continue;

		for (int j = 0; j < current_table->capacity; j++) {
			Symbol* current_symbol = current_table->symbols[j];
			while (current_symbol) {
				Symbol* link_to_current_symbol = current_symbol->link;
				if (symbol->name && strcmp(symbol->name, current_symbol->name) == 0) {
					return current_symbol;
				}
				current_symbol = link_to_current_symbol;
			}
		}
	}
	printf("Symbol '%s' not found in any scope.\n", symbol->name ? symbol->name : NULL);
	return NULL;
}

Symbol* lookup_symbol_in_current_scope(CompilerContext* ctx, Symbol* symbol) {
	if (!symbol) return NULL;

	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return NULL;
	
	for (int i = 0; i < current_table->capacity; i++) {
		Symbol* current_symbol = current_table->symbols[i];
		while (current_symbol) {
			Symbol* link_to_current_symbol = current_symbol->link;
			if (symbol->name && strcmp(symbol->name, current_symbol->name) == 0) {
				return current_symbol;
			}
			current_symbol = link_to_current_symbol;
		}
	}
	printf("Symbol '%s' not found in current scope.\n", symbol->name);
	return NULL;
}

size_t get_num_bytes(Symbol* symbol) {
	if (!symbol || (symbol && !symbol->type)) return 0;

	switch (symbol->type->kind) {
		case TYPE_INTEGER: {
			return sizeof(int);
		}

		case TYPE_CHAR:
		case TYPE_BOOL: {
			return sizeof(char);
		}

		case TYPE_ARRAY: {
			if (symbol->type->subtype) {
				switch (symbol->type->subtype->kind) {
					case TYPE_INTEGER: return sizeof(int);
					case TYPE_CHAR:
					case TYPE_BOOL: return sizeof(char);
					default: 
						return 0;
				}
			} 
		}

		case TYPE_FUNCTION:
		case TYPE_VOID:
		case TYPE_STRUCT:
		case TYPE_UNKNOWN:
		default:
			printf("Warning: get_num_bytes: Unhandled or UNKNOWN type kind %d for symbol '%s'. Returning 0 bytes.\n", symbol->type->kind, symbol->name ? symbol->name : "N/A");
			return 0;
	}	
}

TypeKind get_kind(struct Type* t) {
	if (t) {
		return t->kind;
	}
	return TYPE_UNKNOWN;
}

void update_node_info(Node* node, UpdateNodeInfoType kind, size_t size) {
	switch (kind) {
		case PARAM_UPDATE: {
			Symbol* param_symbol = node->symbol;
			if (param_symbol) {
				param_symbol->param_byte_offset = param_byte_offset;
				param_symbol->actual_bytes = size;
			}
			break;
		}

		case LOCAL_UPDATE: {
			Symbol* sym = node->symbol;
			if (sym) {
				if (sym->type && sym->type->kind == TYPE_ARRAY) {
					int element_count = node->right ? node->right->value.val : 0;
					int new_size = size * element_count;

					sym->local_byte_offset = local_byte_offset;
					sym->actual_bytes = new_size; 
					update_local_byte_offset(new_size);
				} else {
					sym->local_byte_offset = local_byte_offset;
					sym->actual_bytes = size;
				}
			}
			break;
		}
	}
}

void update_total_local_bytes(size_t size) {
	if (total_local_bytes == 0) {
	 	total_local_bytes = size; 
	 } else { 
	 	total_local_bytes += size; 
	}
}

void update_local_byte_offset(size_t size) {
	if (!start_at_param_bytes && param_byte_offset > 0) {
		local_byte_offset = param_byte_offset;
		local_byte_offset += size;
		start_at_param_bytes = true;
	} else { 
		local_byte_offset += size; 
	}
}

bool has_expression_context() {
	context_t kind = peek_context();

	switch (kind) {
		case CONTEXT_OP:
		case CONTEXT_IF:
		case CONTEXT_LOOP:
		case CONTEXT_RETURN:
		case CONTEXT_AUG:
		case CONTEXT_SUBSCRIPT:
		case CONTEXT_ASSIGNMENT:
			return true;
		
		default:
			return false;
	}
}

void resolve_expression(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_CHAR: 
		case NODE_BOOL:
		case NODE_INTEGER:
			break;

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
			SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
			if (!current_table) return;
				
			int hash_key = hash(current_table->capacity, node->value.name);
			
			Symbol* node_symbol = node->symbol;
			//for definitions
			if (node->t && !node->symbol) {
				Symbol* new_symbol = create_symbol(ctx, SYMBOL_LOCAL, node->value.name, NULL, node->t);
				int hash_key = hash(current_table->capacity, node->value.name);

				if (new_symbol && (hash_key >= 0)) {
					bind(ctx, LOCAL, new_symbol, hash_key);
					node->symbol = (void*)new_symbol;
				}

				// if array, resolving index
				if (node->left) resolve_expression(ctx, node->left);

				if (new_symbol) {
					size_t size = get_num_bytes(new_symbol);
					update_total_local_bytes(size);
					update_local_byte_offset(size);
					update_node_info(node, LOCAL_UPDATE, size);
				}
				break;
			}
			
			if (peek_context() == CONTEXT_CALL) {
				Symbol* temp_symbol = create_symbol(ctx, SYMBOL_LOCAL, node->value.name, NULL, NULL);

				Symbol* func_symbol = lookup_function_symbol(ctx, temp_symbol);
				if (!func_symbol) {
					printf("\033[31mError\033[0m: attempting to invoke function that does not exist\n");
				} 
				node->symbol = func_symbol;
				break;
			}

			if (has_expression_context()) {
				context_t type = peek_context();
				printf("\033[32m\nCurrent context type is %d\033[0m\n", type);
				Symbol* temp_symbol = create_symbol(ctx, SYMBOL_LOCAL, node->value.name, NULL, NULL);
				
				Symbol* retrieved_symbol = lookup_symbol_in_current_scope(ctx, temp_symbol);
				if (retrieved_symbol) {
					node->symbol = (void*)retrieved_symbol;
					node->t = (void*)retrieved_symbol->type;
				
				} else {
					retrieved_symbol = lookup_symbol_in_all_scopes(ctx, temp_symbol);
					
					if (retrieved_symbol) {
						node->symbol = (void*)retrieved_symbol;
						node->t = (void*)retrieved_symbol->type;
					} else {
						printf("\033[31mError\033[0m: did not find symbol in remaining scopes\n");
					}
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

			if (node->left) {
				node->symbol = node->left->symbol;
				
				Type* retrieved_type = NULL;
				if (node->symbol) {
					retrieved_type = ((Symbol*)node->symbol)->type;
				}

				if (retrieved_type) {
					node->t = (void*)retrieved_type->subtype;
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
			
			pop_scope(ctx);
			pop_context();
			break;
		}

		case NODE_ELSE: {
			push_scope(ctx);
			if (node->right) {
				resolve_statement(ctx, node->right);
			}
			pop_scope(ctx);
			break;
		}

		case NODE_WHILE: {
			push_scope(ctx);
			push_context(ctx, CONTEXT_LOOP);

			if (node->left) {
				resolve_expression(ctx, node->left);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			}

			pop_scope(ctx);
			pop_context();
			break;
		}

		case NODE_FOR: {
			push_scope(ctx);
			push_context(ctx, CONTEXT_LOOP);

			if (node->left) {
				resolve_statement(ctx, node->left);
			}

			Node* condition = node->left ? node->left->next : NULL;
			if (condition) {
				resolve_expression(ctx, condition);
			}

			Node* update = condition ? condition->next : NULL;
			if (update) {
				resolve_expression(ctx, update);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			}

			pop_scope(ctx);
			pop_context();
			break;
		}
		
		case NODE_RETURN: {
			if (!node->right) {
				context_lookup(CONTEXT_VOID_FUNCTION);
			} else {
				context_lookup(CONTEXT_NONVOID_FUNCTION);
				push_context(ctx, CONTEXT_RETURN);
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

		case NODE_CONTINUE:
		case NODE_BREAK: {
			context_lookup(CONTEXT_LOOP);
			break;
		}

		case NODE_CALL: {
			resolve_expression(ctx, node);
			break;
		}
	}
}

void update_global_parameter_variables(size_t param_size) {
	if (param_byte_offset == 0) {
		param_byte_offset = param_size;
	} else {
		param_byte_offset += param_size;
	}

	total_param_bytes += param_size;	
}

void resolve_params(CompilerContext* ctx, Node* node) {
	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return;

	Node* current_wrapped_param = node;
	while (current_wrapped_param) {
		Node* next_wrapped_param = current_wrapped_param->next;

		Node* param = current_wrapped_param->right;
		if (param) {
			int hash_key = hash(current_table->capacity, param->value.name);
			if (hash_key >= 0) {
				Symbol* sym = create_symbol(ctx, SYMBOL_PARAM, param->value.name, NULL, param->t);
				bind(ctx, LOCAL, sym, hash_key);
				param->symbol = (void*)sym;
				
				size_t size = get_num_bytes(sym);
				update_global_parameter_variables(size);
				update_node_info(param, PARAM_UPDATE, size);
			}
		}

		current_wrapped_param = next_wrapped_param;
	}	
}

void set_total_local_bytes(Node* node) {
	Symbol* sym = node->symbol;
	if (sym) {
		sym->total_bytes = total_param_bytes + total_local_bytes;
		if ((sym->total_bytes % 16) != 0) {
			sym->total_bytes += 16 - (sym->total_bytes % 16);
		}
	}
}

void reset_offsets() {
	total_param_bytes = 0;
	param_byte_offset = 0;
	local_byte_offset = 0;
	total_local_bytes = 0;
	start_at_param_bytes = false;
}

void resolve_globals(CompilerContext* ctx, Node* node) {
	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			int hash_key = -1;
			if (node->left) {
				hash_key = hash(current_table->capacity, node->left->value.name);
			}

			if (hash_key >= 0) {
				Symbol* sym = create_symbol(ctx, SYMBOL_GLOBAL, node->left->value.name, NULL, node->left->t);		
				if (!lookup_symbol_in_current_scope(sym, hash_key)) {
					bind(ctx, GLOBAL, sym, hash_key);
					node->left->symbol = (void*)sym;
				} 
			}
			break;
		}

		case NODE_NAME: {
			Type* type = node->t;
			if (!type) return;

			if (type->kind != TYPE_FUNCTION) return;

			if (!type->subtype) return;

			reset_offsets();

			if (type->subtype->kind == TYPE_VOID) {
				push_context(ctx, CONTEXT_VOID_FUNCTION);
			} else {
				push_context(ctx, CONTEXT_NONVOID_FUNCTION);
			}

			push_scope(ctx);

			if (node->params) {
				resolve_params(ctx, node->params);
			}

			if (node->right) {
				resolve_statement(ctx, node->right);
			}

			set_total_local_bytes(node);

			pop_scope(ctx);
			pop_context();
			break;
		}
	}
}

bool rehash_variable_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity) {
	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return false;

	for (int i = 0; i < prev_capacity; i++) {
		Symbol* current_symbol = prev_symbols[i];
		while (current_symbol) {
			Symbol* link_for_current_symbol = current_symbol->link;

			int updated_hash_key = hash(new_capacity, current_symbol->name);
			if (updated_hash_key == -1) return false;

			current_symbol->link = current_table->symbols[updated_hash_key];
			current_table->symbols[updated_hash_key] = current_symbol;
			if (!current_symbol->link) {
				current_table->size++;
			}

			current_symbol = link_for_current_symbol;
		}
	}
	return true;
}

bool rehash_function_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity) {
	SymbolTable* global_table = ctx->global_table;

	for (int i = 0; i < prev_capacity; i++) {
		Symbol* current_prev_symbol = prev_symbols[i];
		while (current_prev_symbol) {
			Symbol* link_for_current_symbol = current_prev_symbol->link;

			int updated_hash_key = hash(new_capacity, current_prev_symbol->name);
			if (updated_hash_key == -1) return false;

			current_prev_symbol->link = global_table->symbols[updated_hash_key];
			global_table->symbols[updated_hash_key] = current_prev_symbol;
			if (!current_prev_symbol->link) {
				global_table->size++;
			}

			current_prev_symbol = link_for_current_symbol;
		}
	}
	return true;
}

bool variable_symbol_bind(CompilerContext* ctx, Symbol* symbol, int hash_key) {
	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return false;

	if (current_table->size >= current_table->capacity) {
		int prev_capacity = current_table->capacity;
		Symbol** all_symbols = current_table->symbols;

		current_table->capacity *= 2;
		int new_capacity = current_table->capacity;
		void** new_local_symbols = arena_reallocate(
			ctx->symbol_arena,
			current_table->symbols,
			prev_capacity * sizeof(Symbol*),
			new_capacity * sizeof(Symbol*)
		);

		if (!new_local_symbols) return false;
		current_table->symbols = new_local_symbols;

		for (int i = 0; i < current_table->capacity; i++) {
			current_table->symbols[i] = NULL;
		}

		current_table->size = 0;
		if (!rehash_variable_symbols(ctx, all_symbols, new_capacity, prev_capacity)) {
			return false;
		}

		hash_key = hash(current_table->capacity, symbol->name);
	}

	if (hash_key == -1) return false;

	printf("In 'variable_symbol_bind', current table address: %p\n", (void*)current_table);

	if (current_table->symbols[hash_key]) {
		printf("added symbol '%s' to table with address: %p at level %d\n", symbol->name, (void*)current_table, ctx->symbol_stack->top);
		symbol->link = current_table->symbols[hash_key];
		current_table->symbols[hash_key] = symbol;
	} else {
		printf("added symbol '%s' to table with address: %p at level %d\n", symbol->name, (void*)current_table, ctx->symbol_stack->top);
		current_table->symbols[hash_key] = symbol;
		symbol->link = NULL;
		current_table->size++;
	}

	return true;
}

bool function_symbol_bind(CompilerContext* ctx, Symbol* func_symbol, int hash_key) {
	if (ctx->global_table->size >= ctx->global_table->capacity) {
		int prev_capacity = ctx->global_table->capacity;
		Symbol** all_symbols = ctx->global_table->symbols;

		ctx->global_table->capacity *= 2;
		int new_capacity = ctx->global_table->capacity;
		void** new_global_symbols = arena_reallocate(
			ctx->symbol_arena,
			ctx->global_table->symbols,
			prev_capacity * sizeof(Symbol*),
			new_capacity * sizeof(Symbol*)
		);

		if (!new_global_symbols) return false;
		ctx->global_table->symbols = new_global_symbols;

		for (int i = 0; i < ctx->global_table->capacity; i++) {
			ctx->global_table->symbols[i] = NULL;
		}

		ctx->global_table->size = 0;
		if (!rehash_function_symbols(ctx, all_symbols, new_capacity, prev_capacity)) {
			return false;
		}

		hash_key = hash(ctx->global_table->capacity, func_symbol->name);
	}

	if (hash_key == -1) return false;

	if (ctx->global_table->symbols[hash_key]) {
		func_symbol->link = ctx->global_table->symbols[hash_key];
		ctx->global_table->symbols[hash_key] = func_symbol;
	} else {
		ctx->global_table->symbols[hash_key] = func_symbol;
		func_symbol->link = NULL;
		ctx->global_table->size++;
	}

	return true;
}

bool variable_symbol_exists(CompilerContext* ctx, Symbol* symbol) {
	SymbolTable* current_table = ctx->symbol_stack->tables[ctx->symbol_stack->top];
	if (!current_table) return false;

	printf("In 'variable_symbol_exists'\n");
	printf("\033[32mGot table at level %d\033[0m\n", ctx->symbol_stack->top);

	for (int i = 0; i < current_table->capacity; i++) {
		Symbol* current_symbol = current_table->symbols[i];
		while (current_symbol) {
			Symbol* link_for_current_symbol = current_symbol->link;
			if (symbol->name && strcmp(symbol->name, current_symbol->name) == 0) {
				printf("\033[31mError\033[0m: cannot have two variables with identical names in same scope\n");
				return true;
			}
			current_symbol = link_for_current_symbol;
		}
	}
	return false;
}

bool function_symbol_exists(CompilerContext* ctx, Symbol* symbol) {
	for (int i = 0; i < ctx->global_table->capacity; i++) {
		Symbol* current_global_symbol = ctx->global_table->symbols[i];
		while (current_global_symbol) {
			Symbol* link_for_current_global_symbol = current_global_symbol->link;
			if (symbol->name && strcmp(symbol->name, current_global_symbol->name) == 0) {
				printf("\033[31mError\033[0m: cannot have two functions with the same name\n");
				return true;
			}
			current_global_symbol = link_for_current_global_symbol;
		}
	}
	return false;
}

bool symbol_exists(CompilerContext* ctx, BindType kind, Symbol* symbol) {
	switch (kind) {
		case GLOBAL:
		case LOCAL: {
			if (variable_symbol_exists(ctx, symbol)) {
				return true;
			}
			break;
		}

		case FUNCTION: {
			if (function_symbol_exists(ctx, symbol)) {
				return true;
			}
			break;
		}
	}
	return false;
}

bool bind(CompilerContext* ctx, BindType kind, Symbol* symbol, int hash_key) {
	if (!symbol || (symbol && !symbol->name)) return false;

	if (symbol_exists(ctx, kind, symbol)) return false;

	switch (kind) {
		case GLOBAL:
		case LOCAL: 
			return variable_symbol_bind(ctx, symbol, hash_key);
		
		case FUNCTION: 
			return function_symbol_bind(ctx, symbol, hash_key);	
	}
}

Symbol* lookup_function_symbol(CompilerContext* ctx, Symbol* symbol) {
	if (!symbol) return NULL;

	for (int i = 0; i < ctx->global_table->capacity; i++) {
		Symbol* current_func_symbol = ctx->global_table->symbols[i];
		while (current_func_symbol) {
			Symbol* link_for_current_func_symbol = current_func_symbol->link;
			if (symbol->name && strcmp(symbol->name, current_func_symbol->name) == 0) {
				return current_func_symbol;
			}
			current_func_symbol = link_for_current_func_symbol;
		}
	}
	return NULL;
}

void validate_node_signature(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_NAME: {
			Type* type = node->t;
			if (type) {
				switch (type->kind) {
					case TYPE_INTEGER: 
					case TYPE_CHAR:
					case TYPE_BOOL:
					case TYPE_STRING:
					case TYPE_STRUCT:
					case TYPE_ENUM:
					case TYPE_ARRAY: 
						break;

					case TYPE_FUNCTION: {
						Symbol* func_symbol = create_symbol(
							ctx,
							SYMBOL_GLOBAL,
							node->value.name,
							node->params,
							node->t
						);

						int hash_key = hash(ctx->global_table->capacity, func_symbol->name);
						if (hash_key != -1) {
							bind(ctx, FUNCTION, func_symbol, hash_key);
						}						
						break;
					}
				}	
			}

			break;
		}

		default:
			break;
	}
}

void collect_function_symbols(CompilerContext* ctx, Node* root) {
	if (!root) return;

	Node* current = root;
	while (current) {
		Node* next = current->next;
		validate_node_signature(ctx, current);
		current = next;
	}
}

void resolve_tree(CompilerContext* ctx, Node* root) {
	if (!root) return;

	collect_function_symbols(ctx, root);
	init_context_stack(ctx);

	push_scope(ctx);
	
	Node* current = root;
	while (current) {
		Node* next = current->next;
		resolve_globals(ctx, current);
		current = next;
	}

	pop_scope(ctx);
}