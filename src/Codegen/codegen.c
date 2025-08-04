#include "codegen.h"

static size_t total_bytes = 0;
static jmp_true_index = 1;
static jmp_false_index = 1;
static jmp_end_index = 1;

char* registers[] = {
	"rax", "rbx", "rdi", "rsi", "rdx", "rcx",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

void write_asm_to_file(ASMWriter* writer, char* text) {
	if (!text) return;
	fseek(writer->file, 0, SEEK_END);
	fprintf(writer->file, "%s\n", text);
}

bool is_caller_saved(int reg) {
	if (reg == 0 || reg == 2 ||
		reg == 3 || reg == 4 ||
		reg == 5 || reg == 6 ||
		reg == 7 || reg == 8 || 
		reg == 9) {
		return true;
	}
	return false;
}

bool is_callee_saved(int reg) {
	if (reg == 1 || reg == 10 ||
		reg == 11 || reg == 12 ||
		reg == 13) {
		return true;
	}
	return false;
}

char* operator_to_string(tac_t type) {
	switch (type) {
		case TAC_ADD: return "add";
		case TAC_SUB: return "sub";
		case TAC_MUL: return "mul";
		case TAC_DIV: return "div";
	}
}

char* generate_jmp_label(CompilerContext* ctx, jmp_label_t type) {   
	char buffer[40];

	switch (type) {
		case TRUE_LBL: {
	    	snprintf(buffer, sizeof(buffer), ".L_true%d", jmp_true_index++);
	    	break;
	 	}

	 	case FALSE_LBL: {
	    	snprintf(buffer, sizeof(buffer), ".L_false%d", jmp_false_index++);
	    	break;
	 	}

		case END_LBL: {
	     	snprintf(buffer, sizeof(buffer), ".L_end%d", jmp_end_index++);
	     	break;
	 	}
 }

	 char* jmp_label = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	 if (!jmp_label) return NULL;

	 strcpy(jmp_label, buffer);
	 return jmp_label;

}

char* get_op_code(tac_t type) {
	switch (type) {
		case TAC_NOT_EQUAL: return "je";
		case TAC_EQUAL: return "jne";
		
		case TAC_LESS:
		case TAC_LESS_EQUAL: return "jg";

 		case TAC_GREATER:
 		case TAC_GREATER_EQUAL: return "jl";

	}
}

void generate_jmp_asm(TACInstruction* tac, char* false_label, ASMWriter* writer) {
	if (!tac || !false_label) return;

	char buffer[60];
	snprintf(buffer, sizeof(buffer), "\t%s %s", get_op_code(tac->kind), false_label);
	write_asm_to_file(writer, buffer);
}

void generate_logical_operator_asm(TACInstruction* tac, Operand* jmp_op, ASMWriter* writer) {
	if (!tac || !jmp_op) return;

	char buffer[100];

	bool both_ops_spill = tac->op1->needs_spill && tac->op2->needs_spill; 
	bool only_op1_spill = tac->op1->needs_spill && !tac->op2->needs_spill;
	bool only_op2_spill = !tac->op1->needs_spill && tac->op2->needs_spill;

	if (both_ops_spill) {
		snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], [rbp - %zu]",
			tac->op1->frame_byte_offset,
			tac->op2->frame_byte_offset
		);
	
	} else if (only_op1_spill) {
		snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], %s",
			tac->op1->frame_byte_offset,
			registers[tac->op2->assigned_register]
		);

	} else if (only_op2_spill) {
		snprintf(buffer, sizeof(buffer), "\tcmp %s, [rbp - %zu]",
			registers[tac->op1->assigned_register],
			tac->op2->frame_byte_offset
		);
	
	} else {
		// printf("op1 register => %d\n", tac->op1->assigned_register);
		// printf("op2 register => %d\n", tac->op2->assigned_register);
		snprintf(buffer, sizeof(buffer), "\tcmp %s, %s",
			registers[tac->op1->assigned_register],
			registers[tac->op2->assigned_register]
		);
	}

	write_asm_to_file(writer, buffer);

	switch (tac->kind) {
		case TAC_LESS:
		case TAC_LESS_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjg %s", jmp_op->value.label_name);
			break;	
		}

		case TAC_GREATER:
		case TAC_GREATER_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjl %s", jmp_op->value.label_name);
			break;
		}

		case TAC_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjne %s", jmp_op->value.label_name);
			break;			
		}

		case TAC_NOT_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tje %s", jmp_op->value.label_name);
			break;
		}
	}

	write_asm_to_file(writer, buffer);
}

