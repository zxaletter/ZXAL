#include "tac.h"

static int label_counter = 0;
static int tac_variable_index = 0;
static int tac_parameter_index = 0;
static int tac_function_argument_index = 0;
static int tac_instruction_index = 0;
static int depth = 0;
TACTable* tac_table = NULL;
TACContextStack tac_context_stack;

void init_tac_table(CompilerContext* ctx) {
	tac_table = create_tac_table(ctx);
	if (!tac_table) {
		printf("In 'init_tac_table', received NULL tac table.\n");
		// exit(EXIT_FAILURE);
	}
}

TACTable* create_tac_table(CompilerContext* ctx) {
	// TACTable* table = malloc(sizeof(TACTable));
	TACTable* table = arena_allocate(ctx->ir_arena, sizeof(TACTable));
	if (!table) {
		perror("In 'create_tac_table', unable to allocate space for tac table.\n");
		return NULL;
	} 

	table->size = 0;
	table->tac_index = 0;
	table->capacity = INITIAL_TABLE_CAPACITY;
	// table->tacs = calloc(table->capacity, sizeof(TACInstruction*));
	table->tacs = arena_allocate(ctx->ir_arena, sizeof(TACInstruction*) * table->capacity);
	if (!table->tacs) {
		perror("In 'create_tac_table', unable to allocate space and initialize table->tacs.\n");
		// free(table);
		return NULL;
	}
	return table;
}

TACInstruction* create_tac(CompilerContext* ctx, tac_t type, Operand* result, Operand* op1, Operand* op2, Operand* op3) {
	TACInstruction* tac = arena_allocate(ctx->ir_arena, sizeof(TACInstruction));
	if (!tac) {
		perror("In 'create_TAC', unable to allocate space for tac.\n");
		return NULL;
	}

	tac->type = type;
	tac->id = tac_instruction_index++;
	// printf("\033[32mCreated TAC with type %d, tac ID: %d\033[0m\n", tac->type, tac->id);
	tac->result = result;
	tac->op1 = op1;
	tac->op2 = op2;
	tac->op3 = op3;
	return tac;
}

bool init_tac_context_stack(CompilerContext* ctx) {
	tac_context_stack = create_tac_context_stack(ctx);
	if (!tac_context_stack.contexts) {
		return false;
	}
}

TACContextStack create_tac_context_stack(CompilerContext* ctx) {
	// TACContext** contexts = calloc(INITIAL_TACCONTEXT_CAPACITY, sizeof(TACContext*));
	TACContext** contexts = arena_allocate(ctx->ir_arena, sizeof(TACContext*) * INITIAL_TACCONTEXT_CAPACITY);
	if (!contexts) {
		TACContextStack dummy_stack = {
			.size = 0,
			.capacity = 0,
			.top = -1,
			.contexts = NULL
		};
		return dummy_stack;
	}

	TACContextStack new_tac_context_stack = {
		.size = 0,
		.capacity = INITIAL_TACCONTEXT_CAPACITY,
		.top = -1,
		.contexts = contexts
	};
	return new_tac_context_stack;
}

bool is_tac_context_stack_empty() {
	return tac_context_stack.top == -1;
} 

void push_tac_context(CompilerContext* ctx, TACContext* context) {
	if (!context || !tac_context_stack.contexts) return;

	if (tac_context_stack.size >= tac_context_stack.capacity) {
		int prev_capacity = tac_context_stack.capacity;
		tac_context_stack.capacity *= 2;
		int new_capacity = tac_context_stack.capacity;

		void* new_contexts = arena_reallocate(
			ctx->ir_arena, 
			tac_context_stack.contexts, 
			prev_capacity * sizeof(TACContext*), 
			new_capacity * sizeof(TACContext*)
		);

		if (!new_contexts) {
			perror("In 'push_tac_context', unable to reallocate TAC contexts.\n");
			return;
		}
		tac_context_stack.contexts = new_contexts;
	}
	tac_context_stack.contexts[++tac_context_stack.top] = context;
	tac_context_stack.size++;
}

TACContext* peek_tac_context() {
	if (!is_tac_context_stack_empty()) {
		return tac_context_stack.contexts[tac_context_stack.top];
	}
	return NULL;
}

void pop_tac_context() {
	if (!is_tac_context_stack_empty()) {
		tac_context_stack.top--;
		tac_context_stack.size--;
	}
}

void clear_tac_contexts(int target_depth) {
	if (!is_tac_context_stack_empty()) {
		while (tac_context_stack.top >= 0 && tac_context_stack.contexts[tac_context_stack.top]->depth == target_depth) {
			tac_context_stack.top--;
			tac_context_stack.size--;
		}
	}
}

TACContext* tac_context_lookup(tac_t* tac_target_types, size_t length) {
	if (!tac_target_types || !tac_context_stack.contexts){
	 	printf("\033[31mIn 'tac_context_lookup', tac target typesor contexts are NULL\033[0m\n");
		return NULL;
	}
	if (length > 0) {
		printf("length greater than 0\n");
		for (int i = tac_context_stack.top; i >= 0; i--) {
			TACContext* current_context = tac_context_stack.contexts[i];
			if (current_context) {
				printf("We have current context\n");
				for (int j = 0; j < length; j++) {
					printf("Looping through tac target types\n");
					if (current_context->type == tac_target_types[j]) {
						return current_context;
					}
				}			
			}
		}
	} else {
		printf("\033[31mIn 'tac_context_lookup', length of tac target types array is is not greater than ZERO\033[0m\n");
	}

	return NULL;
}

TACContext* create_tac_context(CompilerContext* ctx, tac_t type, char* next_label, char* end_label, 
	char* update_label, bool conditional_statement_next, int depth, bool root_chain_mem) {

	TACContext* tac_context = arena_allocate(ctx->ir_arena, sizeof(TACContext));
	if (!tac_context) {
		perror("In 'create_context_stack', unable to allocate space for tac context.\n");
		return NULL;
	}

	tac_context->type = type;
	tac_context->end_label = NULL;
	tac_context->update_label = NULL;
	tac_context->next_label = NULL;
	tac_context->conditional_statement_next = conditional_statement_next;
	tac_context->depth = depth;
	tac_context->root_chain_mem = root_chain_mem;

	if (next_label) {
		tac_context->next_label = arena_allocate(ctx->ir_arena, strlen(next_label) + 1);
		if (!tac_context->next_label) {
			printf("In 'create_tac_context', unable to duplicate label '%s'\n", next_label);
			return NULL;
		}
		strcpy(tac_context->next_label, next_label);
	}

	if (end_label) {
		tac_context->end_label = arena_allocate(ctx->ir_arena, strlen(end_label) + 1);
		if (!tac_context->end_label) {
			printf("In 'create_context_stack', unable to duplicate label '%s'\n", end_label);
			return NULL;
		}
		strcpy(tac_context->end_label, end_label);
	}

	if (update_label) {
		tac_context->update_label = arena_allocate(ctx->ir_arena, strlen(update_label) + 1);
		if (!tac_context->update_label) {			
			return NULL;
		}
		strcpy(tac_context->update_label, update_label);
	}

	return tac_context;
}

char* generate_label(CompilerContext* ctx) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), ".L%d", label_counter++);

	char* label = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	if (!label) return NULL;
	
	strcpy(label, buffer);
	return label;
}

char* tac_variable_create(CompilerContext* ctx) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "t%d", tac_variable_index++);

	char* tac_variable = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	if (!tac_variable) return NULL;

	strcpy(tac_variable, buffer);
	return tac_variable;
}

char* tac_parameter_label(CompilerContext* ctx) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "p%d", tac_parameter_index++);

	char* param = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	if (!param) return NULL;
	strcpy(param, buffer);
	return param;
}

