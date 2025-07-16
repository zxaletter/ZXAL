#include "tac.h"

static int label_counter = 0;
static int tac_variable_index = 0;
static int tac_parameter_index = 0;
static int tac_function_argument_index = 0;
static int tac_instruction_index = 0;

TACTable* tac_table = NULL;
TACContextStack tac_context_stack;

bool init_tac_table(CompilerContext* ctx) {
	tac_table = create_tac_table(ctx);
	if (!tac_table) return false;
	return true;
}

TACTable* create_tac_table(CompilerContext* ctx) {
	TACTable* table = arena_allocate(ctx->ir_arena, sizeof(TACTable));
	if (!table) return NULL;

	table->size = 0;
	table->capacity = INITIAL_TABLE_CAPACITY;
	table->tacs = arena_allocate(ctx->ir_arena, sizeof(TACInstruction*) * table->capacity);

	if (!table->tacs) return NULL;
	return table;
}

TACInstruction* create_tac(CompilerContext* ctx, tac_t type, Operand* result, Operand* op1, Operand* op2, Operand* op3) {
	TACInstruction* tac = arena_allocate(ctx->ir_arena, sizeof(TACInstruction));
	if (!tac) return NULL;

	tac->type = type;
	tac->id = tac_instruction_index++;
	tac->result = result;
	tac->op1 = op1;
	tac->op2 = op2;
	tac->op3 = op3;
	return tac;
}

bool init_tac_context_stack(CompilerContext* ctx) {
	tac_context_stack = create_tac_context_stack(ctx);
	if (!tac_context_stack.contexts) return false;
	return true;
}

TACContextStack create_tac_context_stack(CompilerContext* ctx) {
	TACContext** contexts = arena_allocate(
		ctx->ir_arena, 
		sizeof(TACContext*) * INITIAL_TACCONTEXT_CAPACITY
	);

	if (!contexts) {
		TACContextStack dummy_stack = {
			.top = -1,
			.capacity = 0,
			.contexts = NULL
		};
		return dummy_stack;
	}

	TACContextStack new_tac_context_stack = {
		.top = -1,
		.capacity = INITIAL_TACCONTEXT_CAPACITY,
		.contexts = contexts
	};
	return new_tac_context_stack;
}

TACContext* create_tac_context(CompilerContext* ctx, context_t type, char* next_label, char* end_label, 
	char* update_label, bool root_chain_mem) {

	TACContext* tac_context = arena_allocate(ctx->ir_arena, sizeof(TACContext));
	if (!tac_context) {
		perror("In 'create_context_stack', unable to allocate space for tac context.\n");
		return NULL;
	}

	tac_context->type = type;
	tac_context->next_label = next_label;
	tac_context->end_label = end_label;
	tac_context->update_label = update_label;
	tac_context->root_chain_mem = root_chain_mem;

	tac_context->parent = peek_tac_context();

	return tac_context;
}

TACContext* tac_context_lookup(tac_t* tac_target_types, size_t length) {
	if (!tac_target_types || !tac_context_stack.contexts) return NULL;

	if (length <= 0) return NULL;
			
	for (int i = tac_context_stack.top; i >= 0; i--) {
		TACContext* current_context = tac_context_stack.contexts[i];
		if (current_context) {
			for (int j = 0; j < length; j++) {
				if (current_context->type == tac_target_types[j]) {
					return current_context;
				}
			}			
		}
	}
	
	return NULL;
}

bool is_tac_context_stack_empty() {
	return tac_context_stack.top == -1;
} 

void push_tac_context(CompilerContext* ctx, TACContext* context) {
	if (!context || !tac_context_stack.contexts) return;

	if (tac_context_stack.top >= tac_context_stack.capacity) {
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
	}
}

void reset_context_stack() {
	if (!is_tac_context_stack_empty()) {
		while (tac_context_stack.top >= 0) {
			tac_context_stack.top--;
		}
	}
}

char* generate_label(CompilerContext* ctx, LabelKind kind) {
	char buffer[52];

	switch (kind) {
		case VIRTUAL: {
			snprintf(buffer, sizeof(buffer), "t%d", tac_variable_index++);
			break;
		}

		case ARG_LABEL: {
			snprintf(buffer, sizeof(buffer), "a%d", tac_function_argument_index++);
			break;
		}

		case PARAM_LABEL: {
			snprintf(buffer, sizeof(buffer), "p%d", tac_parameter_index++);
			break;
		}

		case REG_LABEL: {
			snprintf(buffer, sizeof(buffer), ".L%d", label_counter++);
			break;
		}
	}


	char* label = NULL;
	if (buffer) {
		int length = strlen(buffer);
		label = arena_allocate(ctx->ir_arena, length + 1);
		if (!label) return NULL;
		
		strncpy(label, buffer, length);
		label[length] = '\0';
	} 
	return label;
} 