void generate_function_body(CompilerContext* ctx, CFG* cfg, ASMWriter* writer) {
	char buffer[100];

	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i]; 
		for (int j = 0; j < block->num_instructions; j++) {
			TACInstruction* tac = block->instructions[j];
			if (!tac || (tac && tac->handled)) continue;

			switch (tac->kind) {
				case TAC_BOOL:
				case TAC_CHAR:
				case TAC_INTEGER: {
					if (tac->result->needs_spill) {
						snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %d",
							tac->result->frame_byte_offset,
							tac->op1->value.int_val
						);

					} else {
						snprintf(buffer, sizeof(buffer), "\tmov %s, %d",
							registers[tac->result->assigned_register],
							tac->op1->value.int_val
						);
					}
					write_asm_to_file(writer, buffer);
					break;
				}

				case TAC_LABEL: {
					snprintf(buffer, sizeof(buffer), "%s:", tac->result->value.label_name);
					write_asm_to_file(writer, buffer);
					break;
				}

				case TAC_GOTO: {
					snprintf(buffer, sizeof(buffer), "\tjmp %s", tac->result->value.label_name);
					write_asm_to_file(writer, buffer);
					break;
				}

				case TAC_RETURN: {
					if (tac->result) {
						if (tac->op1) {
							bool can_emit = false;
							if (tac->op1->needs_spill) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
									registers[tac->result->assigned_register],
									tac->op1->frame_byte_offset
								);
								can_emit = true;

							} else {
								if (strcmp(registers[tac->op1->assigned_register], "rax") != 0) {
									snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
										registers[tac->result->assigned_register],
										registers[tac->op1->assigned_register]
									);									
									can_emit = true;
								}
								
							}

							if (can_emit) {
								write_asm_to_file(writer, buffer);
							}							
						}

					} 
					break;
				}

				case TAC_ASSIGNMENT: {
					if (tac->result) {
						if (tac->result->needs_spill) {
							if (tac->op2->needs_spill) {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], [rbp - %zu]",
									tac->result->frame_byte_offset,
									tac->op2->frame_byte_offset
								);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
									tac->result->frame_byte_offset,
									registers[tac->op2->assigned_register]
								);
							}

						} else {
							if (tac->op2->needs_spill) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
									registers[tac->result->assigned_register],
									tac->op2->frame_byte_offset
								);
							} else {
								printf("result reg: %s\n", registers[tac->result->assigned_register]);
								printf("op2 reg: %s\n", registers[tac->op2->assigned_register]);
								snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
									registers[tac->result->assigned_register],
									registers[tac->op2->assigned_register]
								);
							}
						}
						write_asm_to_file(writer, buffer);
					} 
					break;
				}

				case TAC_NOT_EQUAL:
				case TAC_EQUAL:
				case TAC_LESS:
				case TAC_GREATER:
				case TAC_LESS_EQUAL:
				case TAC_GREATER_EQUAL: {
					Operand* jmp_op = NULL;
					if (j + 1 < block->num_instructions) {
						TACInstruction* next_tac = block->instructions[j + 1];
						if (next_tac) {
							switch (next_tac->kind) {
								case TAC_IF_FALSE: {
									next_tac->handled = true;
									jmp_op = next_tac->op2;
									break;
								}
							}
						}

						if (jmp_op) {
							generate_logical_operator_asm(tac, jmp_op, writer);
						} else {
							char* false_label = generate_jmp_label(ctx, FALSE_LBL);
							char* end_label = generate_jmp_label(ctx, END_LBL);
						
                     		generate_jmp_asm(tac, false_label, writer);

						}
					}
					break;
				}

				case TAC_ADD:
				case TAC_SUB: {
					bool both_ops_spill = tac->op1->needs_spill && tac->op2->needs_spill; 
					bool only_op1_spill = tac->op1->needs_spill && !tac->op2->needs_spill;
					bool only_op2_spill = !tac->op1->needs_spill && tac->op2->needs_spill;
					
					if (both_ops_spill) {
						snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], [rbp - %zu]", 
							operator_to_string(tac->kind),
							tac->op1->frame_byte_offset,
							tac->op2->frame_byte_offset
						);

					} else if (only_op1_spill) {
						snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], %s",
							operator_to_string(tac->kind),
							tac->op1->frame_byte_offset,
							registers[tac->op2->assigned_register]
						);

					} else if (only_op2_spill)  {
						snprintf(buffer, sizeof(buffer), "\t%s %s, [rbp - %zu]",
							operator_to_string(tac->kind),
							registers[tac->op1->assigned_register],
							tac->op2->frame_byte_offset
						);

					} else {
						snprintf(buffer, sizeof(buffer), "\t%s %s, %s",
							operator_to_string(tac->kind),
							registers[tac->op1->assigned_register],
							registers[tac->op2->assigned_register]
						);					
					}
					
					write_asm_to_file(writer, buffer);

					if (tac->result) {
						if (tac->result->needs_spill) {
							if (tac->op1->needs_spill) {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], [rbp - %zu]",
									tac->result->frame_byte_offset,
									tac->op1->frame_byte_offset
								);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
									tac->result->frame_byte_offset,
									registers[tac->op1->assigned_register]
								);
							}
						} else {
							if (tac->op1->needs_spill) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
									registers[tac->result->assigned_register],
									tac->op1->frame_byte_offset
								);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
									registers[tac->result->assigned_register],
									registers[tac->op1->assigned_register]
								);
							}
						}
						write_asm_to_file(writer, buffer);
					}
					break;
				}

				case TAC_MUL:
				case TAC_DIV: {
					if (tac->result->needs_spill) {
						bool both_ops_spill = tac->op1->needs_spill && tac->op2->needs_spill; 
						bool only_op1_spill = tac->op1->needs_spill && !tac->op2->needs_spill;
						bool only_op2_spill = !tac->op1->needs_spill && tac->op2->needs_spill;

						if (both_ops_spill) {
							snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], [rbp - %zu], [rbp - %zu]", 
								operator_to_string(tac->kind),
								tac->result->frame_byte_offset,
								tac->op1->frame_byte_offset,
								tac->op2->frame_byte_offset
							);
						
						} else if (only_op1_spill) {
							snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], [rbp - %zu], %s",
								operator_to_string(tac->kind),
								tac->result->frame_byte_offset,
								tac->op1->frame_byte_offset,
								registers[tac->op2->assigned_register]
							);

						} else if (only_op2_spill)  {
							snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], %s, [rbp - %zu]",
								operator_to_string(tac->kind),
								tac->result->frame_byte_offset,
								registers[tac->op1->assigned_register],
								tac->op2->frame_byte_offset
							);

						} else {
							snprintf(buffer, sizeof(buffer), "\t%s [rbp - %zu], %s, %s",
								operator_to_string(tac->kind),
								tac->result->frame_byte_offset,
								registers[tac->op1->assigned_register],
								registers[tac->op2->assigned_register]
							);					
						}

					} else {
						bool both_ops_spill = tac->op1->needs_spill && tac->op2->needs_spill; 
						bool only_op1_spill = tac->op1->needs_spill && !tac->op2->needs_spill;
						bool only_op2_spill = !tac->op1->needs_spill && tac->op2->needs_spill;

						if (both_ops_spill) {
							snprintf(buffer, sizeof(buffer), "\t%s %s, [rbp - %zu], [rbp - %zu]", 
								operator_to_string(tac->kind),
								registers[tac->result->assigned_register],
								tac->op1->frame_byte_offset,
								tac->op2->frame_byte_offset
							);
						
						} else if (only_op1_spill) {
							snprintf(buffer, sizeof(buffer), "\t%s %s, [rbp - %zu], %s",
								operator_to_string(tac->kind),
								registers[tac->result->assigned_register],
								tac->op1->frame_byte_offset,
								registers[tac->op2->assigned_register]
							);

						} else if (only_op2_spill) {
							snprintf(buffer, sizeof(buffer), "\t%s %s, %s, [rbp - %zu]",
								operator_to_string(tac->kind),
								registers[tac->result->assigned_register],
								registers[tac->op1->assigned_register],
								tac->op2->frame_byte_offset
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\t%s %s, %s, %s",
								operator_to_string(tac->kind),
								registers[tac->result->assigned_register],
								registers[tac->op1->assigned_register],
								registers[tac->op2->assigned_register]
							);
						}
					}

					write_asm_to_file(writer, buffer);
					break;
				}

				case TAC_CALL: {
					for (int i = 0; i < tac->live_out->size; i++) {
						if (tac->live_out->elements[i] && is_caller_saved(tac->live_out->elements[i]->assigned_register)) {
							snprintf(buffer, sizeof(buffer), "\tpush %s", registers[tac->live_out->elements[i]->assigned_register]);
							write_asm_to_file(writer, buffer);
						}
					}

					snprintf(buffer, sizeof(buffer), "\tcall %s", tac->result->value.sym->name);
					write_asm_to_file(writer, buffer);

					
					for (int i = tac->live_out->size - 1; i >= 0; i--) {
						if (tac->live_out->elements[i] && is_caller_saved(tac->live_out->elements[i]->assigned_register)) {
							snprintf(buffer, sizeof(buffer), "\tpop %s", registers[tac->live_out->elements[i]->assigned_register]);
							write_asm_to_file(writer, buffer);
						}
					}
					write_asm_to_file(writer, "");

					break;
				}
			}		
		}
	}
}