char* tac_function_argument_label(CompilerContext* ctx) {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "a%d", tac_function_argument_index++);

	char* arg = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	if (!arg) return NULL;

	strcpy(arg, buffer);
	return arg;
}

char* tac_function_name(CompilerContext* ctx, char* function_name) {
	char* buffer = arena_allocate(ctx->ir_arena, strlen(function_name) + 3);
	if (!buffer) return NULL;
	snprintf(buffer, strlen(function_name) + 3, "_%s:", function_name);
	return buffer;
}

void add_tac_to_table(CompilerContext* ctx, TACInstruction* tac) {
	if (!tac || !tac_table) return;

	if (tac_table->size >= tac_table->capacity) {
		int prev_capacity = tac_table->capacity;
		tac_table->capacity *= 2;
		int new_capacity = tac_table->capacity;
		void* new_tacs = arena_reallocate(
			ctx->ir_arena, 
			tac_table->tacs, 
			prev_capacity * sizeof(TACInstruction*), 
			new_capacity * sizeof(TACInstruction*)
		);
		
		if (!new_tacs) {
			perror("In 'add_tac_to_table', unable to reallocate tacs\n");
			return;
		}
		tac_table->tacs = new_tacs;
	}
	tac_table->tacs[tac_table->tac_index++] = tac;
	tac_table->size++;
}

tac_t get_tac_type(node_t type) {
	switch (type) {
		case NODE_ADD: return TAC_ADD;
		case NODE_SUB: return TAC_SUB;
		case NODE_MUL: return TAC_MUL;
		case NODE_DIV: return TAC_DIV;
		case NODE_ADD_EQUAL: return TAC_ADD_EQUAL;
		case NODE_SUB_EQUAL: return TAC_SUB_EQUAL;
		case NODE_MUL_EQUAL: return TAC_MUL_EQUAL;
		case NODE_DIV_EQUAL: return TAC_DIV_EQUAL;
		case NODE_MODULO: return TAC_MODULO;
		case NODE_LOGICAL_AND: return TAC_LOGICAL_AND;
		case NODE_LOGICAL_OR: return TAC_LOGICAL_OR;
		case NODE_LESS: return TAC_LESS;
		case NODE_GREATER: return TAC_GREATER;
		case NODE_LESS_EQUAL: return TAC_LESS_EQUAL;
		case NODE_GREATER_EQUAL: return TAC_GREATER_EQUAL;
		case NODE_EQUAL: return TAC_EQUAL;
		case NODE_NOT_EQUAL: return TAC_NOT_EQUAL;
		case NODE_INCREMENT: return TAC_INCREMENT;
		case NODE_DECREMENT: return TAC_DECREMENT;
		case NODE_INTEGER: return TAC_INTEGER;
		case NODE_BOOL: return TAC_BOOL;
		case NODE_CHAR: return TAC_CHAR;
	}
}

Operand* create_operand(CompilerContext* ctx, operand_t kind, OperandValue value) {
	Operand* new_operand = arena_allocate(ctx->ir_arena, sizeof(Operand));
	if (!new_operand) {
		perror("In 'create_operand', unabel to allocate space for new operand\n");
		return NULL;
	}

	new_operand->kind = kind;
	switch (kind) {	
		case OP_ARG:
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MODULO:
		case OP_LESS:
		case OP_GREATER:
		case OP_LESS_EQUAL:
		case OP_GREATER_EQUAL:
		case OP_EQUAL:
		case OP_NOT_EQUAL:
		case OP_LOGICAL_OR:
		case OP_LOGICAL_AND:
		case OP_UNARY_ADD:
		case OP_UNARY_SUB:
		case OP_NOT:
		case OP_STORE: {
			if (value.label_name) {
				int length = strlen(value.label_name);
				char* copy = arena_allocate(ctx->ir_arena, length + 1);
				if (!copy) return NULL;

				strncpy(copy, value.label_name, length);
				copy[length] = '\0';
				new_operand->value.label_name = copy;
			} else {
				new_operand->value.label_name = NULL;
			}
			break;
		}

		case OP_SYMBOL: {
			new_operand->value.sym = value.sym;
			break;
		}
		default: {
			new_operand->value = value; 
			break;
		}
	}

	return new_operand;
}

char* convert_subtype_to_string(struct type* type) {
	if (!type) return NULL;

	switch (type->kind) {
		case TYPE_INTEGER: return "SIZEOF_INT";
		case TYPE_CHAR: return "SIZEOF_CHAR";
		case TYPE_BOOL: return "SIZEOF_BOOL";
		case TYPE_ARRAY: {
			return convert_subtype_to_string(type->subtype);
		}
		default: return NULL;
	}
}

TACInstruction* build_tac_from_array_dagnode(CompilerContext* ctx, Node* array_identifier, Node* array_list) {
	if (!array_identifier) return NULL;

	OperandValue array_subtype_val =  {.label_name = convert_subtype_to_string(array_identifier->left->t)};

	Operand* array_subtype_op = create_operand(ctx, OP_SUBTYPE_STR, array_subtype_val);
	if (!array_subtype_op) {
		printf("\033[31mUnable to create subtype op in 'BUILD_TAC_FROM_ARRAY_DAGNODE'\033[0m\n");
		return NULL;
	}

	OperandValue array_identifier_val = { .sym = array_identifier->left->symbol }; 
	Operand* array_identifier_op = create_operand(ctx, OP_SYMBOL, array_identifier_val);
	if (!array_identifier_op) {
		printf("\033[31mUNABLE TO CREATE array identifier op\033[0m\n");
		return NULL;
	} else {
		printf("\033[31mSUCCESSFULLY MADE ARRAY IDENTIFIER OP\033[0m\n");
	}

	int i = 0;
	Node* element = array_list->right;

	while (element) {
		Node* next_element = element->next;

		OperandValue element_index_label_val = { .label_name = tac_variable_create(ctx) };
		Operand* element_index_op = create_operand(ctx, OP_MUL, element_index_label_val);
		
		OperandValue buffer_val = { .int_val = i };
		Operand* buffer_op = create_operand(ctx, OP_INT_LITERAL, buffer_val);
		
		TACInstruction* element_index_tac = create_tac(ctx, TAC_MUL, element_index_op, buffer_op, array_subtype_op, NULL);
		add_tac_to_table(ctx, element_index_tac);

		OperandValue element_address_val = { .label_name = tac_variable_create(ctx) };
		Operand* element_address_op = create_operand(ctx, OP_STORE, element_address_val);

		TACInstruction* pos_tac = create_tac(ctx, TAC_ADD, element_address_op, array_identifier_op, element_index_op, NULL);
		add_tac_to_table(ctx, pos_tac);

		OperandValue element_val = { .int_val = element->value.val };
		Operand* element_op = create_operand(ctx, OP_INT_LITERAL, element_val);
		if (!element_op) {
			printf("\033[31mIn build_tac_from_array_dagnode', element op is NULL\033[0m\n");
			return NULL;
		}
		TACInstruction* element_store_tac = create_tac(ctx, TAC_STORE, element_address_op, element_op, NULL, NULL); 
		add_tac_to_table(ctx, element_store_tac);
		
		i++;
		element = next_element;
	}
}

operand_t get_operand_type(Operand* op) {
	if (!op) return OP_UNKNOWN;
	return op->kind;
}

