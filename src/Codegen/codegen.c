// #include "codegen.h"

// asm_writer* writer = NULL;
// label_stack* ls_struct = NULL;
// struct RegisterTable* rt_struct = NULL;
// char* function_regs[6] = {"edi", "esi", "edx", "rcx", "r8", "r9"};
// char* scratch_regs[10] = {"eax", "ebx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
// static int label_counter = 0;

// struct Register* create_register(register_state state, char* name) {
// 	struct Register* reg = malloc(sizeof(struct Register));
// 	if (!reg) {
// 		perror("Unable to allocate space for reg\n");
// 		return NULL;
// 	}

// 	reg->state = state;
// 	reg->name = strdup(name);
// 	if (!reg->name) {
// 		printf("Unable to duplicate '%s' for register\n", name);
// 		free(reg);
// 		return NULL;
// 	}
// 	return reg;
// }

// struct RegisterTable* create_register_table() {
// 	struct RegisterTable* rt_struct = malloc(sizeof(struct RegisterTable));
// 	if (!rt_struct) {
// 		perror("Unable to allocate space for register table\n");
// 		return NULL;
// 	}

// 	rt_struct->capacity = MAX_REGISTERS;
// 	for (int i = 0; i < rt_struct->capacity; i++) {
// 		rt_struct->register_table[i] = create_register(REGISTER_FREE, scratch_regs[i]);
// 		if (!rt_struct->register_table[i]) {
// 			for (int j = 0; j < i; j++) { 
// 				free_register(rt_struct->register_table[j]);
// 			}
// 			return NULL;
// 		}
// 	}

// 	return rt_struct;
// }

// char* scratch_name(struct RegisterTable* rt_struct, int i) {
// 	if (!rt_struct || i < 0 || i > rt_struct->capacity) return NULL;

// 	return rt_struct->register_table[i]->name;
// }

// int scratch_alloc(struct RegisterTable* rt_struct) {
// 	if (!rt_struct) return -1;

// 	for (int i = 0; i < rt_struct->capacity; i++) {
// 		if (rt_struct->register_table[i]->state == REGISTER_FREE) {
// 			rt_struct->register_table[i]->state = REGISTER_USED;
// 			return i;
// 		}
// 	}
// 	printf("Error: no free registers\n");
// 	return -1;
// }

// void scratch_free(struct RegisterTable* rt_struct, int i) {
// 	if (!rt_struct || i < 0 || i > rt_struct->capacity) return;

// 	rt_struct->register_table[i]->state = REGISTER_FREE;
// }

// int label_create() {
// 	return label_counter++;
// }

// char* label_name(int label) {
// 	static char buffer[32];
// 	for (int i = 0; i < ls_struct->capacity; i++) {
// 		if (ls_struct->labels[i] == label) {
// 			snprintf(buffer, sizeof(buffer), ".L%d", label + 1);
// 			return buffer;
// 		}
// 	}
// 	snprintf(buffer, sizeof(buffer), ".L%d", label);
// 	return buffer;
// }

// asm_writer* create_asm_writer(char* output) {
// 	asm_writer* asm_struct = malloc(sizeof(asm_writer));
// 	if (!asm_struct) {
// 		perror("Unable to allocate space for asm_struct.\n");
// 		return NULL;
// 	}

// 	asm_struct->file = NULL;
// 	asm_struct->current_text_pos = 0;
// 	asm_struct->current_data_pos = 0;

// 	if (output) {
// 		asm_struct->file = fopen(output, "w");
// 		if (!asm_struct->file) {
// 			printf("Unable to open file with name '%s'\n", output);
// 			free(asm_struct);
// 			return NULL;
// 		}
// 	}

// 	return asm_struct;
// }

// void init_asm_writer(char* output) {
// 	writer = create_asm_writer(output);
// 	if (!writer) {
// 		printf("Unable to create asm_writer.\n");
// 		exit(EXIT_FAILURE);
// 	}
// }

// label_stack* create_label_stack() {
// 	label_stack* ls_struct = malloc(sizeof(label_stack));
// 	if (!ls_struct) {
// 		perror("Unable to allocate space for label stack.\n");
// 		return NULL;	
// 	}

// 	ls_struct->top = -1;
// 	ls_struct->size = 0;
// 	ls_struct->capacity = LABEL_STACK_CAPACITY;
// 	ls_struct->labels = malloc(sizeof(int) * ls_struct->capacity);
// 	if (!ls_struct->labels) {
// 		perror("Unable to allocate space for labels\n");
// 		free(ls_struct);
// 		return NULL;
// 	}
// 	return ls_struct;
// }