void generate_function_prologue(CompilerContext* ctx, FunctionInfo* info, ASMWriter* writer) {
	write_asm_to_file(writer, "\tpush rbp");
	write_asm_to_file(writer, "\tmov rbp, rsp");

	char buffer[40];
	if (info->total_frame_bytes > 0) {
		snprintf(buffer, sizeof(buffer), "\tsub rsp, %d\n", info->total_frame_bytes);
		write_asm_to_file(writer, buffer);
	}
}

char* create_function_label(CompilerContext* ctx, char* func_name) {
	if (!func_name) return NULL;

	int res_length = strlen(func_name) + 2;
	char* func_label = arena_allocate(ctx->codegen_arena, res_length);
	if (!func_label) return NULL;

	strncpy(func_label, func_name, res_length - 2);
	func_label[res_length - 2] = '\0';

	strcat(func_label, ":");
	return func_label;
} 

void emit_asm_for_functions(CompilerContext* ctx, FunctionList* function_list, ASMWriter* writer) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		bool last_function = (i == function_list->size - 1);

		if (info->symbol) {
			char* func_label = create_function_label(ctx, info->symbol->name);
			write_asm_to_file(writer, func_label);
		}

		generate_function_prologue(ctx, info, writer);
		generate_function_body(ctx, info->cfg, writer);

		if (last_function) {
			write_asm_to_file(writer, "\n\tleave\n\tret");
		} else {
			write_asm_to_file(writer, "\n\tleave\n\tret\n");

		}
	}
}