char* tac_function_name(CompilerContext* ctx, char* function_name) {
	if (!function_name) return NULL;

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
	tac_table->tacs[tac_table->size++] = tac;
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
	if (!new_operand) return NULL;

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

char* convert_subtype_to_string(struct Type* t) {
	if (!t) return NULL;

	switch (t->kind) {
		case TYPE_INTEGER: return "SIZEOF_INT";
		case TYPE_CHAR: return "SIZEOF_CHAR";
		case TYPE_BOOL: return "SIZEOF_BOOL";
		case TYPE_ARRAY: {
			return convert_subtype_to_string(t->subtype);
		}
		default: return NULL;
	}
}

TACInstruction* build_tac_from_array_dagnode(CompilerContext* ctx, Node* array_identifier, Node* array_list) {
	if (!array_identifier) return NULL;

	OperandValue array_subtype_val;
	Operand* array_subtype_op = NULL;
	if (array_identifier->left) {
		array_subtype_val.label_name = convert_subtype_to_string(array_identifier->left->t);
		array_subtype_op = create_operand(ctx, OP_SUBTYPE_STR, array_subtype_val);
	}

	OperandValue array_identifier_val;
	Operand* array_identifier_op = NULL;
	if (array_identifier->left) {
		array_identifier_val.sym = array_identifier->left->symbol;
		array_identifier_op = create_operand(ctx, OP_SYMBOL, array_identifier_val);
	}

	int i = 0;
	Node* element = array_list->right;
	while (element) {
		Node* next_element = element->next;

		OperandValue element_index_val = { 
			.label_name = generate_label(ctx, VIRTUAL) 
		};
		Operand* element_index_op = create_operand(ctx, OP_MUL, element_index_val);
		
		OperandValue buffer_val = {
			.int_val = i 
		};
		Operand* buffer_op = create_operand(ctx, OP_INT_LITERAL, buffer_val);
		
		TACInstruction* element_index_tac = create_tac(ctx, TAC_MUL, element_index_op, buffer_op, array_subtype_op, NULL);
		add_tac_to_table(ctx, element_index_tac);

		OperandValue element_address_val = { 
			.label_name = generate_label(ctx, VIRTUAL) 
		};
		Operand* element_address_op = create_operand(ctx, OP_STORE, element_address_val);

		TACInstruction* pos_tac = create_tac(ctx, TAC_ADD, element_address_op, array_identifier_op, element_index_op, NULL);
		add_tac_to_table(ctx, pos_tac);

		OperandValue element_val = { 
			.int_val = element->value.val 
		};
		Operand* element_op = create_operand(ctx, OP_INT_LITERAL, element_val);

		TACInstruction* tac_store_element = create_tac(ctx, TAC_STORE, element_address_op, element_op, NULL, NULL); 
		add_tac_to_table(ctx, tac_store_element);
		
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
	if (!node) {
		return NULL;
	}
	 
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
			printf("\033[32mIn BINARY NODE\033[0m\n");		
			tac_t type = get_tac_type(node->type);
			printf("got here\n");
			TACInstruction* left = build_tac_from_expression_dag(ctx, node->left);
			TACInstruction* right = build_tac_from_expression_dag(ctx, node->right);

			if (!left || !right) return NULL;
			printf("here\n");
			OperandValue main_operand_value = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			operand_t op_type = node_to_operand_type(node->type);
			Operand* binary_operand = create_operand(ctx, op_type, main_operand_value);
			printf("About to create tac\n");
			result = create_tac(ctx, type, binary_operand, left->result, right->result, NULL);
			add_tac_to_table(ctx, result);
			printf("\033[32mLeaving BINARY NODE\033[0m\n");
			break;
		}

		case NODE_UNARY_ADD:
		case NODE_UNARY_SUB: {
			printf("\033[32mIn NODE_UNARY_ADD/SUB\033[0m\n");
			TACInstruction* unary_tac = build_tac_from_expression_dag(ctx, node->right);
			if (!unary_tac) return NULL;

			OperandValue val = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			operand_t type = (node->type == NODE_UNARY_ADD) ? OP_UNARY_ADD : OP_UNARY_SUB;
			Operand* op = create_operand(ctx, type, val);

			tac_t kind = (type == OP_UNARY_ADD) ? TAC_UNARY_ADD : TAC_UNARY_SUB; 
			result = create_tac(ctx, kind, op, unary_tac->result, NULL, NULL);
			add_tac_to_table(ctx, result);
			printf("\033[32mLeaving NODE_UNARY_ADD/SUB\033[0m\n");
			break;
		}

		case NODE_NOT: {
			printf("\033[32mIn NODE_NOT\033[0m\n");
			TACInstruction* unary_tac = build_tac_from_expression_dag(ctx, node->right);
			if (!unary_tac) return NULL;

			OperandValue val = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			Operand* op = create_operand(ctx, OP_NOT, val);
			
			result = create_tac(ctx, TAC_NOT, op, unary_tac->result, NULL, NULL);
			add_tac_to_table(ctx, result);
			printf("\033[32mLeaving NODE_NOT\033[0m\n");
			break;
		}

		case NODE_CHAR:
		case NODE_BOOL:
		case NODE_INTEGER: {
			printf("\033[32mIn NODE INTEGER|BOOL|CHAR\033[0m\n");
			tac_t type = get_tac_type(node->type);
			
			OperandValue tac_variable_union = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			Operand* left_operand = create_operand(ctx, OP_STORE, tac_variable_union);

			OperandValue buffer_union = {
				.int_val = node->value.val,
			};
			Operand* right_operand = create_operand(ctx, OP_INT_LITERAL, buffer_union);
			
			result = create_tac(ctx, type, left_operand, right_operand, NULL, NULL);
			add_tac_to_table(ctx, result);
			printf("\033[32mLeaving NODE_INTEGER|BOOL|CHAR\033[0m\n");
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			printf("\033[32mIn NODE_INCREMENT|DECREMENT\033[0m\n");
			tac_t type = get_tac_type(node->type);
			char* tac_variable = generate_label(ctx, VIRTUAL);

			Node* int_node = create_int_node(ctx, NODE_INTEGER, 1, NULL, NULL, NULL, NULL, NULL, NULL);

			node_t kind = (node->type == NODE_INCREMENT) ? NODE_ADD : NODE_SUB;
			Node* op_node = create_node(ctx, kind, node->left, int_node, NULL, NULL, NULL, NULL);
			
			TACInstruction* tac = build_tac_from_expression_dag(ctx, op_node);
			build_tac_from_expression_dag(ctx, node->left);
			
			if (!tac || (tac && !tac->result)) return NULL;

			OperandValue res_val;
			Operand* res_op = NULL;
			if (node->left) {
				res_val.sym = node->left->symbol;
				res_op = create_operand(ctx, OP_SYMBOL, res_val);
			}

			TACInstruction* tac_assignment = create_tac(ctx, TAC_ASSIGNMENT, res_op, NULL, tac->result, NULL);
			add_tac_to_table(ctx, tac_assignment);
			
			result = tac_assignment;		
			printf("\033[32mLeaving NODE_INCREMENT|DECREMENT\033[0m\n");
			break;
		}

		case NODE_AUG:
		case NODE_DECL:
		case NODE_DEF: {
			result = build_tac_from_expression_dag(ctx, node->left);
			break;
		}

		case NODE_NAME: {		
			printf("\033[32mIn NODE_NAME\033[0m\n");	
			OperandValue sym_val;
			Operand* sym_op = NULL;
			
			if (node->symbol) {
				sym_val.sym = node->symbol;
				sym_op = create_operand(ctx, OP_SYMBOL, sym_val);
			}

			result = create_tac(ctx, TAC_NAME, sym_op, NULL, NULL, NULL);	
			printf("\033[32mLeaving NODE_NAME\033[0m\n");
			break;
		}

		case NODE_ARG: {
			OperandValue arg_value_label = {
				.label_name = generate_label(ctx, ARG_LABEL)
			};
			Operand* left_operand = create_operand(ctx, OP_ARG, arg_value_label);

			TACInstruction* arg = build_tac_from_expression_dag(ctx, node->right);
			if (!arg) return NULL;

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
			if (!function_name) return NULL;

			TACInstruction* tac_call = create_tac(ctx, TAC_CALL, function_name->result, NULL, NULL, NULL);
			add_tac_to_table(ctx, tac_call);

			OperandValue func_return_val = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			Operand* function_label_operand = create_operand(ctx, OP_STORE, func_return_val);

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

			OperandValue array_val = { .label_name = convert_subtype_to_string(node->t) };
			Operand* op_array_val = create_operand(ctx, OP_SUBTYPE_STR, array_val);


			OperandValue mul_label_val = {.label_name = generate_label(ctx, VIRTUAL)};
			Operand* op_mul = create_operand(ctx, OP_MUL, mul_label_val);

			TACInstruction* tac_index = create_tac(ctx, TAC_MUL, op_mul, index_tac->result, op_array_val, NULL);
			add_tac_to_table(ctx, tac_index);

			
			OperandValue add_val = { .label_name = generate_label(ctx, VIRTUAL) };
			Operand* op_add = create_operand(ctx, OP_ADD, add_val);


			TACInstruction* address_tac = create_tac(ctx, TAC_ADD, op_add, array_tac->result, tac_index->result, NULL);
			add_tac_to_table(ctx, address_tac);

			OperandValue store_val = { .label_name = generate_label(ctx, VIRTUAL) };
			Operand* store_op = create_operand(ctx, OP_STORE, store_val);
			
			TACInstruction* dereference_tac = create_tac(ctx, TAC_DEREFERENCE, store_op, op_add, NULL, NULL);
			add_tac_to_table(ctx, dereference_tac);
			result = dereference_tac;
			break;
		}

		default: {
			return NULL;
		}
	}
	return result;
}