operand_t node_to_operand_type(node_t type) {
	switch (type) {
		case NODE_ADD: return OP_ADD;
		case NODE_SUB: return OP_SUB;
		case NODE_MUL: return OP_MUL;
		case NODE_DIV: return OP_DIV;
		case NODE_MODULO: return OP_MODULO;
		case NODE_LESS: return OP_LESS;
		case NODE_GREATER: return OP_GREATER;
		case NODE_LESS_EQUAL: return OP_LESS_EQUAL;
		case NODE_GREATER_EQUAL: return OP_GREATER_EQUAL;
		case NODE_LOGICAL_OR: return OP_LOGICAL_OR;
		case NODE_LOGICAL_AND: return OP_LOGICAL_AND;
		case NODE_EQUAL: return OP_EQUAL;
		case NODE_NOT_EQUAL: return OP_NOT_EQUAL;
	}
}

TACInstruction* build_tac_from_expression_dag(CompilerContext* ctx, Node* node) {
	if (!node) return NULL;
	
	TACInstruction* result = NULL;
	switch (node->type) {
		case NODE_ADD:
		case NODE_SUB:
		case NODE_MUL:
		case NODE_DIV:
		case NODE_MODULO:
		case NODE_LESS:
		case NODE_GREATER:
		case NODE_LESS_EQUAL:
		case NODE_GREATER_EQUAL:
		case NODE_EQUAL:
		case NODE_NOT_EQUAL:
		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {		
			tac_t type = get_tac_type(node->type);
			TACInstruction* left = build_tac_from_expression_dag(ctx, node->left);
			TACInstruction* right = build_tac_from_expression_dag(ctx, node->right);

			if (!left || !right) {
				
				return NULL;
			}
			OperandValue main_operand_value = {
				.label_name = tac_variable_create(ctx)
			};
			operand_t op_type = node_to_operand_type(node->type);
			Operand* binary_operand = create_operand(ctx, op_type, main_operand_value);
			
			result = create_tac(ctx, type, binary_operand, left->result, right->result, NULL);
			add_tac_to_table(ctx, result);
			break;
		}

		case NODE_UNARY_ADD:
		case NODE_UNARY_SUB: {
			TACInstruction* unary_tac = build_tac_from_expression_dag(ctx, node->right);
			if (!unary_tac) return NULL;

			OperandValue val = {
				.label_name = tac_variable_create(ctx)
			};
			operand_t type = (node->type == NODE_UNARY_ADD) ? OP_UNARY_ADD : OP_UNARY_SUB;
			Operand* op = create_operand(ctx, type, val);

			tac_t kind = (type == OP_UNARY_ADD) ? TAC_UNARY_ADD : TAC_UNARY_SUB; 
			result = create_tac(ctx, kind, op, unary_tac->result, NULL, NULL);
			add_tac_to_table(ctx, result);
			break;
		}

		case NODE_NOT: {
			TACInstruction* unary_tac = build_tac_from_expression_dag(ctx, node->right);
			if (!unary_tac) return NULL;

			OperandValue val = {
				.label_name = tac_variable_create(ctx)
			};

			Operand* op = create_operand(ctx, OP_NOT, val);
			result = create_tac(ctx, TAC_NOT, op, unary_tac->result, NULL, NULL);
			add_tac_to_table(ctx, result);
			break;
		}

		case NODE_CHAR:
		case NODE_BOOL:
		case NODE_INTEGER: {
			tac_t type = get_tac_type(node->type);
			
			OperandValue tac_variable_union = {
				.label_name = tac_variable_create(ctx)
			};
			Operand* left_operand = create_operand(ctx, OP_STORE, tac_variable_union);

			OperandValue buffer_union = {
				.int_val = node->value.val,
			};
			Operand* right_operand = create_operand(ctx, OP_INT_LITERAL, buffer_union);
			result = create_tac(ctx, type, left_operand, right_operand, NULL, NULL);
			add_tac_to_table(ctx, result);
			
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			tac_t type = get_tac_type(node->type);
			char* tac_variable = tac_variable_create(ctx);

			Node* int_node = create_int_node(ctx, NODE_INTEGER, 1, NULL, NULL, NULL, NULL, NULL);
			// Node* copy_node = copy_node(ctx, node->left);

			node_t kind = (node->type == NODE_INCREMENT) ? NODE_ADD : NODE_SUB;
			Node* op_node = create_node(ctx, kind, node->left, int_node, NULL, NULL, NULL);
			TACInstruction* tac_from_node = build_tac_from_expression_dag(ctx, op_node);
			build_tac_from_expression_dag(ctx, node->left);
			
			if (!tac_from_node || (tac_from_node && !tac_from_node->result)) return NULL;

			if (!node->left->value.name) {
				return NULL;
			}

			OperandValue res_value = {
				.sym = node->left->symbol,
			};
			Operand* res = create_operand(ctx, OP_SYMBOL, res_value);
			TACInstruction* tac_assignment = create_tac(ctx, TAC_ASSIGNMENT, res, NULL, tac_from_node->result, NULL);
			result = tac_assignment;		
			add_tac_to_table(ctx, tac_assignment);
			break;
		}

		case NODE_AUG:
		case NODE_DECL:
		case NODE_DEF: {
			result = build_tac_from_expression_dag(ctx, node->left);
			break;
		}

		case NODE_NAME: {			
			OperandValue value = {
				.sym = node->symbol,
			};

			Operand* op = create_operand(ctx, OP_SYMBOL, value);
			if (!op) return NULL;
			result = create_tac(ctx, TAC_NAME, op, NULL, NULL, NULL);	
			break;
		}

		case NODE_ARG: {
			OperandValue arg_value_label = {
				.label_name = tac_function_argument_label(ctx)
			};
			Operand* left_operand = create_operand(ctx, OP_ARG, arg_value_label);
			if (!left_operand) return NULL;

			TACInstruction* arg = build_tac_from_expression_dag(ctx, node->right);
			if (!arg) {
				printf("\033[31mIN NODE_ARG CASE, TAC INSTRUTCTION ARG IS NULL\033[0m\n");
				return NULL;
			}
			
			if (arg->result) {
				printf("\033[31mARG RESULT OPERAND HAS TYPE %d\033[0m\n", arg->result->kind);
			}			
			TACInstruction* arg_tac = create_tac(ctx, TAC_ARG, left_operand, arg->result, NULL, NULL);
			add_tac_to_table(ctx, arg_tac);
			break;
		}

		case NODE_CALL: {
			Node* arg = node->right;
			while (arg) {
				Node* next_arg = arg->next;
				build_tac_from_expression_dag(ctx, arg);
				arg = next_arg;
			}

			TACInstruction* function_name = build_tac_from_expression_dag(ctx, node->left);

			TACInstruction* tac_call = create_tac(ctx, TAC_CALL, function_name->result, NULL, NULL, NULL);
			add_tac_to_table(ctx, tac_call);

			OperandValue func_return_label = {
				.label_name = tac_variable_create(ctx)
			};
			Operand* function_label_operand = create_operand(ctx, OP_STORE, func_return_label);

			OperandValue _return_label = {
				.label_name = "_RET"
			};
			Operand* _return_operand = create_operand(ctx, OP_RETURN, _return_label);

			TACInstruction* tac_function_return = create_tac(ctx, TAC_ASSIGNMENT, function_label_operand, NULL, _return_operand, NULL);
			add_tac_to_table(ctx, tac_function_return);

			result = tac_function_return;
			break;
		}

		case NODE_SUBSCRIPT: {
			TACInstruction* array_tac = build_tac_from_expression_dag(ctx, node->left);
			TACInstruction* index_tac = build_tac_from_expression_dag(ctx, node->right);

			if (!array_tac || !index_tac) return NULL;

			OperandValue array_val = {
				.label_name = convert_subtype_to_string(node->t)
			};
			if (!array_val.label_name) { printf("\033[31mIn NODE_SUBSCRIPT case, unable to convert subtype to string\033[0m\n");}
			Operand* op_array_val = create_operand(ctx, OP_SUBTYPE_STR, array_val);

			OperandValue mul_label_val = {
				.label_name = tac_variable_create(ctx)
			};
			Operand* op_mul = create_operand(ctx, OP_MUL, mul_label_val);

			TACInstruction* tac_index = create_tac(ctx, TAC_MUL, op_mul, index_tac->result, op_array_val, NULL);
			add_tac_to_table(ctx, tac_index);

			OperandValue add_val = {
				.label_name = tac_variable_create(ctx)
			};
			Operand* op_add = create_operand(ctx, OP_ADD, add_val);

			TACInstruction* address_tac = create_tac(ctx, TAC_ADD, op_add, array_tac->result, tac_index->result, NULL);
			add_tac_to_table(ctx, address_tac);

			OperandValue store_val = {
				.label_name = tac_variable_create(ctx)
			};
			Operand* store_op = create_operand(ctx, OP_STORE, store_val);
			
			TACInstruction* dereference_tac = create_tac(ctx, TAC_DEREFERENCE, store_op, op_add, NULL, NULL);
			add_tac_to_table(ctx, dereference_tac);
			result = dereference_tac;
			break;
		}

		default: {
			printf("\033[31mUNKNOWN NODE in 'build_tac_from_expression_dag' with type %d\033[0m\n", node->type);
			return NULL;
		}
	}
	return result;
}