char* get_full_text(CompilerContext* ctx, char* func_name) {
	char buffer[120];
	snprintf(buffer, sizeof(buffer), "public %s", func_name);

	int length = strlen(buffer);
	char* full_text = arena_allocate(ctx->codegen_arena, length + 1);
	if (!full_text) return NULL;	 
	
	strncpy(full_text, buffer, length);
	full_text[length] = '\0';
	return full_text;
}

void generate_globals(CompilerContext* ctx, ASMWriter* writer) {
	write_asm_to_file(writer, "format ELF64");
	write_asm_to_file(writer, "section '.text' executable\n");

	for (int i = 0; i < ctx->global_table->capacity; i++) {
		Symbol* func_symbol = ctx->global_table->symbols[i];
		if (func_symbol) {
			char* full_text = get_full_text(ctx, func_symbol->name);
			if (full_text) {
				
				write_asm_to_file(writer, full_text);
			}			
		}
	}

	write_asm_to_file(writer, "public _start\n\n_start:");

	char buffer[50];

	snprintf(buffer, sizeof(buffer), "\tcall main");
	write_asm_to_file(writer, buffer);

	snprintf(buffer, sizeof(buffer), "\tmov rdi, rax");
	write_asm_to_file(writer, buffer);

	snprintf(buffer, sizeof(buffer), "\tmov rax, 60");
	write_asm_to_file(writer, buffer);

	snprintf(buffer, sizeof(buffer), "\tsyscall\n");
	write_asm_to_file(writer, buffer);

}

void ensure_alignment(int* op_size, int alignment) {
	if (*op_size % alignment != 0) {
		*op_size += alignment - (*op_size % alignment);
	}
}

bool contains_operand_symbol(OperandSet* op_set, Symbol* target_symbol) {
	if (!target_symbol) return false;

	for (int i = 0; i < op_set->size; i++) {
		if (op_set->elements[i]->value.sym == target_symbol) {
			return true;
		}
	}

	return false;
}

size_t get_size(Operand* op) {
	if (!op) return 0;

	switch (op->type) {
		case TYPE_INTEGER: return sizeof(int);
		case TYPE_BOOL:
		case TYPE_CHAR: return sizeof(char);
		default: {
			if (op->value.sym && op->value.sym->type) {				
				if (op->value.sym->type->subtype) {
					switch (op->value.sym->type->subtype->kind) {
						case TYPE_INTEGER: return sizeof(int);
						case TYPE_BOOL:
						case TYPE_CHAR: return sizeof(char);
					}
				}	
			}
			break;
		}
	}
	
}