void emit_label(CompilerContext* ctx, char* label) {
	OperandValue val = {.label_name = label};
	Operand* op = create_operand(ctx, OP_LABEL, val);
	TACInstruction* tac = create_tac(ctx, TAC_LABEL, op, NULL, NULL, NULL);
	add_tac_to_table(ctx, tac);
}

void emit_if_false(CompilerContext* ctx, Operand* condition, char* target) {
	OperandValue val = {.label_name = target};
	Operand* op = create_operand(ctx, OP_LABEL, val);
	TACInstruction* tac = create_tac(ctx, TAC_IF_FALSE, NULL, condition, op, NULL);
	add_tac_to_table(ctx, tac);
}

void emit_goto(CompilerContext* ctx, char* target) {
	OperandValue val = {.label_name = target};
	Operand* op = create_operand(ctx, OP_LABEL, val);
	TACInstruction* tac = create_tac(ctx, TAC_GOTO, op, NULL, NULL, NULL);
	add_tac_to_table(ctx, tac);
}

bool determine_if_next_conditional(Node* node, bool has_next_statement) {
	return has_next_statement && (node->next->type == NODE_ELSE_IF || node->next->type == NODE_ELSE);
}

void set_end_label_based_on_context(TACContext* context, char** end_label) {
	switch (context->type) {
		case CONTEXT_LOOP: {
			*end_label = context->update_label;
			break;
		}

		default: 
			*end_label = context->end_label;
			break;
	}
}