bool contains_logical_operator(Node* node) {
	if (!node) return false;

	switch (node->type) {
		case NODE_LOGICAL_OR:
		case NODE_LOGICAL_AND: {
			return true;
		}

		case NODE_ARG: {
			return contains_logical_operator(node->right);
		}
		case NODE_CALL: {
			Node* wrapped_arg = node->right;
			while (wrapped_arg) {
				return contains_logical_operator(wrapped_arg);
			}
		}
		default: {
			return contains_logical_operator(node->left) || contains_logical_operator(node->right);
		}
	}

	return false;
}

void build_tac_from_statement_dag(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			if (node->right && node->right->type == NODE_ARRAY_LIST) {
				build_tac_from_array_dagnode(ctx, node->left, node->right);

			} else if (node->left->type == NODE_AUG) {
				if (node->left->left && node->left->left->type == NODE_SUBSCRIPT) {
					// printf("\033[31mWe're here in this case for the NODE_ASSIGNMENT\033[0m\n");
					TACInstruction* right_instruction = build_tac_from_expression_dag(ctx, node->right);
					if (!right_instruction) return;

					TACInstruction* array_tac = build_tac_from_expression_dag(ctx, node->left->left->left);
					TACInstruction* index_tac = build_tac_from_expression_dag(ctx, node->left->left->right);
					if (!array_tac || !index_tac) return;

					OperandValue array_val = {
						.label_name = convert_subtype_to_string(node->left->left->t)
					};
					Operand* op_array_val = create_operand(ctx, OP_SUBTYPE_STR, array_val);
					if (!op_array_val) return;

					OperandValue mul_label_val = {
						.label_name = tac_variable_create(ctx)
					};
					Operand* op_mul = create_operand(ctx, OP_MUL, mul_label_val);
					if (!op_mul) return;

					TACInstruction* offset_tac = create_tac(ctx, TAC_MUL, op_mul, index_tac->result, op_array_val, NULL);
					if (!offset_tac) return;
					add_tac_to_table(ctx, offset_tac);

					OperandValue add_val = {
						.label_name = tac_variable_create(ctx)
					};
					Operand* op_add = create_operand(ctx, OP_ADD, add_val);
					if (!op_add) return;

					TACInstruction* address_tac = create_tac(ctx, TAC_ADD, op_add, array_tac->result, offset_tac->result, NULL);
					if (!address_tac) return;
					add_tac_to_table(ctx, address_tac);

					if (address_tac) {
						TACInstruction* tac_dereference = create_tac(
							ctx,
							TAC_DEREFERENCE_AND_ASSIGN,
							address_tac->result,
							right_instruction->result,
							NULL,
							NULL
						);
						
						add_tac_to_table(ctx, tac_dereference);

					}

				} else {
					TACInstruction* left_instruction = build_tac_from_expression_dag(ctx, node->left);
					TACInstruction* right_instruction = build_tac_from_expression_dag(ctx, node->right);
					
					if (!left_instruction || !right_instruction) {
						printf("\033[31mIn node assignment case -> left or right child is NULL\033[0m\n");
						return;
					}

					if (!left_instruction->result || !right_instruction->result) {
						printf("\033[31mIn Node assignment case -> left result or right result is NULL\033[0m\n");
						return;
					}


					TACInstruction* tac_assignment = create_tac(
						ctx, 
						TAC_ASSIGNMENT, 
						left_instruction->result, 
						NULL, 
						right_instruction->result, 
						NULL
					);

					add_tac_to_table(ctx, tac_assignment);
				}
				
			} else {
				TACInstruction* left_instruction = build_tac_from_expression_dag(ctx, node->left);
				TACInstruction* right_instruction = build_tac_from_expression_dag(ctx, node->right);
					
				if (!left_instruction || !right_instruction) {
					printf("\033[31mIn node assignment case -> left or right child is NULL\033[0m\n");
					return;
				}

				if (!left_instruction->result || !right_instruction->result) {
					printf("\033[31mIn Node assignment case -> left result or right result is NULL\033[0m\n");
					return;
				} 

				TACInstruction* tac_assignment = create_tac(
					ctx, 
					TAC_ASSIGNMENT, 
					left_instruction->result, 
					NULL, 
					right_instruction->result, 
					NULL
				);

				add_tac_to_table(ctx, tac_assignment);
			}
			
			break;
		}

		case NODE_CALL: {
			build_tac_from_expression_dag(ctx, node);
			break;
		}

		case NODE_IF: {	
			bool next_statement = false;
			bool conditional_statement_next = false;
			
			TACContext* retrieved_context = peek_tac_context();
			if (retrieved_context) {
				char* new_if_false_label = generate_label(ctx);
				TACContext* new_context = NULL;
				if (node->next) {
					next_statement = true;
					if (node->next->type != NODE_ELSE_IF && node->next->type != NODE_ELSE) {
						new_context = create_tac_context(
							ctx,
							TAC_IF,
							NULL,
							retrieved_context->end_label,
							NULL,
							false,
							retrieved_context->depth,
							false
						);	
					} else {
						conditional_statement_next = true;
						new_context = create_tac_context(
							ctx,
							TAC_IF,
							new_if_false_label,
							retrieved_context->end_label,
							NULL,
							true,
							retrieved_context->depth,
							false
						);
					}
				} else {
					new_context = create_tac_context(
						ctx,
						TAC_IF,
						NULL,
						retrieved_context->end_label,
						NULL,
						false,
						retrieved_context->depth,
						false
					);
				}
				if (!new_context) return;
				push_tac_context(ctx, new_context);

				TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
				if (!condition_tac) return;

				OperandValue if_false_val = { .label_name = new_if_false_label };
				Operand* if_false_op = create_operand(ctx, OP_LABEL, if_false_val);

				OperandValue retrieved_end_val = {.label_name = retrieved_context->end_label};
				Operand* retrieved_end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);

				TACInstruction* condition_false_tac = NULL;
				if (next_statement) {
					condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, if_false_op, NULL);
					add_tac_to_table(ctx, condition_false_tac);
				} else {
					condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, retrieved_end_op, NULL);
					add_tac_to_table(ctx, condition_false_tac);
				}

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right);
				}

				TACInstruction* goto_tac = create_tac(ctx, TAC_GOTO, retrieved_end_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, goto_tac);

				if (!next_statement && new_level != -1) {
					clear_tac_contexts(new_level);
					clear_tac_contexts(new_context->depth);
				}
				
			
			} else {
				char* if_false_label = generate_label(ctx);
				char* end_label = generate_label(ctx);

				TACContext* context_if = NULL;
				if (node->next) {
					next_statement = true;
					if (node->next->type != NODE_ELSE_IF && 
						node->next->type != NODE_ELSE) {

						context_if = create_tac_context(
							ctx,
							TAC_IF,
							NULL,
							end_label,
							NULL,
							false,
							depth,
							true
						);

					} else {
						conditional_statement_next = true;
						context_if = create_tac_context(
							ctx, 
							TAC_IF,
							if_false_label,
							end_label,
							NULL,
							true,
							depth,
							true
						);
					}
				} else {
					context_if = create_tac_context(
						ctx,
						TAC_IF,
						NULL,
						NULL,
						end_label,
						false,
						depth,
						true
					);
				}
				if (!context_if) return;
				push_tac_context(ctx, context_if);

				TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
				if (!condition_tac) return;

				OperandValue if_false_val = {.label_name = if_false_label};
				Operand* if_false_op = create_operand(ctx, OP_LABEL, if_false_val);

				OperandValue end_val = {.label_name = end_label};
				Operand* end_op = create_operand(ctx, OP_LABEL, end_val);

				TACInstruction* condition_false_tac = NULL;
				
				if (next_statement) {
					if (!conditional_statement_next) {
						condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, end_op, NULL);
						add_tac_to_table(ctx, condition_false_tac);
						pop_tac_context();
					} else {
						condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, if_false_op, NULL);
						add_tac_to_table(ctx, condition_false_tac);
					}
				} else {
					condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, end_op, NULL);
					add_tac_to_table(ctx, condition_false_tac);
				}

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right);
				}

				if (conditional_statement_next) {
					TACInstruction* goto_tac = create_tac(ctx, TAC_GOTO, end_op, NULL, NULL, NULL);
					add_tac_to_table(ctx, goto_tac);
				}

				if (context_if->root_chain_mem && !conditional_statement_next && new_level != -1) {
					TACInstruction* end_label_tac = create_tac(ctx, TAC_LABEL, end_op, NULL, NULL, NULL);
					add_tac_to_table(ctx, end_label_tac);
					clear_tac_contexts(new_level);
					clear_tac_contexts(context_if->depth);
				}
			}		
			printf("Leaving NODE_IF\n");
			break;
		}

		case NODE_ELSE_IF: {
			printf("Currently processing NODE_ELSE_IF\n");
			TACContext* retrieved_context = peek_tac_context();
			if (!retrieved_context) return;

			OperandValue retrieved_next_val = {
				.label_name = retrieved_context->next_label
			};
			Operand* retrieved_next_op = create_operand(ctx, OP_LABEL, retrieved_next_val);

			OperandValue retrieved_end_val = {
				.label_name = retrieved_context->end_label
			};
			Operand* retrieved_end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);

			TACInstruction* else_if_label_tac = create_tac(ctx, TAC_LABEL, retrieved_next_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, else_if_label_tac);

			char* else_if_condition_true_label = generate_label(ctx);
			char* else_if_condition_false_label = generate_label(ctx);
			
			OperandValue condition_true_val = {
				.label_name = else_if_condition_true_label
			};
			Operand* condition_true_op = create_operand(ctx, OP_STORE, condition_true_val);

			OperandValue condition_false_val = {
				.label_name = else_if_condition_false_label
			};
			Operand* condition_false_op = create_operand(ctx, OP_LABEL, condition_false_val);

			bool next_statement = false; 
			bool conditional_statement_next = false;
			TACContext* context_else_if = NULL;
			
			if (node->next) {
				next_statement = true;
				if (node->next->type != NODE_ELSE_IF && node->next->type != NODE_ELSE) {
					context_else_if = create_tac_context(
						ctx,
						TAC_ELSE_IF,
						NULL,
						retrieved_context->end_label,
						NULL,
						false,
						retrieved_context->depth,
						retrieved_context->root_chain_mem
					);

				} else {
					conditional_statement_next = true;
					context_else_if = create_tac_context(
						ctx,
						TAC_ELSE_IF,
						else_if_condition_false_label,
						retrieved_context->end_label,
						NULL,
						true,
						retrieved_context->depth,
						retrieved_context->root_chain_mem
					);
				}
			}
			if (!context_else_if) return;
			push_tac_context(ctx, context_else_if);
			
			TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
			if (!condition_tac) return;

			TACInstruction* else_if_condition_false_tac = NULL;

			if (next_statement) {
				if (!conditional_statement_next) {
					else_if_condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, retrieved_end_op, NULL);
					add_tac_to_table(ctx, else_if_condition_false_tac);
					clear_tac_contexts(TAC_IF);
				} else {
					else_if_condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, condition_false_op, NULL);
					add_tac_to_table(ctx, else_if_condition_false_tac);
				}
			} else {
				else_if_condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, retrieved_end_op, NULL);
				add_tac_to_table(ctx, else_if_condition_false_tac);
			}

			int new_level = -1;
			if (node->right) {
				new_level = depth++;
				build_tac_from_statement_dag(ctx, node->right);
			}

			TACInstruction* goto_tac = create_tac(ctx, TAC_GOTO, retrieved_end_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, goto_tac);

			if (context_else_if->root_chain_mem && !conditional_statement_next && new_level != -1) {
				TACInstruction* end_label_tac = create_tac(ctx, TAC_LABEL, retrieved_end_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, end_label_tac);
				clear_tac_contexts(new_level);
				clear_tac_contexts(context_else_if->depth);
				depth = 0;
			}
			// pop_tac_context();
			printf("Leaving NODE_ELSE_IF\n");
			break;
		}

		case NODE_ELSE: {
			printf("Currently processing NODE_ELSE\n");
			TACContext* retrieved_context = peek_tac_context();
			if (!retrieved_context) return;

			OperandValue retrieved_next_val = { .label_name = retrieved_context->next_label };
			Operand* retrieved_next_op = create_operand(ctx, OP_LABEL, retrieved_next_val);

			TACInstruction* else_tac = create_tac(ctx, TAC_LABEL, retrieved_next_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, else_tac);

			int new_level = -1;
			if (node->right) {
				new_level = depth++;
				build_tac_from_statement_dag(ctx, node->right);
			}

			// if (retrieved_context->root_chain_mem) {
			OperandValue retrieved_end_val = {.label_name = retrieved_context->end_label};
			Operand* retrieved_end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);
				
			TACInstruction* remaining_tac_instructions_label = create_tac(ctx, TAC_LABEL, retrieved_end_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, remaining_tac_instructions_label);
			if (retrieved_context->root_chain_mem && new_level != -1) {
				clear_tac_contexts(new_level);
				clear_tac_contexts(new_level - 1);
				depth = 0;
			}
			// }
			printf("Leaving NODE_ELSE\n");
			break;
		}

		case NODE_WHILE: {
			char* while_condition_label = generate_label(ctx);
			
			bool next_statement = false;

			TACContext* retrieved_context = peek_tac_context();
			if (retrieved_context) {
				Operand* jmp_op = NULL;
				
				TACContext* new_while_context = NULL;				
				if (node->next) {
					next_statement = true;
					
					OperandValue jmp_val = {.label_name = generate_label(ctx)};
					jmp_op = create_operand(ctx, OP_LABEL, jmp_val);

					new_while_context = create_tac_context(
						ctx,
						TAC_WHILE,
						NULL,
						jmp_val.label_name,
						NULL,
						false,
						retrieved_context->depth,
						false
					);
				} else {
					new_while_context = create_tac_context(
						ctx,
						TAC_WHILE,
						NULL, 
						retrieved_context->end_label,
						NULL,
						false,
						retrieved_context->depth,
						false
					);
				}

				if (!new_while_context) return;
				push_tac_context(ctx, new_while_context);

				OperandValue while_condition_val = {.label_name = while_condition_label};
				Operand* while_condition_op = create_operand(ctx, OP_LABEL, while_condition_val);
				if (!while_condition_op) return;
				
				TACInstruction* tac_while_label = create_tac(ctx, TAC_LABEL, while_condition_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_while_label);					

				TACInstruction* tac_while_condition = build_tac_from_expression_dag(ctx, node->left);
				if (!tac_while_condition) return;

				TACInstruction* tac_if_false_while_condition = NULL;
				if (next_statement) {
					tac_if_false_while_condition = create_tac(ctx, TAC_IF_FALSE, NULL, tac_while_condition->result, jmp_op, NULL);
				
				} else {
					OperandValue retrieved_end_val = {.label_name = retrieved_context->end_label};
					Operand* end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);
					tac_if_false_while_condition = create_tac(ctx, TAC_IF_FALSE, NULL, tac_while_condition->result, end_op, NULL);
				}

				if (!tac_if_false_while_condition) return;
				add_tac_to_table(ctx, tac_if_false_while_condition);

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right);
				}

				TACInstruction* goto_tac = create_tac(ctx, TAC_GOTO, while_condition_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, goto_tac);

				if (!next_statement && new_level != -1) {
					clear_tac_contexts(new_level);
					clear_tac_contexts(new_while_context->depth);
				} else {
					TACInstruction* tac_label = create_tac(ctx, TAC_LABEL, jmp_op, NULL, NULL, NULL);
					add_tac_to_table(ctx, tac_label);

				}
				// pop_tac_context();

			} else {
				OperandValue while_condition_val = { .label_name = while_condition_label };
				Operand* while_condition_op = create_operand(ctx, OP_LABEL, while_condition_val);
				if (!while_condition_op) return;

				TACContext* context_while = create_tac_context(
					ctx, 
					TAC_WHILE, 
					NULL, 
					NULL,
					while_condition_label, 
					false,
					depth,
					true
				);
				if (!context_while) return;
				push_tac_context(ctx, context_while);

				TACInstruction* while_condition_tac = create_tac(ctx, TAC_LABEL, while_condition_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, while_condition_tac);

				TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
				
				OperandValue end_val = {.label_name = generate_label(ctx)};
				Operand* end_op = create_operand(ctx, OP_LABEL, end_val);
				if (!end_op) return;

				TACInstruction* condition_false_tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, end_op, NULL);
				add_tac_to_table(ctx, condition_false_tac);

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right);
				}
				TACInstruction* goto_tac = create_tac(ctx, TAC_GOTO, while_condition_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, goto_tac);

				TACInstruction* end_tac = create_tac(ctx, TAC_LABEL, end_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, end_tac);

				if (context_while->root_chain_mem && new_level != -1) {
					clear_tac_contexts(new_level);
					clear_tac_contexts(context_while->depth);
				}
				// pop_tac_context();

			}
			break;
		}

		case NODE_FOR: {
			bool next_statement = false;

			TACContext* retrieved_context = peek_tac_context();
			if (retrieved_context) {
				OperandValue loop_start_val = {.label_name = generate_label(ctx)};
				Operand* loop_start_op = create_operand(ctx, OP_LABEL, loop_start_val);

				Operand* jmp_op = NULL;
				TACContext* context_node_for = NULL;
				if (node->next) {
					next_statement = true;

					OperandValue jmp_val = {.label_name = generate_label(ctx)};
					jmp_op = create_operand(ctx, OP_LABEL, jmp_val);
					
					context_node_for = create_tac_context(
						ctx,
						TAC_FOR,
						NULL,
						jmp_val.label_name,
						NULL,
						false,
						retrieved_context->depth,
						false
					);
				} else {
					context_node_for = create_tac_context(
						ctx,
						TAC_FOR,
						NULL,
						retrieved_context->end_label,
						NULL,
						false,
						retrieved_context->depth,
						false
					);
				}

				if (!context_node_for) return;
				push_tac_context(ctx, context_node_for);

				Node* initializer_node = node->left ? node->left : NULL;
				build_tac_from_statement_dag(ctx, initializer_node);

				TACInstruction* tac_loop_start_label = create_tac(ctx, TAC_LABEL, loop_start_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_loop_start_label);

				Node* condition_node = initializer_node ? initializer_node->next : NULL;
				TACInstruction* tac_loop_condition = build_tac_from_expression_dag(ctx, condition_node);
				if (!tac_loop_condition) return;

				TACInstruction* tac_loop_condition_false = NULL;
				if (next_statement) {
					tac_loop_condition_false = create_tac(ctx, TAC_IF_FALSE, NULL, tac_loop_condition->result, jmp_op, NULL);
				} else {
					OperandValue retrieved_end_val = {.label_name = retrieved_context->end_label};
					Operand* end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);
					tac_loop_condition_false = create_tac(ctx, TAC_IF_FALSE, NULL, tac_loop_condition->result, end_op, NULL);
				}

				if (!tac_loop_condition_false) return;
				add_tac_to_table(ctx, tac_loop_condition_false);

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right);	
				}

				OperandValue loop_update_val = {.label_name = generate_label(ctx)};
				Operand* loop_update_op = create_operand(ctx, OP_LABEL, loop_update_val);
				TACInstruction* tac_update_label = create_tac(ctx, TAC_LABEL, loop_update_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_update_label);

				Node* update_node = condition_node ? condition_node->next : NULL;
				build_tac_from_statement_dag(ctx, update_node);

				TACInstruction* tac_goto = create_tac(ctx, TAC_GOTO, loop_start_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_goto);

				if (!next_statement && new_level != -1) {
					clear_tac_contexts(new_level);
					clear_tac_contexts(context_node_for->depth);
				}

			} else {
				char* loop_start_label = generate_label(ctx);
				char* loop_update_label = generate_label(ctx);
				char* loop_end_label = generate_label(ctx);

				OperandValue loop_start_val = { .label_name = loop_start_label };
				OperandValue loop_update_val = { .label_name = loop_update_label };
				OperandValue loop_end_val = { .label_name = loop_end_label };

				Operand* loop_start_op = create_operand(ctx, OP_LABEL, loop_start_val);
				Operand* loop_update_op = create_operand(ctx, OP_LABEL, loop_update_val);
				Operand* loop_end_op = create_operand(ctx, OP_LABEL, loop_end_val);

				TACContext* context_for = create_tac_context(
					ctx, 
					TAC_FOR, 
					NULL, 
					loop_end_label,
					loop_update_label, 
					false,
					depth,
					true
				);

				push_tac_context(ctx, context_for);

				Node* initializer_node = node->left ? node->left : NULL;
				build_tac_from_statement_dag(ctx, initializer_node);

				TACInstruction* tac_loop_start_label = create_tac(ctx, TAC_LABEL, loop_start_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_loop_start_label);
				Node* condition_node = initializer_node ? initializer_node->next : NULL;

				TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, condition_node);
				
				TACInstruction* tac_false_label = create_tac(ctx, TAC_IF_FALSE, NULL, condition_tac->result, loop_end_op, NULL);
				add_tac_to_table(ctx, tac_false_label);

				int new_level = -1;
				if (node->right) {
					new_level = depth++;
					build_tac_from_statement_dag(ctx, node->right); 
				}
				
				TACInstruction* tac_update_label = create_tac(ctx, TAC_LABEL,loop_update_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_update_label);
				Node* update_node = condition_node ? condition_node->next : NULL;
				
				build_tac_from_statement_dag(ctx, update_node);

				TACInstruction* tac_goto = create_tac(ctx, TAC_GOTO, loop_start_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_goto);

				TACInstruction* tac_loop_end_label = create_tac(ctx, TAC_LABEL, loop_end_op, NULL, NULL, NULL);
				add_tac_to_table(ctx, tac_loop_end_label);

				if (context_for->root_chain_mem && new_level != -1) {
					clear_tac_contexts(new_level);
					clear_tac_contexts(context_for->depth);					
				}
			}
			break;
		}

		case NODE_RETURN: {
			TACInstruction* tac_result = NULL;
			if (!node->right) {
				tac_result = create_tac(ctx, TAC_RETURN, NULL, NULL, NULL, NULL);
			} else {
				TACInstruction* tac_return = build_tac_from_expression_dag(ctx, node->right);
				if (!tac_return) {
					return;
				}
				tac_result = create_tac(ctx, TAC_RETURN, NULL, tac_return->result, NULL, NULL);
			}
			add_tac_to_table(ctx, tac_result);
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			Node* int_node = create_int_node(ctx, NODE_INTEGER, 1, NULL, NULL, NULL, NULL, NULL);
			if (!int_node) return;

			node_t kind = (node->type == NODE_INCREMENT) ? NODE_ADD : NODE_SUB;
			Node* op_dagnode = create_node(ctx, kind, node->left, int_node, NULL, NULL, NULL);	
			if (!op_dagnode) return;

			TACInstruction* op_dagnode_instruction = build_tac_from_expression_dag(ctx, op_dagnode);
			TACInstruction* left_instruction = build_tac_from_expression_dag(ctx, node->left);

			if (!op_dagnode_instruction || !left_instruction) return;
			TACInstruction* tac_assignment = create_tac(ctx, TAC_ASSIGNMENT, left_instruction->result, NULL, op_dagnode_instruction->result, NULL);
			add_tac_to_table(ctx, tac_assignment);
			break;
		}

		case NODE_BREAK: {
			printf("\033[31mIN DAG_BREAK CASE\033[0m\n");
			tac_t tac_target_types[2] = {TAC_FOR, TAC_WHILE};
			size_t length = sizeof(tac_target_types) / sizeof(tac_target_types[0]);
			TACContext* context_break = tac_context_lookup(tac_target_types, length);
			if (!context_break) return;

			OperandValue retrieved_end_val = { .label_name = context_break->end_label };
			Operand* end_op = create_operand(ctx, OP_LABEL, retrieved_end_val);
			TACInstruction* tac_break = create_tac(ctx, TAC_GOTO, end_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, tac_break);
			printf("\033[31mLeaving DAG_BREAK CASE\033[0m\n");
			break;
		}

		case NODE_CONTINUE: {
			printf("\033[31mIN DAG_CONTINUE CASE\033[0m\n");
			tac_t tac_target_types[2] = {TAC_FOR, TAC_WHILE};
			size_t length = sizeof(tac_target_types) / sizeof(tac_target_types[0]);
			TACContext* context_continue = tac_context_lookup(tac_target_types, length);
			if (!context_continue) return;

			OperandValue retrieved_update_val = { .label_name = context_continue->update_label };
			Operand* update_op = create_operand(ctx, OP_LABEL, retrieved_update_val);
			TACInstruction* tac_continue = create_tac(ctx, TAC_GOTO, update_op, NULL, NULL, NULL);
			add_tac_to_table(ctx, tac_continue);
			printf("\033[31mLeaving DAG_CONTINUE CASE\033[0m\n");
			break;
		}

		case NODE_BLOCK: {
			Node* stmt = node->right;
			while (stmt) {
				Node* next_stmt = stmt->next;
				build_tac_from_statement_dag(ctx, stmt);
				stmt = next_stmt;
			}
			break;
		}
	}
}