// void push_label(int label) {
// 	if (ls_struct->size >= ls_struct->capacity) {
// 		ls_struct->capacity *= 2;
// 		ls_struct->labels = realloc(ls_struct->labels, sizeof(int) * ls_struct->capacity);
// 		if (!ls_struct->labels) {
// 			perror("Unable to rellocate space for labels\n");
// 			return;
// 		}
// 	}
// 	ls_struct->labels[++ls_struct->top] = label;
// 	ls_struct->size++;
// }

// bool is_label_stack_empty() {
// 	return ls_struct->top == -1;
// }

// int peek_label() {
// 	if (!ls_struct) return -1;

// 	if (!is_label_stack_empty()) {
// 		return ls_struct->labels[ls_struct->top];
// 	}
// 	return -1;
// }

// void write_asm_to_section(asm_writer* writer, char* text, section_t type) {
// 	fseek(writer->file, 0, SEEK_END);

// 	if (type == SECTION_DATA) {
// 		fprintf(writer->file, "%s\n", text);
// 	} else {
// 		fprintf(writer->file, "%s\n", text);
// 	}

// 	if (type == SECTION_DATA) {
// 		writer->current_data_pos = ftell(writer->file);
// 	} else {
// 		writer->current_text_pos = ftell(writer->file);
// 	}
	
// }

// void expression_codegen(Node* node) {
// 	if (!node) return;

// 	char buffer[32];

// 	switch (node->type) {
// 		case NODE_NAME: {
// 			node->reg = scratch_alloc(rt_struct);
// 			snprintf(buffer, sizeof(buffer), "\tmov %s, dword [rbp - %zu]", 
// 				scratch_name(rt_struct, node->reg),
// 				node->symbol->local_byte_offset);
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			break;
// 		}

// 		case NODE_DEF:
// 		case NODE_AUG: {
// 			expression_codegen(node->left);
// 			node->reg = node->left->reg;
// 			break;
// 		}

// 		case NODE_MUL: {
// 			printf("IN NODE_MUL\n");
// 			if (node->left->type != NODE_INTEGER && node->right->type != NODE_INTEGER) {
// 				expression_codegen(node->left);
// 				expression_codegen(node->right);

// 				snprintf(buffer, sizeof(buffer), "\tmul, %s, %s",
// 					scratch_name(rt_struct, node->left->reg),
// 					scratch_name(rt_struct, node->right->reg));

// 				node->reg = node->left->reg;
// 				scratch_free(rt_struct, node->left->reg);
// 				scratch_free(rt_struct, node->right->reg);
			
// 			} else if (node->left->type != NODE_INTEGER && node->right->type == NODE_INTEGER) {
// 				expression_codegen(node->left);

// 				snprintf(buffer, sizeof(buffer), "\timul %s, %s",
// 					scratch_name(rt_struct, node->left->reg),
// 					node->right->value.val);

// 				node->reg = node->left->reg;
// 				scratch_free(rt_struct, node->left->reg);
// 			} else if (node->left->type == NODE_INTEGER && node->right->type != NODE_INTEGER) {
// 				expression_codegen(node->right);

// 				snprintf(buffer, sizeof(buffer), "\timul %s, %d",
// 					scratch_name(rt_struct, node->right->reg),
// 					node->left->value.val);

// 				node->reg = node->right->reg;
// 				scratch_free(rt_struct, node->right->reg);

// 			} else {
// 				int product = node->left->value.val * node->right->value.val;
// 				node->reg = scratch_alloc(rt_struct);
// 				snprintf(buffer, sizeof(buffer), "\tmov %s, %d", scratch_name(rt_struct, node->reg), product);
// 			}
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
	
// 			break;
// 		}

// 		case NODE_SUB:
// 		case NODE_ADD: {
// 			char* type = (node->type == NODE_ADD) ? "add" : "sub";
// 			if (node->left->type != NODE_INTEGER && node->right->type != NODE_INTEGER) {
// 				expression_codegen(node->left);
// 				expression_codegen(node->right);

// 				snprintf(buffer, sizeof(buffer), "\t%s %s, %s",
// 					type, 
// 					scratch_name(rt_struct, node->left->reg), 
// 					scratch_name(rt_struct, node->right->reg));

