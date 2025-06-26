#include "tac.h"

static int label_counter = 0;
static int tac_variable_index = 0;
static int tac_parameter_index = 0;
static int tac_function_argument_index = 0;
TACTable* tac_table = NULL;
TACContextStack tac_context_stack;

void init_tac_table() {
	tac_table = create_tac_table();
	if (!tac_table) {
		printf("In 'init_tac_table', received NULL tac table.\n");
		exit(EXIT_FAILURE);
	}
	printf("Created tac table\n");
}

TACTable* create_tac_table() {
	TACTable* table = malloc(sizeof(TACTable));
	if (!table) {
		perror("In 'create_tac_table', unable to allocate space for tac table.\n");
		return NULL;
	} 

	table->size = 0;
	table->tac_index = 0;
	table->capacity = INITIAL_TABLE_CAPACITY;
	table->tacs = calloc(table->capacity, sizeof(TACInstruction*));
	if (!table->tacs) {
		perror("In 'create_tac_table', unable to allocate space and initialize table->tacs.\n");
		free(table);
		return NULL;
	}
	printf("About to return tac table\n");
	return table;
}

TACInstruction* create_tac(tac_t type, char* name, char* op1, char* op2, char* op3, Symbol* symbol) {
	TACInstruction* tac = malloc(sizeof(TACInstruction));
	if (!tac) {
		perror("In 'create_TAC', unable to allocate space for tac.\n");
		return NULL;
	}

	tac->type = type;
	tac->freed = false;
	tac->name = NULL;
	tac->op1 = NULL;
	tac->op2 = NULL;
	tac->op3 = NULL;
	tac->symbol = NULL;
	
	if (name) {
		tac->name = strdup(name);
		if (!tac->name) {
			printf("In 'create_tac', unable to duplicate '%s'.\n", name);
			free(tac);
			return NULL;
		}

	}

	if (op1) {
		tac->op1 = strdup(op1);
		if (!tac->op1) {
			perror("In 'create_TAC', unable to duplicate 'op1'.\n");
			if (tac->name) free(tac->name);
			free(tac);
			return NULL;
		}
	}

	if (op2) {
		tac->op2 = strdup(op2);
		if (!tac->op2) {
			perror("In 'create_TAC', unable to duplicate 'op2'.\n");
			if (tac->op1) free(tac->op1);
			if (tac->name) free(tac->name);
			free(tac);
			return NULL;
		}

	}

	if (op3) {
		tac->op3 = strdup(op3);
		if (!tac->op3) {
			perror("In 'create_TAC', unable to duplicate 'op3'.\n");
			if (tac->op2) free(tac->op2);
			if (tac->op1) free(tac->op1);
			if (tac->name) free(name);
			free(tac);
			return NULL;
		}

	}

	if (symbol) {
		tac->symbol = symbol_copy(symbol);
		if (!tac->symbol) {
			printf("In 'create_tac', unable to copy symbol with name '%s'.\n", symbol->name ? symbol->name : "N/A");
			if (tac->op3) free(tac->op3);
			if (tac->op2) free(tac->op2);
			if (tac->op1) free(tac->op1);
			if (tac->name) free(tac->name);
			free(tac);
			return NULL;
		}
	}
	return tac;
}

void init_tac_context_stack() {
	tac_context_stack = create_tac_context_stack();
	if (!tac_context_stack.contexts) {
		free_tac_table();
		exit(EXIT_FAILURE);
	}
}