void build_tac_from_parameter_dag(CompilerContext* ctx, Node* wrapped_param) {
	if (!wrapped_param) return;

	TACInstruction* result = NULL;
	
	Node* current_wrapped_param = wrapped_param;
	while (current_wrapped_param) {
		Node* next_wrapped_param = current_wrapped_param->next;

		OperandValue parameter_val = { .label_name = tac_parameter_label(ctx) };
		Operand* parameter_op = create_operand(ctx, OP_LABEL, parameter_val);

		TACInstruction* param_instruction = build_tac_from_expression_dag(ctx, current_wrapped_param->right);
		result = create_tac(ctx, TAC_PARAM, parameter_op, param_instruction->result, NULL, NULL);
		add_tac_to_table(ctx, result);

		current_wrapped_param = next_wrapped_param;
	}
}

void build_tac_from_global_dag(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_NAME: {
			if (node->symbol) {
				if (node->symbol->type) {
					if (node->symbol->type->kind == TYPE_FUNCTION) {
						tac_instruction_index = 0;
						tac_parameter_index = 0;
						tac_function_argument_index = 0;
						printf("\033[31mBUILDING TACS FOR FUNCTION '%s'\033[0m\n", node->value.name);
						OperandValue func_val = { .sym = node->symbol };
						Operand* func_op = create_operand(ctx, OP_SYMBOL, func_val);
						TACInstruction* tac_function = create_tac(ctx, TAC_NAME, func_op, NULL, NULL, NULL);
						add_tac_to_table(ctx, tac_function);

						build_tac_from_parameter_dag(ctx, node->t->params);
						build_tac_from_statement_dag(ctx, node->right);
						printf("\033[31mFINISHED BUILDING TACS FOR FUNCTION '%s'\033[0m\n", node->value.name);
					}
				} 
			}
			break;
		}
	}
}