// 				node->reg = node->left->reg;
// 				scratch_free(rt_struct, node->left->reg);	
// 				scratch_free(rt_struct, node->right->reg);

// 			} else if (node->left->type != NODE_INTEGER && node->right->type == NODE_INTEGER) {
// 				expression_codegen(node->left);
// 				snprintf(buffer, sizeof(buffer), "\t%s %s, %d",
// 					type,
// 					scratch_name(rt_struct, node->left->reg),
// 					node->right->value.val);

// 				node->reg = node->left->reg;
// 				scratch_free(rt_struct, node->left->reg);

// 			} else if (node->left->type == NODE_INTEGER && node->right->type != NODE_INTEGER) {
// 				expression_codegen(node->right);
// 				snprintf(buffer, sizeof(buffer), "\t%s %s, %d",
// 					type,
// 					scratch_name(rt_struct, node->right->reg),
// 					node->left->value.val);
				
// 				node->reg = node->right->reg;
// 				scratch_free(rt_struct, node->right->reg);
// 			} else {
// 				int sum = node->left->value.val + node->right->value.val;
// 				node->reg = scratch_alloc(rt_struct);
// 				snprintf(buffer, sizeof(buffer), "\tmov %s, %d", scratch_name(rt_struct, node->reg), sum);
// 			}

// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			break;
// 		}

// 		case NODE_EQUAL:
// 		case NODE_NOT_EQUAL:
// 		case NODE_LESS_EQUAL:
// 		case NODE_GREATER_EQUAL:
// 		case NODE_GREATER:
// 		case NODE_LESS: {
// 			expression_codegen(node->left);
// 			expression_codegen(node->right);

// 			snprintf(buffer, sizeof(buffer), "\tcmp %s, %s",
// 				scratch_name(rt_struct, node->left->reg),
// 				scratch_name(rt_struct, node->right->reg));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			scratch_free(rt_struct, node->left->reg);
// 			scratch_free(rt_struct, node->right->reg);
// 			break;
// 		}

// 		case NODE_LOGICAL_OR: {
// 			expression_codegen(node->left);
// 			expression_codegen(node->right);

// 			int label_true = label_create();
// 			int label_end = label_create();

// 			snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", scratch_name(rt_struct, node->left->reg));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "\tje %s", label_name(label_true));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", scratch_name(rt_struct, node->right->reg));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "\tje %s", label_name(label_true));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "\tmov rax, 0");
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "jmp %s", label_name(label_end));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "%s:", label_name(label_true));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "\tmov rax, 1");
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "%s:", label_name(label_end));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			break;
// 		}
// 		case NODE_LOGICAL_AND: {
// 			expression_codegen(node->left);
// 			expression_codegen(node->right);

// 			int label_false = label_create();
// 			int label_end = label_create();

// 			snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", scratch_name(rt_struct, node->left->reg));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "\tjne %s", label_name(label_false));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", scratch_name(rt_struct, node->right->reg));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "\tjne %s", label_name(label_false));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "\tmov rax, 1");
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			snprintf(buffer, sizeof(buffer), "jmp %s", label_name(label_end));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "%s:\n\tmov rax, 0", label_name(label_false));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			snprintf(buffer, sizeof(buffer), "%s:", label_name(label_end));
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 			break;

// 		}

// 		case NODE_DECREMENT:
// 		case NODE_INCREMENT: {
// 			char* type = (node->type == NODE_INCREMENT) ? "inc" : "dec";
// 			snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu]", 
// 				type,
// 				node->symbol->local_byte_offset);
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			break;
// 		}

// 		case NODE_BOOL:
// 		case NODE_CHAR:
// 		case NODE_INTEGER: {
// 			node->reg = scratch_alloc(rt_struct);
// 			if (node->reg > 0 && node->reg <= rt_struct->capacity) {
// 				snprintf(buffer, sizeof(buffer), "\tmov %s, %d", 
// 					scratch_name(rt_struct, node->reg), 
// 					node->value.val);
// 				write_asm_to_section(writer, buffer, SECTION_TEXT);
// 			}
// 			break;
// 		}

// 		case NODE_PARAM: {
// 			int param_count = 0;
// 			Node* wrapped_param = node;
// 			while (wrapped_param) {
// 				param_count++;
// 				wrapped_param = wrapped_param->next;
// 			}