TACContext* context_loop_lookup(TACContext* context) {
	TACContext* current = context;
	while (current) {
		if (current->type == CONTEXT_LOOP) {
			return current;
		} 
		current = current->parent;
	}

	return NULL;
}

void build_tac_from_statement_dag(CompilerContext* ctx, Node* node) {
	if (!node) return;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			printf("\033[32mIn NODE_ASSIGNMENT\033[0m\n");
			if (node->right && node->right->type == NODE_ARRAY_LIST) {
				build_tac_from_array_dagnode(ctx, node->left, node->right);

			} else if (node->left->type == NODE_AUG) {
				if (node->left->left && node->left->left->type == NODE_SUBSCRIPT) {
					TACInstruction* right = build_tac_from_expression_dag(ctx, node->right);

					TACInstruction* array_tac = build_tac_from_expression_dag(ctx, node->left->left->left);
					TACInstruction* index_tac = build_tac_from_expression_dag(ctx, node->left->left->right);
					if (!array_tac || !index_tac) return;


					OperandValue array_val = { .label_name = convert_subtype_to_string(node->left->left->t) };
					Operand* op_array_val = create_operand(ctx, OP_SUBTYPE_STR, array_val);
					if (!op_array_val) return;


					OperandValue mul_label_val = { .label_name = generate_label(ctx, VIRTUAL) };
					Operand* op_mul = create_operand(ctx, OP_MUL, mul_label_val);
					if (!op_mul) return;


					TACInstruction* offset_tac = create_tac(ctx, TAC_MUL, op_mul, index_tac->result, op_array_val, NULL);
					add_tac_to_table(ctx, offset_tac);

					OperandValue add_val = { .label_name = generate_label(ctx, VIRTUAL) };
					Operand* op_add = create_operand(ctx, OP_ADD, add_val);

					TACInstruction* address_tac = create_tac(ctx, TAC_ADD, op_add, array_tac->result, offset_tac->result, NULL);
					add_tac_to_table(ctx, address_tac);

					if (address_tac) {
						TACInstruction* tac_dereference = create_tac(
							ctx,
							TAC_DEREFERENCE_AND_ASSIGN,
							address_tac ? address_tac->result : NULL,
							right ? right->result : NULL,
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
				TACInstruction* left = build_tac_from_expression_dag(ctx, node->left);
				TACInstruction* right = build_tac_from_expression_dag(ctx, node->right);

				TACInstruction* tac_assignment = create_tac(
					ctx, 
					TAC_ASSIGNMENT, 
					left ? left->result : NULL, 
					NULL, 
					right ? right->result : NULL, 
					NULL
				);

				add_tac_to_table(ctx, tac_assignment);
			}
			printf("\033[32mLeaving NODE_ASSIGNMENT\033[0m\n");
			break;
		}

		case NODE_CALL: {
			printf("\033[32mIn NODE_CALL\033[0m\n");
			Node* arg = node->right;
			while (arg) {
				Node* next_arg = arg->next;
				build_tac_from_expression_dag(ctx, arg);
				arg = next_arg;
			}

			TACInstruction* function_name = build_tac_from_expression_dag(ctx, node->left);
			if (!function_name) return NULL;

			TACInstruction* tac_call = create_tac(ctx, TAC_CALL, function_name->result, NULL, NULL, NULL);
			add_tac_to_table(ctx, tac_call);

			OperandValue func_return_val = {
				.label_name = generate_label(ctx, VIRTUAL)
			};
			Operand* function_label_operand = create_operand(ctx, OP_STORE, func_return_val);

			OperandValue _return_label = {
				.label_name = "_RET"
			};
			Operand* _return_operand = create_operand(ctx, OP_RETURN, _return_label);

			TACInstruction* tac_function_return = create_tac(ctx, TAC_ASSIGNMENT, function_label_operand, NULL, _return_operand, NULL);
			add_tac_to_table(ctx, tac_function_return);
			printf("\033[32mLeaving NODE_CALL\033[0m\n");
			break;
		}

		// case NODE_IF: {	
		// 	printf("\033[32mIn NODE_IF\033[0m\n");
		// 	bool has_next_statement = node->next != NULL;
		// 	bool has_next_conditional = determine_if_next_conditional(node, has_next_statement);
			 						
		// 	char* if_false_label = NULL;
		// 	char* next_jmp_label = NULL;
		// 	char* end_label = NULL;

		// 	TACContext* retrieved_context = peek_tac_context();
		// 	if (retrieved_context) {
		// 		set_end_label_based_on_context(retrieved_context, &end_label);				
		// 	} else {
		// 		end_label = generate_label(ctx, REG_LABEL);
		// 	}

		// 	if (has_next_conditional) {
		// 		if_false_label = generate_label(ctx, REG_LABEL);
		// 	}

		// 	if (has_next_statement && !has_next_conditional) {
		// 		next_jmp_label = generate_label(ctx, REG_LABEL);
		// 	}

		// 	TACContext* context_if = create_tac_context(
		// 		ctx,
		// 		CONTEXT_IF,
		// 		if_false_label,
		// 		end_label,
		// 		NULL,
		// 		retrieved_context ? false : true
		// 	);
		// 	push_tac_context(ctx, context_if);

		// 	TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
		// 	if (!condition_tac) return;

		// 	char* jump_target = NULL;
		// 	if (has_next_conditional) {
		// 		jump_target = if_false_label;
		// 	} else if (has_next_statement) {
		// 		jump_target = next_jmp_label;
		// 	} else {
		// 		jump_target = end_label;
		// 	}

		// 	emit_if_false(ctx, condition_tac->result, jump_target);

		// 	if (node->right) {
		// 		build_tac_from_statement_dag(ctx, node->right);
		// 	}
				
		// 	if (has_next_conditional) {
		// 		emit_goto(ctx, end_label);
		// 	} else if (has_next_statement) {
		// 		emit_goto(ctx, next_jmp_label);
		// 		emit_label(ctx, next_jmp_label);
		// 	}

		// 	if (!has_next_statement && context_if->root_chain_mem) {
		// 		emit_label(ctx, end_label);
		// 	}	

		// 	if (!has_next_conditional) {
		// 		pop_tac_context(ctx);
		// 	}
		// 	printf("\033[32mLeaving NODE_IF\033[0m\n");
		// 	break;
		// }

		// case NODE_ELSE_IF: {
		// 	printf("\033[32mIn NODE_ELSE_IF\033[0m\n");
		// 	TACContext* retrieved_context = peek_tac_context();
		// 	if (!retrieved_context) return;

		// 	bool has_next_statement = node->next != NULL;
		// 	bool has_next_conditional = determine_if_next_conditional(node, has_next_statement);

		// 	char* if_false_label = NULL;
		// 	char* next_jmp_label = NULL;
		// 	char* end_label = retrieved_context->end_label;

		// 	if (has_next_conditional) {
		// 		if_false_label = generate_label(ctx, REG_LABEL);
		// 	}

		// 	if (has_next_statement && !has_next_conditional) {
		// 		next_jmp_label = generate_label(ctx, REG_LABEL);
		// 	}

		// 	pop_tac_context(ctx);
		// 	TACContext* context_else_if = create_tac_context(
		// 		ctx,
		// 		CONTEXT_ELSE_IF,
		// 		if_false_label,
		// 		end_label,
		// 		NULL,
		// 		retrieved_context->root_chain_mem
		// 	);
		// 	push_tac_context(ctx, context_else_if);

		// 	emit_label(ctx, retrieved_context->next_label);			
			
		// 	TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
		// 	if (!condition_tac) return;

		// 	char* jump_target = NULL;
		// 	if (has_next_conditional) {
		// 		jump_target = if_false_label;
		// 	} else if (has_next_statement) {
		// 		jump_target = next_jmp_label;
		// 	} else {
		// 		jump_target = end_label;
		// 	}

		// 	emit_if_false(ctx, condition_tac->result, jump_target);

		// 	if (node->right) {
		// 		build_tac_from_statement_dag(ctx, node->right);
		// 	}

		// 	if (has_next_conditional) {
		// 		emit_label(ctx, end_label);
		// 	} else if (has_next_statement) {
		// 		emit_goto(ctx, next_jmp_label);
		// 		emit_label(ctx, next_jmp_label);
		// 	}

		// 	if (!has_next_statement && context_else_if->root_chain_mem) {
		// 		emit_label(ctx, end_label);
		// 	}

		// 	if (!has_next_conditional) {
		// 		pop_tac_context(ctx);
		// 	}
		// 	printf("\033[32mLeaving NODE_ELSE_IF\033[0m\n");
		// 	break;
		// }

		// case NODE_ELSE: {
		// 	printf("\033[32mIn NODE_ELSE\033[0m\n");
		// 	TACContext* retrieved_context = peek_tac_context();
		// 	if (!retrieved_context) return;

		// 	bool has_next_statement = node->next != NULL;
		//     char* next_jmp_label = NULL;
		//     if (has_next_statement) {
		//         next_jmp_label = generate_label(ctx, REG_LABEL);
		//     }

		// 	pop_tac_context(ctx);
		// 	TACContext* context_else = create_tac_context(
		// 		ctx,
		// 		CONTEXT_ELSE,
		// 		NULL,
		// 		retrieved_context->end_label,
		// 		NULL,
		// 		retrieved_context->root_chain_mem
		// 	);
		// 	push_tac_context(ctx, context_else);

		// 	emit_label(ctx, retrieved_context->next_label);

		// 	if (node->right) {
		// 		build_tac_from_statement_dag(ctx, node->right);
		// 	}

		// 	if (has_next_statement) {
		//         emit_goto(ctx, next_jmp_label);
		//         emit_label(ctx, next_jmp_label);
		//     }

		// 	if (context_else->root_chain_mem) {
		// 		emit_label(ctx, retrieved_context->end_label);
		// 	}

		// 	pop_tac_context(ctx);
		// 	printf("\033[32mLeaving NODE_ELSE\033[0m\n");
		// 	break;
		// }

		// Fixed NODE_IF case
		case NODE_IF: {
		    printf("\033[32mIn NODE_IF\033[0m\n");
		    bool has_next_statement = node->next != NULL;
		    bool has_next_conditional = determine_if_next_conditional(node, has_next_statement);
		    
		    char* if_false_label = NULL;
		    char* next_jmp_label = NULL;
		    char* end_label = generate_label(ctx, REG_LABEL);

		    // TACContext* retrieved_context = peek_tac_context();

		    
		    // Generate labels for control flow
		    if (has_next_conditional) {
		        if_false_label = generate_label(ctx, REG_LABEL);
		    }
		    if (has_next_statement && !has_next_conditional) {
		        next_jmp_label = generate_label(ctx, REG_LABEL);
		    }

		    TACContext* context_if = create_tac_context(
		    	ctx, 
		    	CONTEXT_IF,
		    	if_false_label,
		    	end_label,
		    	NULL,
		    	true
		    );
		    
		    push_tac_context(ctx, context_if);

		    // Generate condition
		    TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
		    if (!condition_tac) return;

		    // Determine jump target for false condition
		    char* jump_target = NULL;
		    if (has_next_conditional) {
		        jump_target = if_false_label;
		    } else if (has_next_statement && !has_next_conditional) {
		        jump_target = next_jmp_label;
		    } else {
		        jump_target = end_label;
		    }

		    emit_if_false(ctx, condition_tac->result, jump_target);

		    // Generate body
		    if (node->right) {
		        build_tac_from_statement_dag(ctx, node->right);
		    }

		    // Handle control flow after body
		    if (has_next_conditional) {
		        emit_goto(ctx, end_label);
		    } else if (has_next_statement) {
		        emit_goto(ctx, next_jmp_label);
		        emit_label(ctx, next_jmp_label);
		        pop_tac_context(ctx);
		    } else {
		    	emit_label(ctx, next_jmp_label);
		    	pop_tac_context(ctx);
		    }

		    // Only emit end label if this is the root of the chain and no next statement
		    // if (context_if->root_chain_mem && !has_next_conditional) {
		    //     emit_label(ctx, end_label);
		    //     pop_tac_context(ctx);
		    // }

		    // Don't pop context if there's a next conditional (else/else-if)
		    // if (!has_next_conditional) {
		    	// emit_label(ctx, end_label);
		    // }
		    printf("\033[32mLeaving NODE_IF\033[0m\n");
		    break;
		}

		// Fixed NODE_ELSE_IF case
		case NODE_ELSE_IF: {
		    printf("\033[32mIn NODE_ELSE_IF\033[0m\n");
		    TACContext* retrieved_context = peek_tac_context();
		    if (!retrieved_context) return;

		    bool has_next_statement = node->next != NULL;
		    bool has_next_conditional = determine_if_next_conditional(node, has_next_statement);

		    char* if_false_label = NULL;
		    char* next_jmp_label = NULL;
		    char* end_label = retrieved_context->end_label;

		    // Generate new labels as needed
		    if (has_next_conditional) {
		        if_false_label = generate_label(ctx, REG_LABEL);
		    }
		    if (has_next_statement && !has_next_conditional) {
		        next_jmp_label = generate_label(ctx, REG_LABEL);
		    }

		    // Pop the previous context and create new one
		    pop_tac_context(ctx);
		    TACContext* context_else_if = create_tac_context(
		        ctx,
		        CONTEXT_ELSE_IF,
		        if_false_label,
		        end_label,
		        NULL,
		        retrieved_context->root_chain_mem
		    );
		    push_tac_context(ctx, context_else_if);

		    // Emit the label for this else-if block
		    emit_label(ctx, retrieved_context->next_label);

		    // Generate condition
		    TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
		    if (!condition_tac) return;

		    // Determine jump target for false condition
		    char* jump_target = NULL;
		    if (has_next_conditional) {
		        jump_target = if_false_label;
		    } else if (has_next_statement) {
		        jump_target = next_jmp_label;
		    } else {
		        jump_target = end_label;
		    }

		    emit_if_false(ctx, condition_tac->result, jump_target);

		    // Generate body
		    if (node->right) {
		        build_tac_from_statement_dag(ctx, node->right);
		    }

		    // Handle control flow after body
		    if (has_next_conditional) {
		        emit_goto(ctx, end_label);
		    } else if (has_next_statement) {
		        emit_goto(ctx, next_jmp_label);
		        emit_label(ctx, next_jmp_label);
		    }

		    // Only emit end label if this is the end of the chain and no next statement
		    if (!has_next_statement && context_else_if->root_chain_mem) {
		        emit_label(ctx, end_label);
		    }

		    // Don't pop context if there's a next conditional
		    if (!has_next_conditional) {
		        pop_tac_context(ctx);
		    }
		    printf("\033[32mLeaving NODE_ELSE_IF\033[0m\n");
		    break;
		}

		// Fixed NODE_ELSE case
		case NODE_ELSE: {
		    printf("\033[32mIn NODE_ELSE\033[0m\n");
		    TACContext* retrieved_context = peek_tac_context();
		    if (!retrieved_context) return;

		    bool has_next_statement = node->next != NULL;
		    char* next_jmp_label = NULL;
		    if (has_next_statement) {
		        next_jmp_label = generate_label(ctx, REG_LABEL);
		    }

		    // Pop the previous context and create new one
		    pop_tac_context(ctx);
		    TACContext* context_else = create_tac_context(
		        ctx,
		        CONTEXT_ELSE,
		        NULL,
		        retrieved_context->end_label,
		        NULL,
		        retrieved_context->root_chain_mem
		    );
		    push_tac_context(ctx, context_else);

		    // Emit the label for this else block
		    emit_label(ctx, retrieved_context->next_label);

		    // Generate body
		    if (node->right) {
		        build_tac_from_statement_dag(ctx, node->right);
		    }

		    // Handle control flow after body
		    if (has_next_statement) {
		        emit_goto(ctx, next_jmp_label);
		        emit_label(ctx, next_jmp_label);
		    }

		    // Always emit end label for else (end of chain)
		    if (context_else->root_chain_mem) {
		        emit_label(ctx, retrieved_context->end_label);
		    }

		    pop_tac_context(ctx);
		    printf("\033[32mLeaving NODE_ELSE\033[0m\n");
		    break;
		}

		case NODE_WHILE: {
			printf("\033[32mIn NODE_WHILE\033[0m\n");
			char* loop_start_label = generate_label(ctx, REG_LABEL);
			char* end_label = generate_label(ctx, REG_LABEL);

			TACContext* context_while = create_tac_context(
				ctx, 
				CONTEXT_LOOP, 
				NULL, 
				end_label, 
				loop_start_label,
				true
			);
			push_tac_context(ctx, context_while);

			emit_label(ctx, loop_start_label);

			TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, node->left);
			if (!condition_tac) return;

			emit_if_false(ctx, condition_tac->result, end_label);				

			if (node->right) {
				build_tac_from_statement_dag(ctx, node->right);
			}

			emit_goto(ctx, loop_start_label);
			emit_label(ctx, end_label);

			pop_tac_context(ctx);
			printf("\033[32mLeaving NODE_WHILE\033[0m\n");
			break;
		}

		case NODE_FOR: {
			printf("\033[32mIn NODE_FOR\033[0m\n");
			char* loop_start_label = generate_label(ctx, REG_LABEL);
			char* update_label = generate_label(ctx, REG_LABEL);
			char* end_label = generate_label(ctx, REG_LABEL);

	
			TACContext* context = create_tac_context(
				ctx, 
				CONTEXT_LOOP, 
				NULL, 
				end_label,
				update_label,
				true
			);
			push_tac_context(ctx, context);

			Node* initializer_node = node->left ? node->left : NULL;
			build_tac_from_statement_dag(ctx, initializer_node);

			emit_label(ctx, loop_start_label);

			Node* condition_node = initializer_node ? initializer_node->next : NULL;
			TACInstruction* condition_tac = build_tac_from_expression_dag(ctx, condition_node);
			if (!condition_tac) return;

			emit_if_false(ctx, condition_tac->result, end_label);

			if (node->right) {
				build_tac_from_statement_dag(ctx, node->right); 
			}
				
			emit_label(ctx, update_label);
			Node* update_node = condition_node ? condition_node->next : NULL;
			build_tac_from_statement_dag(ctx, update_node);
			emit_goto(ctx, loop_start_label);
			emit_label(ctx, end_label);

			pop_tac_context(ctx);
			printf("\033[32mLeaving NODE_FOR\033[0m\n");
			break;
		}

		case NODE_RETURN: {
			printf("\033[32mIn NODE_RETURN\033[0m\n");
			TACInstruction* result = NULL;
			if (node->right) {
				TACInstruction* tac = build_tac_from_expression_dag(ctx, node->right);
				result = create_tac(
					ctx,
					TAC_RETURN,
					NULL,
					tac ? tac->result : NULL,
					NULL,
					NULL
				);
			} else {
				result = create_tac(ctx, TAC_RETURN, NULL, NULL, NULL, NULL);
			}
			add_tac_to_table(ctx, result);
			printf("\033[32mLeaving NODE_RETURN\033[0m\n");
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			printf("\033[32mIn NODE_INCREMENT/DECREMENT\033[0m\n");
			Node* int_node = create_int_node(ctx, NODE_INTEGER, 1, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!int_node) return;
			printf("HEEEEEELO\n");
			node_t kind = (node->type == NODE_INCREMENT) ? NODE_ADD : NODE_SUB;
			printf("Node kind is %d\n", kind);
			Node* op_node = create_node(ctx, kind, node->left, int_node, NULL, NULL, NULL, NULL);	
			if (op_node) {
				printf("op node address: %p\n", (void*)op_node);
				printf("Node has type %d\n", op_node->type);
			} else {
				printf("We dont have op node\n");
			}
			printf("or heeeeeeelo\n");

			TACInstruction* tac_arithmetic = build_tac_from_expression_dag(ctx, op_node);
			printf("no we're ee\n");
			TACInstruction* tac_var = build_tac_from_expression_dag(ctx, node->left);
			printf("here\n");
			if (!tac_arithmetic || !tac_var) return;
			TACInstruction* tac_assignment = create_tac(
				ctx, 
				TAC_ASSIGNMENT, 
				tac_var ? tac_var->result : NULL, 
				NULL, 
				tac_arithmetic ? tac_arithmetic->result : NULL, 
				NULL
			);
			printf("now were here\n");
			add_tac_to_table(ctx, tac_assignment);
			printf("\033[32mLeaving NODE_INCREMENT/DECREMENT\033[0m\n");
			break;
		}

		case NODE_BREAK: {
			printf("\033[32mIn NODE_BREAK\033[0m\n");
			TACContext* retrieved_context = peek_tac_context();

			TACContext* loop_context = context_loop_lookup(retrieved_context);
			if (loop_context) {
				emit_goto(ctx, loop_context->end_label);
			}
			break;
		}

		case NODE_CONTINUE: {
			printf("\033[32mIn NODE_CONTINUE\033[0m\n");
			TACContext* retrieved_context = peek_tac_context();

			TACContext* loop_context = context_loop_lookup(retrieved_context);
			if (loop_context) {
				emit_goto(ctx, loop_context->update_label);
			}
			printf("\033[32mLeaving NODE_CONTINUE\033[0m\n");
			break;
		}

		case NODE_BLOCK: {
			printf("\033[32mIn NODE_BLOCK\033[0m\n");
			Node* stmt = node->right;
			while (stmt) {
				Node* next_stmt = stmt->next;
				build_tac_from_statement_dag(ctx, stmt);
				stmt = next_stmt;
			}
			printf("\033[32mLeaving NODE_BLOCK\033[0m\n");
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

		OperandValue parameter_val = { .label_name = generate_label(ctx, PARAM_LABEL) };
		Operand* parameter_op = create_operand(ctx, OP_LABEL, parameter_val);

		TACInstruction* param_instruction = build_tac_from_expression_dag(ctx, current_wrapped_param->right);
		if (param_instruction) {
			result = create_tac(ctx, TAC_PARAM, parameter_op, param_instruction->result, NULL, NULL);
			add_tac_to_table(ctx, result);
		}

		current_wrapped_param = next_wrapped_param;
	}
}

void reset_tac_indices() {
	tac_instruction_index = 0;
	tac_parameter_index = 0;
	tac_function_argument_index = 0;
}

void build_tac_from_global_dag(CompilerContext* ctx, Node* node) {
	if (!node) return;

	printf("we have node\n");
	switch (node->type) {
		case NODE_NAME: {
			printf("in node name\n");
			Symbol* sym = node->symbol;
			if (sym) {
				printf("sym\n");
				if (sym->type) {
					printf("type\n");
					Type* t = sym->type;
					if (t->kind == TYPE_FUNCTION) {
						printf("func\n");
						reset_tac_indices();
						reset_context_stack();
						printf("\033[31mBUILDING TACS FOR FUNCTION '%s'\033[0m\n", node->value.name);
						
						Operand* func_op = NULL;
						if (node->symbol) {
							OperandValue func_val = {.sym = node->symbol};
							func_op = create_operand(ctx, OP_SYMBOL, func_val);
						}
						printf("got here\n");
						if (func_op) {
							TACInstruction* tac_function = create_tac(ctx, TAC_NAME, func_op, NULL, NULL, NULL);
							add_tac_to_table(ctx, tac_function);
							
							build_tac_from_parameter_dag(ctx, node->params);
							build_tac_from_statement_dag(ctx, node->right);
						}
						printf("and here\n");
						printf("\033[31mFINISHED BUILDING TACS FOR FUNCTION '%s'\033[0m\n", node->value.name);
					}
				} 
			} 
			break;
		}
	}
}

TACTable* build_tacs(CompilerContext* ctx, Node* node) {
	if (!node) {
		printf("Root node is NULL\n");
		return NULL;
	}
	if (!init_tac_table(ctx)) return NULL;

	if (!init_tac_context_stack(ctx)) return NULL;

	Node* current = node;
	while (current) {
		Node* next = current->next;
		printf("here\n");
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
	printf("Number of tacs is %d\n", tac_table->size);
	for (int i = 0; i < tac_table->capacity; i++) {
		TACInstruction* current = tac_table->tacs[i];
		if (current) {
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
					if (current->result) {
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
					}

					if (current->op2) {
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

					}
					break;
				}

				case TAC_IF_FALSE: {
					printf("\tIF_FALSE");
					if (current->op1) {
						switch (current->op1->kind) {
							case OP_SYMBOL: {
								printf(" %s GOTO", current->op1->value.sym->name ? current->op1->value.sym->name : NULL);
								break;
							} 

							default: {
								printf(" %s GOTO", current->op1->value.label_name ? current->op1->value.label_name : NULL);
								break;
							}
						}
					}

					if (current->op2) {
						printf(" %s\n", current->op2->value.label_name ? current->op2->value.label_name : NULL);
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
					if (current->op1) {
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

					}

					printf("%s ", get_tac_op(current));
					if (current->op2) {
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

					}
					
					break;
				}

				case TAC_LABEL: {
					if (current->result && current->result->value.label_name) {
						printf("%s\n", current->result->value.label_name);
					}
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
		} else {
			// printf("\033[31mError\033[0m: tac at index %d is NULL\n", i);
		}
	}
} 