TACContextStack create_tac_context_stack() {
	TACContext** contexts = calloc(INITIAL_TACCONTEXT_CAPACITY, sizeof(TACContext*));
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

void push_tac_context(TACContext* context) {
	if (!context) return;

	if (tac_context_stack.size >= tac_context_stack.capacity) {
		tac_context_stack.capacity *= 2;
		tac_context_stack.contexts = realloc(tac_context_stack.contexts, sizeof(TACContext*) * tac_context_stack.capacity);
		if (!tac_context_stack.contexts) {
			perror("In 'push_tac_context', unable to realloc TAC contexts.\n");
			return;
		}
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
		TACContext* current_context = tac_context_stack.contexts[tac_context_stack.top];
		free_tac_context(current_context);
		tac_context_stack.top--;
		tac_context_stack.size--;
	}
	printf("TAC context stack is empty, unable to pop\n");
}

TACContext* tac_context_lookup(tac_t target_type) {
	for (int i = tac_context_stack.top; i >= 0; i--) {
		TACContext* current = tac_context_stack.contexts[i];
		if (current->type == target_type) {
			return current;
		}
	}
	return NULL;
}

TACContext* create_tac_context(tac_t type, char* next_label, char* end_label, char* update_label) {
	TACContext* context = malloc(sizeof(TACContext));
	if (!context) {
		perror("In 'create_context_stack', unable to allocate space for tac context.\n");
		return NULL;
	}
	context->type = type;
	context->end_label = NULL;
	context->update_label = NULL;
	context->next_label = NULL;

	if (next_label) {
		context->next_label = strdup(next_label);
		if (!context->next_label) {
			printf("In 'create_tac_context', unable to duplicate label '%s'\n", next_label);
			free(context);
			return NULL;
		}
	}

	if (end_label) {
		context->end_label = strdup(end_label);
		if (!context->end_label) {
			printf("In 'create_context_stack', unable to duplicate label '%s'\n", end_label);
			if (context->next_label) free(context->next_label);
			free(context);
			return NULL;
		}
	}

	if (update_label) {
		context->update_label = strdup(update_label);
		if (!context->update_label) {
			printf("In 'create_context_stack', unable to duplicate update label '%s'.\n", update_label);
			if (context->end_label) free(context->end_label);
			if (context->next_label) free(context->next_label);
			free(context);
			return NULL;
		}
	}
	return context;
}

char* generate_label() {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), ".L%d", label_counter++);
	return strdup(buffer);
}

char* tac_variable_create() {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "t%d", tac_variable_index++);
	return strdup(buffer);
}

char* tac_parameter_label() {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "p%d", tac_parameter_index++);
	return strdup(buffer);
}

char* tac_function_argument_label() {
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "a%d", tac_function_argument_index++);
	return strdup(buffer);
}

char* tac_function_name(char* function_name) {
	char buffer[60];
	snprintf(buffer, sizeof(buffer), "_%s:", function_name);
	return strdup(buffer);
}

void add_tac_to_table(TACInstruction* tac) {
	if (!tac) return;

	if (tac_table->size >= tac_table->capacity) {
		tac_table->capacity *= 2;
		tac_table->tacs = realloc(tac_table->tacs, sizeof(TACInstruction*) * tac_table->capacity);
		if (!tac_table->tacs) {
			perror("In 'add_tac_to_table', unable to reallocate tac_table->tacs.\n");
			return;
		}
	}
	tac_table->tacs[tac_table->tac_index++] = tac;
	tac_table->size++;
}

tac_t get_tac_type(dagnode_t type) {
	switch (type) {
		case DAG_ADD: return TAC_ADD;
		case DAG_SUB: return TAC_SUB;
		case DAG_MUL: return TAC_MUL;
		case DAG_DIV: return TAC_DIV;
		case DAG_ADD_EQUAL: return TAC_ADD_EQUAL;
		case DAG_SUB_EQUAL: return TAC_SUB_EQUAL;
		case DAG_MUL_EQUAL: return TAC_MUL_EQUAL;
		case DAG_DIV_EQUAL: return TAC_DIV_EQUAL;
		case DAG_MODULO: return TAC_MODULO;
		case DAG_LOGICAL_AND: return TAC_LOGICAL_AND;
		case DAG_LOGICAL_OR: return TAC_LOGICAL_OR;
		case DAG_LESS: return TAC_LESS;
		case DAG_GREATER: return TAC_GREATER;
		case DAG_LESS_EQUAL: return TAC_LESS_EQUAL;
		case DAG_GREATER_EQUAL: return TAC_GREATER_EQUAL;
		case DAG_EQUAL: return TAC_EQUAL;
		case DAG_NOT_EQUAL: return TAC_NOT_EQUAL;
		case DAG_INCREMENT: return TAC_INCREMENT;
		case DAG_DECREMENT: return TAC_DECREMENT;
		case DAG_INTEGER: return TAC_INTEGER;
		case DAG_BOOL: return TAC_BOOL;
		case DAG_CHAR: return TAC_CHAR;
	}
}