// 			int param_index = 0;
// 			wrapped_param = node;
// 			while (wrapped_param && param_index <= FUNCTION_REGS_COUNT) {
// 				Node* actual_param = wrapped_param->right;
// 				actual_param->reg = scratch_alloc(rt_struct);
// 				char* current_function_reg = function_regs[param_index];
				
// 				snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
// 					scratch_name(rt_struct, actual_param->reg),
// 					current_function_reg);
// 				write_asm_to_section(writer, buffer, SECTION_TEXT);

// 				param_index++;
// 				wrapped_param = wrapped_param->next;
// 			}

// 			// Node* current_wrapped_param = wrapped_param;
// 			// Node* retrieval_node = current_wrapped_param;
// 			// while (retrieval_node) { retrieval_node = retrieval_node->next; } // last param
			
// 			// int difference = param_count - param_index;
// 			// if (difference > 0) {
// 			// 	int i = difference;
// 			// 	while (i >= 0 && retrieval_node) {
// 			// 		snprintf(buffer, sizeof(buffer), "")


// 			// 		i--;
// 			// 		retrieval_node = retrieval_node->prev;
// 			// 	}
// 			// }
// 			break;
// 		}

// 		case NODE_ARG: {
// 			expression_codegen(node->right);
// 			node->reg = node->right->reg;
// 			break;
// 		}

// 		case NODE_CALL: {
// 			int arg_count = 0;
// 			Node* arg = node->right;
// 			while (arg) {
// 				arg_count++;
// 				arg = arg->next;
// 			}

// 			arg = node->right;
// 			int arg_index = 0;
// 			while (arg && arg_index <= FUNCTION_REGS_COUNT) {
// 				char* current_function_reg = function_regs[arg_index];
// 				if (arg->type == NODE_INTEGER) {
// 					snprintf(buffer, sizeof(buffer), "\tmov %s, %d",
// 						current_function_reg,
// 						arg->value.val);
// 				} else {
// 					expression_codegen(arg);
// 					snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
// 						current_function_reg,
// 						scratch_name(rt_struct, arg->reg));
// 					scratch_free(rt_struct, arg->reg);
// 				}
// 				write_asm_to_section(writer, buffer, SECTION_TEXT);

// 				arg_index++;
// 				arg = arg->next;
// 			}

// 			Node* current_wrapped_arg = arg;
// 			Node* retrieval_node = current_wrapped_arg;
// 			while (retrieval_node) { retrieval_node = retrieval_node->next; } // get last arg;  

// 			int difference = arg_count - arg_index;
// 			if (difference > 0) {
// 				int i = difference;
// 				while (i >= 0 && retrieval_node) {
// 					expression_codegen(retrieval_node);
// 					snprintf(buffer, sizeof(buffer), "\tpush dword %s", scratch_name(rt_struct, retrieval_node->reg));
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);
// 					i--;
// 					retrieval_node = retrieval_node->prev;
// 				}
// 			}

// 			char* function_name = node->left->value.name;
// 			snprintf(buffer, sizeof(buffer), "\tcall %s", function_name);
// 			write_asm_to_section(writer, buffer, SECTION_TEXT);

// 		}
// 		default: break;
// 	}
// }

// void statement_codegen(Node* node) {
// 	if (!node) return;

// 	char buffer[32];

// 	switch (node->type) {
// 		case NODE_ASSIGNMENT: {
// 			if (node->left->type == NODE_DEF && node->right->type == NODE_INTEGER) {
// 				Node* def = node->left;
// 				if (def->left->type == NODE_NAME) {
// 					snprintf(buffer, sizeof(buffer), "\tmov dword [rbp - %zu], %d",
// 						def->left->symbol->local_byte_offset,
// 						node->right->value.val);
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);
					
// 				}
// 			} else if (node->left->type == NODE_DEF && node->right->type != NODE_INTEGER) {
// 				Node* def = node->left;
// 				expression_codegen(node->right);
// 				if (def->left->type == NODE_NAME) {
// 					snprintf(buffer, sizeof(buffer), "\tmov dword [rbp - %zu], %s",
// 						def->left->symbol->local_byte_offset,
// 						scratch_name(rt_struct, node->right->reg));
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);
// 				}
// 			}	

// 			break;
// 		}