TACTable* build_tacs(CompilerContext* ctx, Node* node) {
	if (!node) return NULL;

	init_tac_table(ctx);
	init_tac_context_stack(ctx);
	Node* current = node;
	while (current) {
		Node* next = current->next;
		build_tac_from_global_dag(ctx, current);
		current = next;
	}

	emit_tac_instructions();

	return tac_table;
}

char* get_tac_op(TACInstruction* tac) {
	if (!tac) return NULL;

	switch (tac->type) {
		case TAC_UNARY_ADD:
		case TAC_ADD: {
			return "+";
		}
		case TAC_UNARY_SUB:
		case TAC_SUB: {
			return "-";
		}
		case TAC_MUL: return "*";
		case TAC_DIV: return "/";
		case TAC_LESS: return "<";
		case TAC_GREATER: return ">";
		case TAC_LESS_EQUAL: return "<=";
		case TAC_GREATER_EQUAL: return ">=";
		case TAC_EQUAL: return "==";
		case TAC_NOT_EQUAL: return "!=";
		case TAC_NOT: return "!";
		case TAC_LOGICAL_AND: return "&&";
		case TAC_LOGICAL_OR: return "||";
		case TAC_MODULO: return "%";
	}
}

bool is_op(operand_t type) {
	switch (type) {
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_LESS:
		case OP_LESS_EQUAL:
		case OP_GREATER:
		case OP_GREATER_EQUAL:
		case OP_EQUAL:
		case OP_NOT_EQUAL:
		case OP_LOGICAL_OR:
		case OP_LOGICAL_AND:
		case OP_MODULO:	
		case OP_NOT: {
			return true;
		}
		default: return false;
	}
}