void get_bytes_from_operand(CompilerContext* ctx, OperandSet* symbols_set, Operand* op) {
	if (!op) return;

	switch (op->kind) {
		case OP_SYMBOL: {
			if (op->needs_spill) {
				if (!contains_operand_symbol(symbols_set, op->value.sym)) {
					add_to_operand_set(ctx, symbols_set, op);
				
					size_t op_size = get_size(op);
					ensure_alignment(&op_size, EIGHT_BYTE_ALIGNMENT);
				
					total_bytes += EIGHT_BYTE_ALIGNMENT;
					op->value.sym->frame_byte_offset = total_bytes;
				} 
			} 
			break;
		}

		default: {
			if (op->needs_spill) {
				size_t op_size = get_size(op);
				ensure_alignment(&op_size, EIGHT_BYTE_ALIGNMENT);
					
				total_bytes += EIGHT_BYTE_ALIGNMENT;
				op->frame_byte_offset = total_bytes;

			} 
			break;
		}
	}
}

void adjust_bytes_for_frame(CompilerContext* ctx, OperandSet* symbols_set, TACInstruction* instruction) {
	if (!instruction) return;
		
	switch (instruction->kind) {
		case TAC_ARG:
		case TAC_PARAM: {
			get_bytes_from_operand(ctx, symbols_set, instruction->op1);
			break;
		}

		case TAC_ASSIGNMENT:
		case TAC_INTEGER:
		case TAC_CHAR:
		case TAC_BOOL:
		case TAC_LESS:
		case TAC_GREATER:
		case TAC_LESS_EQUAL:
		case TAC_GREATER_EQUAL:
		case TAC_NOT:
		case TAC_EQUAL:
		case TAC_NOT_EQUAL:
		case TAC_LOGICAL_AND:
		case TAC_LOGICAL_OR:
		case TAC_STORE:
		case TAC_DEREFERENCE:
		case TAC_DEREFERENCE_AND_ASSIGN:
		case TAC_UNARY_ADD:
		case TAC_UNARY_SUB:
		case TAC_MODULO:
		case TAC_DIV:
		case TAC_MUL:
		case TAC_SUB:
		case TAC_ADD: {
			get_bytes_from_operand(ctx, symbols_set, instruction->result);
			break;
		}
	}
}

void accumulate_bytes(CompilerContext* ctx, CFG* cfg, OperandSet* symbols_set, Symbol* func_symbol) {
	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i];
		for (int j = 0; j < block->num_instructions; j++) {
			TACInstruction* instruction = block->instructions[j];
			adjust_bytes_for_frame(ctx, symbols_set, block->instructions[j]);
		}
	}
}

void get_bytes_for_stack_frames(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		total_bytes = 0;
		
		if (info) {
			OperandSet* symbols_set = create_operand_set(ctx);
			accumulate_bytes(ctx, info->cfg, symbols_set, info->symbol);
			info->total_frame_bytes = total_bytes;
			ensure_alignment(&info->total_frame_bytes, SIXTEEN_BYTE_ALIGNMENT);
		}
	}
}

char* get_filename(CompilerContext* ctx, char* file) {
	char* output = NULL;

	size_t filename_length = strlen(file);
	for (size_t i = 0; i < filename_length; i++) {
		if (file[i] == '.') {
			int length = &file[i] - file;
			output = arena_allocate(ctx->lexer_arena, length + 5);
			if (!output) {
				perror("Unable to allocate space for output string.\n");
				return NULL;
			}
			strncpy(output, file, length);
			output[length] = '\0';
			strcat(output, ".asm");
		}
	}
	return output;
}

ASMWriter* create_asm_writer(CompilerContext* ctx, char* file) {
	ASMWriter* writer = arena_allocate(ctx->codegen_arena, sizeof(ASMWriter));
	if (!writer) return NULL;

	writer->current_data_pos = 0;
	writer->current_text_pos = 0;

	writer->filename = get_filename(ctx, file);
	if (!writer->filename) {
		printf("Unable to get filename\n");
		return NULL;
	}

	writer->file = fopen(writer->filename, "w");
	if (!writer->file) return NULL;

	return writer;
} 

void codegen(CompilerContext* ctx, FunctionList* function_list, char* file) {
	ASMWriter* writer = create_asm_writer(ctx, file);
	if (!writer) return;

	get_bytes_for_stack_frames(ctx, function_list);
	generate_globals(ctx, writer);
	emit_asm_for_functions(ctx, function_list, writer);

	fclose(writer->file);
}