// 		case NODE_RETURN: {
// 			if (node->right) {
// 				if (node->right->type == NODE_INTEGER) {
// 					snprintf(buffer, sizeof(buffer), "\tmov eax, %d", node->right->value.val);
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);
// 				} else {
// 					expression_codegen(node->right);
// 					if (strcmp(scratch_name(rt_struct, node->right->reg), "eax") != 0) {
// 						snprintf(buffer, sizeof(buffer), "\tmov eax, %s", scratch_name(rt_struct, node->right->reg));
// 						write_asm_to_section(writer, buffer, SECTION_TEXT);
// 					} 
// 				}
// 			}
// 			break;
// 		}

// 		// case NODE_IF: {
// 		// 	int label_condition_true = label_create();

// 		// 	push_label(label_condition_true);

// 		// 	expression_codegen(node->left);
// 		// 	int cf_count = 0;
// 		// 	if (node->next) {
// 		// 		Node* current = node->next;
// 		// 		while (current && (current->type == NODE_ELSE_IF || current->type == NODE_ELSE)) {
// 		// 			cf_count++;
// 		// 			current = current->next;
// 		// 		}
// 		// 	}

// 		// 	char* remaing_instructions_label = NULL;	
// 		// 	int label_next = 0;
// 		// 	if (cf_count > 0) {
// 		// 		remaing_instructions_label = label_name(cf_count + 1);
// 		// 		if (node->next->type == NODE_ELSE_IF || node->next->type == NODE_ELSE) {
// 		// 			label_next = label_create();
// 		// 			push_label(label_next);
// 		// 			switch (node->left->type) {
// 		// 				case NODE_GREATER:
// 		// 				case NODE_GREATER_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjl %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_LESS:
// 		// 				case NODE_LESS_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjg %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjne %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_NOT_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tje %s", label_name(label_next));
// 		// 					break;
// 		// 				}
// 		// 			}
// 		// 		} else {
// 		// 			int label_remaining_instructions = label_create();
// 		// 			push_label(label_remaining_instructions);
// 		// 			switch (node->left->type) {
// 		// 				case NODE_GREATER:
// 		// 				case NODE_GREATER_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjl %s", remaing_instructions_label);
// 		// 					break;
// 		// 				}

// 		// 				case NODE_LESS:
// 		// 				case NODE_LESS_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjg %s", remaing_instructions_label);
// 		// 					break;
// 		// 				}

// 		// 				case NODE_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjne %s", remaing_instructions_label);
// 		// 					break;
// 		// 				}
// 		// 				case NODE_NOT_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tje %s", remaing_instructions_label);
// 		// 					break;
// 		// 				}
// 		// 			}	
// 		// 		}
// 		// 		write_asm_to_section(writer, buffer, SECTION_TEXT);
// 		// 	}

// 		// 	snprintf(buffer, sizeof(buffer), "%s:", label_name(label_condition_true));
// 		// 	write_asm_to_section(writer, buffer, SECTION_TEXT);

// 		// 	statement_codegen(node->right);
// 		// 	snprintf(buffer, sizeof(buffer), "\tjmp %s", remaing_instructions_label);
// 		// 	write_asm_to_section(writer, buffer, SECTION_TEXT);
			
// 		// 	snprintf(buffer, sizeof(buffer), "%s:", label_name(label_next));
// 		// 	write_asm_to_section(writer, buffer, SECTION_TEXT);
// 		// 	break;
// 		// }

// 		// case NODE_ELSE_IF: {
// 		// 	// int label_condition_true = label_create();
// 		// 	int label_next = peek_label();

// 		// 	expression_codegen(node->left);

// 		// 	if (node->next) {
// 		// 		int label_next = label_create();
// 		// 		if (node->next->type == NODE_ELSE_IF || node->next->type == NODE_ELSE) {
// 		// 			switch (node->left->type) {
// 		// 				case NODE_GREATER:
// 		// 				case NODE_GREATER_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjl %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_LESS:
// 		// 				case NODE_LESS_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjg %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjne %s", label_name(label_next));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_NOT_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tje %s", label_name(label_next));
// 		// 					break;
// 		// 				}
// 		// 			}
// 		// 		} else {

// 		// 			switch (node->left->type) {
// 		// 				case NODE_GREATER:
// 		// 				case NODE_GREATER_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjl %s", label_name(label_remaining_instructions));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_LESS:
// 		// 				case NODE_LESS_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjg %s", label_name(label_remaining_instructions));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tjne %s", label_name(label_remaining_instructions));
// 		// 					break;
// 		// 				}