void build_tac_from_parameter_dag(DAGNode* wrapped_param) {
	if (!wrapped_param) return;
	TACInstruction* result = NULL;
	DAGNode* current_wrapped_param = wrapped_param;
	while (current_wrapped_param) {
		DAGNode* actual_param_node = current_wrapped_param->right;
		char* parameter_label = tac_parameter_label();
		char* actual_param_node_name = actual_param_node->value.name;		
		result = create_tac(TAC_PARAM, parameter_label, actual_param_node_name, NULL, NULL, actual_param_node->symbol);
		add_tac_to_table(result);
		current_wrapped_param = current_wrapped_param->next;
	}
}

TACInstruction* build_tac_from_expression_dag(DAGNode* node) {
	if (!node) return;
	TACInstruction* result = NULL;
	printf("In 'build_tac_from_expression_dag'.\n");
	switch (node->kind) {
		case DAG_ADD:
		case DAG_SUB:
		case DAG_MUL:
		case DAG_DIV:
		case DAG_ADD_EQUAL:
		case DAG_SUB_EQUAL:
		case DAG_MUL_EQUAL:
		case DAG_DIV_EQUAL:
		case DAG_MODULO:
		case DAG_LESS:
		case DAG_GREATER:
		case DAG_LESS_EQUAL:
		case DAG_GREATER_EQUAL:
		case DAG_EQUAL:
		case DAG_NOT_EQUAL: {		
			tac_t type = get_tac_type(node->kind);
			char* tac_variable = tac_variable_create();
			build_tac_from_expression_dag(node->left);
			build_tac_from_expression_dag(node->right);

			char* op1_str = node->left->name;
			char* op2_str = node->right->name;
			result = create_tac(type, tac_variable, op1_str, op2_str, NULL, node->symbol);
			add_tac_to_table(result);
			node->name = tac_variable;
			break;
		}

		case DAG_CHAR:
		case DAG_BOOL:
		case DAG_INTEGER: {
			tac_t type = get_tac_type(node->kind);
			char* tac_variable = tac_variable_create();

			char buffer[32];
			snprintf(buffer, sizeof(buffer), "%d", node->value.val);
			result = create_tac(type, tac_variable, buffer, NULL, NULL, NULL);
			add_tac_to_table(result);
			node->name = tac_variable;
			break;
		}

		case DAG_INCREMENT:
		case DAG_DECREMENT: {
			tac_t type = get_tac_type(node->kind);
			char* tac_variable = tac_variable_create();

			build_tac_from_expression_dag(node->left);
			char* op1_str = node->left->name;
			result = create_tac(type, tac_variable, op1_str, NULL, NULL, node->symbol);
			add_tac_to_table(result);
			node->name = tac_variable;
			break;
		}

		case DAG_NAME: {
			node->name = node->value.name;	
			break;
		}

		case DAG_DECL:
		case DAG_DEF:
		case DAG_AUG: {
			build_tac_from_expression_dag(node->left);
			node->name = strdup(node->left->name);
			break;
		}

		case DAG_ARG: {
			char* arg_label = tac_function_argument_label();
			char* actual_arg_name = node->left->name ? node->left->name : NULL;
			Symbol* actual_arg_symbol = node->left->symbol ? node->left->symbol : NULL;
			TACInstruction* arg_tac = create_tac(TAC_ARG, arg_label, actual_arg_name, NULL, NULL, actual_arg_symbol);
			add_tac_to_table(arg_tac);
			node->name = arg_label;
			break;
		}

		case DAG_CALL: {
			TACInstruction* tac_call = create_tac(TAC_CALL, node->value.name, NULL, NULL, NULL, node->symbol);
			add_tac_to_table(tac_call);

			DAGNode* arg = node->right;
			while (arg) {
				DAGNode* next_arg = arg->next;
				build_tac_from_expression_dag(arg);
				arg = next_arg;
			}
			break;
		}

		// case DAG_SUBSCRIPT: {
		// 	size_t element_size = sizeof(node->symbol->type->subtype);
		// 	DAGNode* index_dagnode = create_int_dag_node(DAG_INTEGER, element_size, NULL, NULL, NULL, NULL, NULL, NULL);
		// 	DAGNode* scaled_index_dagnode = create_dag_node(DAG_MUL, node->right, element_size, NULL, NULL, NULL, NULL);
		// 	TACInstruction* scaled_index_tac = build_tac_from_expression_dag(scaled_index_dagnode);
			
			
		// 	TACInstruction* address_tac = build_tac_from_expression_dag()

		// 	break;
		// }
	}
	return result;
}