void emit_tac_instructions() {
	if (!tac_table || (tac_table && !tac_table->tacs)) return;
	printf("we have tac table and tac table instructions\n");

	for (int i = 0; i < tac_table->size; i++) {
		TACInstruction* current = tac_table->tacs[i];
		switch (current->type) {
			case TAC_NAME: {
				printf("\n%s\n", current->result->value.sym->name);
				break;
			}
			case TAC_CHAR:
			case TAC_BOOL:
			case TAC_INTEGER: {
				printf("\t%s = %d\n", current->result->value.label_name, current->op1->value.int_val);
				break;
			} 

			case TAC_UNARY_ADD:
			case TAC_UNARY_SUB:
			case TAC_NOT: {
				char* op = get_tac_op(current);
				if (current->op1 && op) {
					switch (current->op1->kind) {
						case OP_SYMBOL: {
							printf("\t%s = %s%s\n", current->result->value.label_name, op, current->op1->value.sym->name);
							break;
						}
						case OP_INT_LITERAL: {
							printf("\t%s = %s%d\n", current->result->value.label_name, op, current->op1->value.int_val);
							break;
						}

						case OP_ADD:
						case OP_SUB:
						case OP_MUL:
						case OP_DIV:
						case OP_MODULO:
						case OP_LESS:
						case OP_GREATER:
						case OP_LESS_EQUAL:
						case OP_GREATER_EQUAL:
						case OP_NOT:
						case OP_EQUAL:
						case OP_NOT_EQUAL:
						case OP_UNARY_ADD:
						case OP_UNARY_SUB:
						case OP_LOGICAL_OR:
						case OP_LOGICAL_AND:
						case OP_STORE: {	
							printf("\t%s = %s%s\n", current->result->value.label_name, op, current->op1->value.label_name);
							break;
						}
					}
				}
				
				break;
			}

			case TAC_ASSIGNMENT: {
				switch (current->result->kind) {
					case OP_SYMBOL: {
						printf("\t%s = ", current->result->value.sym->name ? current->result->value.sym->name : NULL);
						break;
					}

					case OP_STORE: {
						printf("\t%s = ", current->result->value.label_name);
						break;
					}
				}

				switch (current->op2->kind) {
					case OP_SYMBOL: {
						printf("%s\n", current->op2->value.sym->name ? current->op2->value.sym->name : NULL);
						break;
					}

					case OP_STORE:
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MODULO:
					case OP_LESS:
					case OP_GREATER:
					case OP_LESS_EQUAL:
					case OP_GREATER_EQUAL:
					case OP_EQUAL:
					case OP_NOT_EQUAL:
					case OP_NOT:
					case OP_LOGICAL_OR:
					case OP_LOGICAL_AND:
					case OP_RETURN: {
						printf("%s\n", current->op2->value.label_name);
						break;
					}

					case OP_INT_LITERAL: {
						printf("%d\n", current->op2->value.int_val);
						break;
					}
				}
				break;
			}

			case TAC_IF_FALSE: {
				if (current->op1->kind == OP_SYMBOL && current->op2->kind == OP_LABEL) {
					printf("\tIF_FALSE %s GOTO %s\n",current->op1->value.sym->name, current->op2->value.label_name);
				} else {
					printf("\tIF_FALSE %s GOTO %s\n", current->op1->value.label_name, current->op2->value.label_name);
				}
				break;
			}

			case TAC_STORE: {
				if (current->op1->kind == OP_INT_LITERAL) {
					printf("\t*%s = %d\n", current->result->value.label_name, current->op1->value.int_val);
				} else if (current->op1->kind == OP_LABEL) {
					printf("\t*%s = %s\n", current->result->value.label_name, current->op1->value.label_name);
				}
				break;
			}

			case TAC_DEREFERENCE: {
				printf("\t%s = *%s\n", current->result->value.label_name, current->op1->value.label_name);
				break;
			}

			case TAC_DEREFERENCE_AND_ASSIGN: {
				printf("\t*%s = ", current->result->value.label_name);
				switch (current->op1->kind) {
					case OP_SYMBOL: {
						printf("%s\n", current->op1->value.sym->name);
						break;
					}
					default: {
						printf("%s\n", current->op1->value.label_name);
						break;
					}
				}
				break;
			}
			
			case TAC_ADD_EQUAL:
			case TAC_SUB_EQUAL:
			case TAC_DIV_EQUAL:
			case TAC_MUL_EQUAL:
			case TAC_EQUAL:
			case TAC_NOT_EQUAL:
			case TAC_MODULO:
			case TAC_LESS:
			case TAC_GREATER:
			case TAC_GREATER_EQUAL:
			case TAC_LESS_EQUAL:
			case TAC_LOGICAL_OR:
			case TAC_LOGICAL_AND:
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV: {
				
				printf("\t%s = ", current->result->value.label_name);
				switch (current->op1->kind) {
					case OP_SYMBOL: {
						printf("%s ", current->op1->value.sym->name);
						break;
					}

					case OP_SUBTYPE_STR:
					case OP_STORE:
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_LESS:
					case OP_GREATER:
					case OP_LESS_EQUAL:
					case OP_GREATER_EQUAL:
					case OP_MODULO: {
						printf("%s ", current->op1->value.label_name);
						break;
					}

					case OP_INT_LITERAL: {
						printf("%d ", current->op1->value.int_val);
						break;
					}
				}

				printf("%s ", get_tac_op(current));

				switch (current->op2->kind) {
					case OP_SYMBOL: {
						printf("%s\n", current->op2->value.sym->name);
						break;
					}

					case OP_SUBTYPE_STR:
					case OP_STORE:
					case OP_ADD:
					case OP_SUB:
					case OP_DIV:
					case OP_MUL:
					case OP_LESS:
					case OP_GREATER:
					case OP_LESS_EQUAL:
					case OP_GREATER_EQUAL:
					case OP_MODULO: {
						// printf("processing op kind %d\n", current->op2->kind);
						// if (!current->op2->value.label_name) {
						// 	printf("dont have label name\n");
						// }
						printf("%s\n", current->op2->value.label_name);
						break;
					}

					case OP_INT_LITERAL: {
						printf("%d\n", current->op2->value.int_val);
						break;
					}
				}
				
				break;
			}

			case TAC_LABEL: {
				printf("%s\n", current->result->value.label_name);
				break;
			}

			case TAC_GOTO: {
				printf("\tGOTO %s\n", current->result->value.label_name);
				break;
			}
			
			case TAC_PARAM: {
				printf("\tPARAM %s, %s\n", current->result->value.label_name, current->op1->value.sym->name);
				break;
			}
			case TAC_ARG: {
				if (is_op(current->op1->kind) || current->op1->kind == OP_STORE) {
					printf("\tARG %s, %s\n", current->result->value.label_name, current->op1->value.label_name);
				
				} else if (current->op1->kind == OP_SYMBOL) {
					printf("\tARG %s, %s\n", current->result->value.label_name, current->op1->value.sym->name);
				}
				break;
			}

			case TAC_CALL: {
				printf("\tCALL %s\n", current->result->value.sym->name);
				break;
			}

			case TAC_RETURN: {
				printf("\tRETURN");
				if (current->op1) {
					switch (current->op1->kind) {
						case OP_SYMBOL: {
							printf(" %s\n", current->op1->value.sym->name);
							break;
						}
						case OP_UNARY_ADD:
						case OP_UNARY_SUB:
						case OP_ADD:
						case OP_SUB:
						case OP_MUL:
						case OP_DIV:
						case OP_MODULO:
						case OP_LESS:
						case OP_GREATER:
						case OP_LESS_EQUAL:
						case OP_GREATER_EQUAL:
						case OP_EQUAL:
						case OP_NOT_EQUAL:
						case OP_LOGICAL_OR:
						case OP_LOGICAL_AND:
						case OP_NOT:
						case OP_SUBTYPE_STR:
						case OP_STORE: {
							printf(" %s\n", current->op1->value.label_name);
							break;
						}
					}
				}
				break;
			}
		}
	}
} 