// 		// 				case NODE_NOT_EQUAL: {
// 		// 					snprintf(buffer, sizeof(buffer), "\tje %s", label_name(label_remaining_instructions));
// 		// 					break;
// 		// 				}
// 		// 			}
// 		// 		}
// 		// 		write_asm_to_section(writer, buffer, SECTION_TEXT);
// 		// 	}

// 		// 	// snprintf(buffer, sizeof(buffer), "%s:", label_name(label_condition_true));
// 		// 	// write_asm_to_section(writer, buffer, SECTION_TEXT);
			
// 		// 	statement_codegen(node->right);
			
// 		// 	snprintf(buffer, sizeof(buffer), "%s:", label_name(label_remaining_instructions));
// 		// 	write_asm_to_section(writer, buffer, SECTION_TEXT);

// 		// 	break;
// 		// }

// 		case NODE_BLOCK: {
// 			Node* stmt = node->right;
// 			while (stmt) {
// 				Node* next_stmt = stmt->next;
// 				statement_codegen(stmt);
// 				stmt = next_stmt;
// 			} 
// 			break;
// 		}

// 		case NODE_INCREMENT:
// 		case NODE_DECREMENT: {
// 			expression_codegen(node->left);
// 			break;
// 		}

// 		case NODE_CALL: {
// 			expression_codegen(node);
// 			break;
// 		}
// 		default: break;
// 	}
// }

// void second_pass_globals_codegen(Node* global_node) {
// 	char buffer[256];	

// 	switch (global_node->type) {
// 		case NODE_NAME: {
// 			if (global_node->t) {
// 				if (global_node->t->kind == TYPE_FUNCTION) {
// 					snprintf(buffer, sizeof(buffer), "%s:", global_node->value.name);
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);

// 					snprintf(buffer, sizeof(buffer), "\tpush rbp\n\tmov rbp, rsp\n\tsub rsp, %zu", global_node->symbol->total_bytes);
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);

// 					if (global_node->t->params) {
// 						expression_codegen(global_node->t->params);
// 					}

// 					if (global_node->right) {
// 						statement_codegen(global_node->right);
// 					}

// 					if (!global_node->next) {
// 						snprintf(buffer, sizeof(buffer), "\tleave\n\tret");
// 						write_asm_to_section(writer, buffer, SECTION_TEXT);
// 					} else {
// 						snprintf(buffer, sizeof(buffer), "\tleave\n\tret\n");
// 						write_asm_to_section(writer, buffer, SECTION_TEXT);
// 					}


// 				}
// 			}
// 			break;
// 		}
// 	}
// }

// void first_pass_globals_codegen(Node* global_node) {
// 	char* buffer[100];

// 	switch (global_node->type) {
// 		case NODE_NAME: {
// 			if (global_node->t) {
// 				if (global_node->t->kind == TYPE_FUNCTION) {
// 					snprintf(buffer, sizeof(buffer), "global %s", global_node->value.name);
// 					write_asm_to_section(writer, buffer, SECTION_TEXT);
					
// 				}
// 			}
// 			break;
// 		}
// 	}
// }

// void codegen(Node* root, char* output) {
// 	init_asm_writer(output);

// 	write_asm_to_section(writer, "section .data\n", SECTION_DATA);
// 	write_asm_to_section(writer, "section .text", SECTION_TEXT);

// 	Node* global_node = root;
// 	while (global_node) {
// 		first_pass_globals_codegen(global_node);
// 		global_node = global_node->next;
// 	}

// 	write_asm_to_section(writer, "global _start\n\n_start:\n", SECTION_TEXT);

// 	global_node = root;
// 	while (global_node) {

// 		global_node = global_node->next;
// 	}

// 	rt_struct = create_register_table();
// 	ls_struct = create_label_stack();
// 	global_node = root;
// 	while (global_node) {
// 		second_pass_globals_codegen(global_node);
// 		global_node = global_node->next;
// 	}

// 	fclose(writer->file);
// 	free_register_table();
// 	free_label_stack();
// }

// void free_register(struct Register* reg) {
// 	if (!reg) return;
// 	if (reg->name) free(reg->name);
// 	free(reg);
// }

// void free_register_table() {
// 	if (!rt_struct) return;

// 	for (int i = 0; i < rt_struct->capacity; i++) {
// 		free_register(rt_struct->register_table[i]);
// 	}
// }

// void free_label_stack() {
// 	if (!ls_struct) return;

// 	if (ls_struct->labels) {
// 		free(ls_struct->labels);
// 	} 
// 	free(ls_struct);
// }