void build_tac_from_statement_dag(DAGNode* node) {
	if (!node) return;

	switch (node->kind) {
		case DAG_ASSIGNMENT: {
			build_tac_from_expression_dag(node->left);
			build_tac_from_expression_dag(node->right);

			char* op2_str = node->right->name;
			TACInstruction* tac_assignment = create_tac(TAC_ASSIGNMENT, node->left->value.name, NULL, op2_str, NULL, node->left->symbol);
			add_tac_to_table(tac_assignment);
			break;
		}

		case DAG_IF: {			
			char* if_true_label = generate_label();
			char* if_false_label = generate_label();
			char* end_label = generate_label();

			TACContext* context_if = create_tac_context(TAC_IF, if_false_label, end_label, NULL);
			push_tac_context(context_if);

			TACInstruction* condition_tac = build_tac_from_expression_dag(node->left);
			TACInstruction* condition_false_tac = create_tac(TAC_IF_FALSE, condition_tac->name, if_false_label, NULL, NULL, NULL);
			add_tac_to_table(condition_false_tac);

			TACInstruction* if_true_tac = create_tac(TAC_LABEL, if_true_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(if_true_tac);
			build_tac_from_statement_dag(node->right);
			TACInstruction* goto_tac = create_tac(TAC_GOTO, end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(goto_tac);

			TACInstruction* end_label_tac = create_tac(TAC_LABEL, end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(end_label_tac);
			pop_tac_context();
			break;
		}

		case DAG_ELSE_IF: {
			TACContext* retrieved_context = peek_tac_context();
			TACInstruction* else_if_tac = create_tac(TAC_LABEL, retrieved_context->next_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(else_if_tac);

			char* else_if_condition_true_label = generate_label();
			char* else_if_condition_false_label = generate_label();

			TACContext* context_else_if = create_tac_context(TAC_ELSE_IF, else_if_condition_false_label, retrieved_context->end_label, NULL);
			push_tac_context(context_else_if);

			TACInstruction* else_if_condition_tac = build_tac_from_expression_dag(node->left);
			TACInstruction* else_if_condition_false_tac = create_tac(TAC_IF_FALSE, else_if_condition_tac->name, else_if_condition_false_label, NULL, NULL, NULL);
			add_tac_to_table(else_if_condition_false_tac);

			TACInstruction* else_if_condition_true_tac = create_tac(TAC_LABEL, else_if_condition_true_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(else_if_condition_true_tac);
			build_tac_from_statement_dag(node->right);
			TACInstruction* goto_tac = create_tac(TAC_GOTO, retrieved_context->end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(goto_tac);
			
			pop_tac_context();
			break;
		}

		case DAG_ELSE: {
			TACContext* retrieved_context = peek_tac_context();
			TACInstruction* else_tac = create_tac(TAC_LABEL, retrieved_context->next_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(else_tac);
			build_tac_from_statement_dag(node->right);
			break;
		}

		case DAG_WHILE: {
			TACInstruction* tac_while_boundary = create_tac(TAC_WHILE, NULL, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_while_boundary);

			char* condition_true_label = generate_label();
			char* update_label = generate_label();
			char* end_label = generate_label();

			TACContext* context_while = create_tac_context(TAC_WHILE, NULL, update_label, end_label);
			push_tac_context(context_while);

			TACInstruction* condition_tac = build_tac_from_expression_dag(node->left);
			TACInstruction* condition_false_tac = create_tac(TAC_IF_FALSE, condition_tac->name, end_label, NULL, NULL, NULL);
			add_tac_to_table(condition_false_tac);

			TACInstruction* condition_true_tac = create_tac(TAC_LABEL, condition_true_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(condition_true_tac);
			build_tac_from_statement_dag(node->right);

			TACInstruction* update_tac = create_tac(TAC_LABEL, update_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(update_tac);

			TACInstruction* goto_tac = create_tac(TAC_GOTO, condition_true_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(goto_tac);

			TACInstruction* end_tac = create_tac(TAC_LABEL, end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(end_tac);

			pop_tac_context();
			break;
		}

		case DAG_FOR: {
			TACInstruction* tac_for_boundary = create_tac(TAC_FOR, NULL, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_for_boundary);

			DAGNode* initializer = node->left;
			build_tac_from_statement_dag(initializer);

			char* loop_start_label = generate_label();
			char* loop_update_label = generate_label();
			char* loop_end_label = generate_label();

			TACContext* context_for = create_tac_context(TAC_FOR, NULL, loop_end_label, loop_update_label);
			push_tac_context(context_for);

			TACInstruction* tac_loop_start_label = create_tac(TAC_LABEL, loop_start_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_loop_start_label);

			DAGNode* condition = initializer->left;
			TACInstruction* condition_tac = build_tac_from_expression_dag(condition);
			TACInstruction* tac_false_label = create_tac(TAC_IF_FALSE, condition_tac->name, loop_end_label, NULL, NULL, NULL);
			add_tac_to_table(tac_false_label);

			build_tac_from_statement_dag(node->right); 

			TACInstruction* tac_update_label = create_tac(TAC_LABEL, loop_update_label, NULL, NULL, NULL, NULL);
			DAGNode* update = condition->left;
			build_tac_from_expression_dag(update);
			TACInstruction* tac_goto = create_tac(TAC_GOTO, loop_start_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_goto);

			TACInstruction* tac_loop_end_label = create_tac(TAC_LABEL, loop_end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_loop_end_label);

			pop_tac_context();
			break;
		}

		case DAG_RETURN: {
			TACInstruction* tac_result = NULL;
			if (!node->right) {
				tac_result = create_tac(TAC_RETURN, NULL, NULL, NULL, NULL, NULL);
			} else {
				TACInstruction* tac_return = build_expression_dag_node(node->right);
				tac_result = create_tac(TAC_RETURN, tac_return->name, NULL, NULL, NULL, node->symbol);
			}
			add_tac_to_table(tac_result);
			break;
		}

		case DAG_INCREMENT:
		case DAG_DECREMENT: {
			tac_t type = get_tac_type(node->kind);
			build_tac_from_expression_dag(node->left);

			DAGNode* left = copy_dag_node(node->left);

			DAGNode* right = create_int_dag_node(DAG_INTEGER, 1, NULL, NULL, NULL, NULL, NULL, NULL);
			

			DAGNode* op_dagnode = create_dag_node(node->kind, left, right, NULL, NULL, NULL, NULL);	
			if (!op_dagnode) {
				free_dag_node(right);
				free_dag_node(left);
				return NULL;
			}

			build_expression_dag_node(op_dagnode);

			char* tac_variable = op_dagnode->name;
			char* op1_str = op_dagnode->left->name;
			char* op2_str = op_dagnode->right->name;
			TACInstruction* tac_increment = create_tac(type, tac_variable, op1_str, op2_str, NULL, NULL);
			add_tac_to_table(tac_increment);

			TACInstruction* tac_assignment = create_tac(TAC_ASSIGNMENT, node->left->name, NULL, tac_variable, NULL, NULL);
			add_tac_to_table(tac_assignment);
			free_dag_node(op_dagnode);
			break;
		}

		case DAG_BREAK: {
			TACContext* context_break = tac_context_lookup(TAC_FOR);
			if (!context_break) return;

			TACInstruction* tac_break = create_tac(TAC_GOTO, context_break->end_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_break);
			break;
		}

		case DAG_CONTINUE: {
			TACContext* context_continue = tac_context_lookup(TAC_FOR);
			if (!context_continue) return;
			TACInstruction* tac_continue = create_tac(TAC_GOTO, context_continue->update_label, NULL, NULL, NULL, NULL);
			add_tac_to_table(tac_continue);
			break;
		}

		case DAG_BLOCK: {
			DAGNode* stmt = node->right;
			while (stmt) {
				build_tac_from_statement_dag(stmt);
				stmt = stmt->next;
			}
			break;
		}
	}
}

void build_tac_from_global_dag(DAGNode* node) {
	if (!node) return;

	switch (node->kind) {
		case NODE_NAME: {
			if (node->type) {
				if (node->type->kind == TYPE_FUNCTION) {
					char* virtual_function_name = tac_function_name(node->value.name);
					TACInstruction* tac_function = create_tac(TAC_NAME, virtual_function_name, NULL, NULL, NULL, node->symbol);
					add_tac_to_table(tac_function);

					build_tac_from_parameter_dag(node->type->params);
					build_tac_from_statement_dag(node->right);

				}
			}
		}
	}
}

TACTable* build_tacs(DAGNode* node) {
	if (!node) return NULL;

	init_tac_table();
	init_tac_context_stack();

	DAGNode* current = node;
	while (current) {
		DAGNode* next = current->next;
		build_tac_from_global_dag(current);
		current = next;
	}
	return tac_table;
}

void free_tac_context(TACContext* context) {
	if (!context) return;
	if (context->next_label) free(context->next_label);
	if (context->end_label) free(context->end_label);
	if (context->update_label) free(context->update_label); 
	free(context);
}

void free_tac_context_stack() {
	for (int i = 0; i < tac_context_stack.capacity; i++) {
		free_tac_context(tac_context_stack.contexts[i]);
	}
	free(tac_context_stack.contexts);
}

void free_tac_instruction(TACInstruction* tac) {
	if (!tac || (tac && tac->freed)) return;	
	
	if (tac->name) {
		free(tac->name);
		tac->name = NULL;
	}

	if (tac->op1) {
		free(tac->op1);
		tac->op1 = NULL;
	}
	
	if (tac->op2) {
		free(tac->op2);
		tac->op2 = NULL;
	}

	if (tac->op3) {
		free(tac->op3);
		tac->op3 = NULL;
	}

	if (tac->symbol) { free_symbol(tac->symbol); }
	tac->freed = true;
	free(tac);
}

void free_tac_table(TACTable* tac_table) {
	if (!tac_table) return;

	free_tac_context_stack();

	for (int i = 0; i < tac_table->capacity; i++) {
		free_tac_instruction(tac_table->tacs[i]);
	}
	free(tac_table->tacs);
	free(tac_table);
}