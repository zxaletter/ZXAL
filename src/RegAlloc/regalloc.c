#include "regalloc.h"

IdealizedASMInstructionList asm_instruction_list;
static int jmp_true_index = 1;
static int jmp_false_index = 1;
static int jmp_end_index = 1;

IdealizedASMInstructionList create_idealized_asm_instruction_list(CompilerContext* ctx) {
	IdealizedASMInstruction* instructions = arena_allocate(ctx->ir_arena, sizeof(IdealizedASMInstruction) * INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY);
	if (!instructions) {
		IdealizedASMInstructionList dummy_list = {
			.size = 0,
			.capacity = 0,
			.instructions = NULL
		};
		return dummy_list;
	}

	IdealizedASMInstructionList new_list = {
		.size = 0,
		.capacity = INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY,
		.instructions = instructions
	};
	return new_list;
}

bool store_idealized_asm_instruction(CompilerContext* ctx, IdealizedASMInstruction instruction) {
	if (asm_instruction_list.size >= asm_instruction_list.capacity) {
		int prev_capacity = asm_instruction_list.capacity;

		asm_instruction_list.capacity *= 2;
		int new_capacity = asm_instruction_list.capacity;
		void* new_asm_instructions = arena_reallocate(
			ctx->ir_arena, 
			asm_instruction_list.instructions, 
			prev_capacity * sizeof(IdealizedASMInstruction), 
			new_capacity * sizeof(IdealizedASMInstruction)
		);

		if (!new_asm_instructions) {
			printf("In 'store_idealized_asm_instruction', unable to reallocate space for new asm instructions\n");
			return false;
		}

		asm_instruction_list.instructions = new_asm_instructions;
	}
	asm_instruction_list.instructions[asm_instruction_list.size++] = instruction;
	return true;
}

IdealizedASMInstruction create_idealized_asm_instruction(OpCode op, Symbol* symbol, 
	Operand* dest, Operand* src1, Operand* src2, int immediate_val) {
	
	IdealizedASMInstruction instruction = {
		.op = op,
		.symbol = symbol,
		.dest = dest,
		.src1 = src1,
		.src2 = src2,
		.immediate_val = immediate_val
	};
	return instruction;
}

OpCode get_op_code(TACInstruction* tac) {
	switch (tac->type) {
		case TAC_ADD: return ADD;
		case TAC_SUB: return SUB;
		case TAC_MUL: return MUL;
		case TAC_DIV: return DIV;
		case TAC_MODULO: return MODULO;
		
		case TAC_LESS:
		case TAC_LESS_EQUAL: {
			return JMP_G;
		}

		case TAC_GREATER:
		case TAC_GREATER_EQUAL: {
			return JMP_L;
		}

		case TAC_EQUAL: return JMP_NE;
		case TAC_NOT_EQUAL: return JMP_E;
	}
}

char* get_idealized_asm_instruction_type_name(OpCode op) {
	switch (op) {
		case ADD: return "ADD";
		case SUB: return "SUB";
		case MUL: return "MUL";
		case DIV: return "DIV";
		case CMP: return "CMP";
		case MODULO: return "MODULO";
		case JMP: return "JMP";
		case JMP_E: return "JMP_E";
		case JMP_NE: return "JMP_NE";
		case JMP_G: return "JMP_G";
		case JMP_L: return "JMP_L";
	}
}

bool is_symbol(operand_t type) {
	switch (type) {
		case OP_SUBTYPE_STR:
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
		case OP_NOT_EQUAL:
		case OP_EQUAL:
		case OP_LOGICAL_AND:
		case OP_LOGICAL_OR:
		case OP_STORE: {
			return true;
		}
		default: return false;
	}
}

bool is_int_literal(operand_t type) {
	switch (type) {
		case OP_INT_LITERAL: return true;
		default: return false;
	}
}

char* generate_jmp_label(CompilerContext* ctx, label_t type) {	
	char buffer[40];

	switch (type) {
		case TRUE: {
			snprintf(buffer, sizeof(buffer), ".L_true%d", jmp_true_index++);
			break;
		}

		case FALSE: {
			snprintf(buffer, sizeof(buffer), ".L_false%d", jmp_false_index++);
			break;
		}

		case END: {
			snprintf(buffer, sizeof(buffer), ".L_end%d", jmp_end_index++);
			break;
		}
	}

	char* jmp_label = arena_allocate(ctx->ir_arena, strlen(buffer) + 1);
	if (!jmp_label) return NULL;

	strcpy(jmp_label, buffer);
	return jmp_label;

}

void emit_idealized_asm_instructions() {
	for (int i = 0; i < asm_instruction_list.size; i++) {
		IdealizedASMInstruction instruction = asm_instruction_list.instructions[i];
		switch (instruction.op) {
			case LABEL: {
				if (instruction.symbol) {
					if (instruction.symbol->type->kind == TYPE_FUNCTION) {
						printf("\n%s:\n", instruction.symbol->name);
					}
				} else {
					printf("%s\n", instruction.dest->value.label_name);
				}
				break;
			}

			case RETURN: {
				if (instruction.dest) {
					switch (instruction.dest->kind) {
						case OP_SYMBOL: {
							printf("\tRETURN %s\n", instruction.dest->value.sym->name);
							break;
						}
						case OP_STORE: {
							printf("\tRETURN %s\n", instruction.dest->value.label_name);
							break;
						}
						default: {
							printf("\tRETURN %s\n", instruction.dest->value.label_name);
							break;
						}
					}
				} else {
					printf("\tRETURN\n");
				}
				break;
			}

			case JMP_NE:
			case JMP_E:
			case JMP_G:
			case JMP_L:
			case JMP: {
				char* jmp_instruction = get_idealized_asm_instruction_type_name(instruction.op);
				if (!jmp_instruction) break;

				printf("\t%s %s\n", jmp_instruction, instruction.dest->value.label_name);
				break;
			}

			case CMP: {
				printf("\tCMP");
				if (instruction.src1) {
					switch (instruction.src1->kind) {
						case OP_SYMBOL: {
							printf(" %s,", instruction.src1->value.sym->name);
							break;
						}
						case OP_ADD:
						case OP_SUB:
						case OP_MUL:
						case OP_DIV:
						case OP_NOT_EQUAL:
						case OP_EQUAL:
						case OP_MODULO:
						case OP_STORE: {
							printf(" %s,", instruction.src1->value.label_name);
							break;
						}
					}
				}

				if (instruction.src2) {
					switch (instruction.src2->kind) {
						case OP_SYMBOL: {
							printf(" %s\n", instruction.src2->value.sym->name);
							break;
						}

						case OP_ADD:
						case OP_SUB:
						case OP_MUL:
						case OP_DIV:
						case OP_MODULO:
						case OP_NOT_EQUAL:

						case OP_EQUAL:
						case OP_STORE: {
							printf(" %s\n", instruction.src2->value.label_name);
							break;
						}

						case OP_INT_LITERAL: {
							printf(" %d\n", instruction.src2->value.int_val);
							break;
						}
					}
				}
				
				break;
			}

			case MODULO:
			case ADD: 
			case SUB: 
			case MUL: 
			case DIV: {
				char* idealized_asm_instruction_type_name = get_idealized_asm_instruction_type_name(instruction.op);
				if (!idealized_asm_instruction_type_name) {
					printf("Idealized asm instruction type name is NULL\n");
				} else {
					// printf("Retrieved asm instruction type name\n");
				}
				printf("\t%s %s,", 
					idealized_asm_instruction_type_name, 
					instruction.dest->value.label_name
				);

				switch (instruction.src1->kind) {
					case OP_SYMBOL: {
						printf(" %s,", instruction.src1->value.sym->name);
						break;
					}

					case OP_SUBTYPE_STR:
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MODULO:
					case OP_STORE: {
						printf(" %s,", instruction.src1->value.label_name);
						break;
					}
					case OP_INT_LITERAL: {
						printf(" %d,", instruction.src1->value.int_val);
						break;
					}
				}

				switch (instruction.src2->kind) {
					case OP_SYMBOL: {
						printf(" %s\n", instruction.src2->value.sym->name);
						break;
					}

					case OP_SUBTYPE_STR:
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MODULO:
					case OP_STORE: {
						printf(" %s\n", instruction.src2->value.label_name);
						break;
					}

					case OP_INT_LITERAL: {
						printf(" %s\n", instruction.src2->value.int_val);
						break;
					}
				}
				break;
			}

			case ASSIGN: {
				printf("\tASSIGN");
				if (instruction.dest) {
					switch (instruction.dest->kind) {
						case OP_SYMBOL: {
							printf(" %s, ", instruction.dest->value.sym->name);
							break;
						}
						case OP_ADD:
						case OP_MUL:
						case OP_DIV:
						case OP_MODULO: 
						case OP_STORE: {
							printf(" %s, ", instruction.dest->value.label_name);
							break;
						}

						default: break; 
					}
				}

				if (instruction.src2) {
					switch (instruction.src2->kind) {
						case OP_SYMBOL: {
							printf("%s\n", instruction.src2->value.sym->name);
							break;
						}

						case OP_ADD:
						case OP_SUB:
						case OP_MUL:
						case OP_DIV:
						case OP_MODULO:
						case OP_STORE: 
						case OP_RETURN: {
							printf("%s\n", instruction.src2->value.label_name);
							break;
						}
						default: break;
					}
				}
				break;
			}

			case MOV: {
				if (!is_symbol(instruction.dest->kind)) {
					printf("\tMOV %s, %d\n",
						instruction.dest->value.sym->name,
						instruction.immediate_val
					);

				} else {
					printf("\tMOV %s, %d\n",
						instruction.dest->value.label_name,
						instruction.immediate_val
					);
				}
				break;
			}

			case PARAM: {
				printf("\tPARAM %s\n", instruction.dest->value.sym->name);
				printf("\n");
				break;
			}

			case MOV_ARG: {
				if (is_symbol(instruction.dest->kind)) {
					printf("\tMOVE_ARG_TO_REG(%s)\n", instruction.dest->value.label_name);
				} else if (!is_symbol(instruction.dest->kind)) {
					printf("\tMOV_ARG_TO_REG(%s)\n", instruction.dest->value.sym->name);
				}
				break;
			}

			case DEREFERENCE: {
				printf("\tDEREFERENCE %s,", instruction.dest->value.label_name);
				switch (instruction.src1->kind) {
					case OP_INT_LITERAL: {
						printf(" %d\n", instruction.src1->value.int_val);
						break;
					}
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MODULO:
					case OP_STORE: {
						printf(" %s\n", instruction.src1->value.label_name);
						break;
					}
				}
				break;
			}

			case STORE: {
				printf("\tSTORE %s,", instruction.dest->value.label_name);
				switch (instruction.src1->kind) {
					case OP_INT_LITERAL: {
						printf(" %d\n", instruction.src1->value.int_val);
						break;
					}
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_STORE:
					case OP_MODULO: {
						printf(" %s\n", instruction.src1->value.label_name);
						break;
					}
				}
				break;
			}

			case CALL: {
				printf("\tcall %s\n", instruction.dest->value.sym->name);
				break;
			}
		}

	}
}

void create_idealized_asm_for_block(CompilerContext* ctx, BasicBlock* block) {
	if (!block || (block && !block->instructions)) return;

	for (int i = 0; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		if (!tac || (tac && tac->handled)) continue;

		switch (tac->type) {
			case TAC_STORE: {
				IdealizedASMInstruction store_instruction = create_idealized_asm_instruction(
					STORE,
					NULL,
					tac->result,
					tac->op1,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, store_instruction);
				break;
			}

			case TAC_DEREFERENCE_AND_ASSIGN: {
				IdealizedASMInstruction dereference_assign_instruction = create_idealized_asm_instruction(
					ASSIGN,
					NULL,
					tac->result,
					NULL,
					tac->op1,
					-1
				);
				store_idealized_asm_instruction(ctx, dereference_assign_instruction);
				break;
			}

			case TAC_DEREFERENCE: {
				IdealizedASMInstruction dereference_instruction = create_idealized_asm_instruction(
					DEREFERENCE,
					NULL,
					tac->result,
					tac->op1,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, dereference_instruction);
				break;
			}

			case TAC_LABEL: {
				IdealizedASMInstruction label = create_idealized_asm_instruction(
					LABEL,
					NULL,
					tac->result,
					NULL,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, label);
				break;
			}

			case TAC_GOTO: {
				IdealizedASMInstruction jump_instruction = create_idealized_asm_instruction(
					JMP,
					NULL,
					tac->result,
					NULL,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, jump_instruction);
				break;
			}

			case TAC_MODULO:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV:
			case TAC_ADD: {
				IdealizedASMInstruction arithmetic_instruction = create_idealized_asm_instruction(
					get_op_code(tac),
					NULL,
					tac->result,
					tac->op1,
					tac->op2,
					-1
				);
				store_idealized_asm_instruction(ctx, arithmetic_instruction);
				break;
			}

			case TAC_CHAR:
			case TAC_BOOL:
			case TAC_INTEGER: {
				IdealizedASMInstruction mov_int_instruction = create_idealized_asm_instruction(
					MOV, 
					NULL, 
					tac->result, 
					NULL, 
					NULL,
					tac->op1->value.int_val
				);
				store_idealized_asm_instruction(ctx, mov_int_instruction);
				break;
			}

			case TAC_ASSIGNMENT: {
				switch (tac->result->kind) {
					case OP_STORE:
					case OP_SYMBOL: {
						IdealizedASMInstruction assign_instruction = create_idealized_asm_instruction(
							ASSIGN,
							NULL,
							tac->result,
							NULL,
							tac->op2,
							-1
						);
						store_idealized_asm_instruction(ctx, assign_instruction);
						break;
					}
				}
				break;
			}

			case TAC_PARAM: {
				IdealizedASMInstruction param = create_idealized_asm_instruction(
					PARAM,
					NULL,
					tac->op1,
					NULL,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, param);
				break;
			}

			case TAC_ARG: {
				IdealizedASMInstruction mov_arg_instruction = create_idealized_asm_instruction(
					MOV_ARG,
					NULL,
					tac->op1,
					NULL,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, mov_arg_instruction);
				break;
			}

			case TAC_CALL: {
				IdealizedASMInstruction call_instruction = create_idealized_asm_instruction(
					CALL,
					NULL,
					tac->result,
					NULL,
					NULL,
					-1
				);
				store_idealized_asm_instruction(ctx, call_instruction);
				break;
			}

			// case TAC_NOT: {
			// 	Operand* jmp_op = NULL;
			// 	if (i + 1 < block->num_instructions) {
			// 		TACInstruction* next_instruction = block->instructions[i + 1];

			// 		if (next_instruction) {
			// 			switch (next_instruction->type) {
			// 				case TAC_IF_FALSE: {
			// 					next_instruction->handled = true;
			// 					jmp_op = next_instruction->op2;
			// 				}
			// 			}
			// 		}
			// 	}
			// 	break;
			// }

			case TAC_LOGICAL_OR: {
				Operand* jmp_op = NULL;
				if (i + 1 < block->num_instructions) {
					TACInstruction* next_instruction = block->instructions[i + 1];

					if (next_instruction) {
						switch (next_instruction->type) {
							case TAC_IF_FALSE: {
								next_instruction->handled = true;
								jmp_op = next_instruction->op2;
							}
						}
					}

					if (jmp_op) {
						OperandValue comparison_val = {.int_val = 1};
						Operand* comparison_op = create_operand(ctx, OP_INT_LITERAL, comparison_val);

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op1,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								jmp_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op2,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								jmp_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}

						{

						}
					} else {
						OperandValue true_val = {.label_name = generate_jmp_label(ctx, FALSE)};
						Operand* true_op =  create_operand(ctx, OP_LABEL, true_val);

						OperandValue end_val = {.label_name = generate_jmp_label(ctx, END)}; 
						Operand* end_op = create_operand(ctx, OP_LABEL, end_val);

						OperandValue comparison_val = {.int_val = 1};
						Operand* comparison_op = create_operand(ctx, OP_INT_LITERAL, comparison_val);

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op1,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								true_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);
						}

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op2,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								true_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}
					
						{
							OperandValue result_false_val = {.int_val = 0};
							Operand* result_false_op = create_operand(ctx, OP_INT_LITERAL, result_false_val);
							IdealizedASMInstruction store_instruction = create_idealized_asm_instruction(
								STORE,
								NULL,
								tac->result,
								result_false_op,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, store_instruction);

							IdealizedASMInstruction unconditional_jmp = create_idealized_asm_instruction(
								JMP,
								NULL,
								end_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, unconditional_jmp);
						}

						{
							IdealizedASMInstruction false_label = create_idealized_asm_instruction(
								LABEL,
								NULL,
								true_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, false_label);

							IdealizedASMInstruction store_instruction = create_idealized_asm_instruction(
								STORE,
								NULL,
								tac->result,
								comparison_op,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, store_instruction);
						}
					}
				}
			} 

			case TAC_LOGICAL_AND: {
				Operand* jmp_op = NULL;
				if (i + 1 < block->num_instructions) {
					TACInstruction* next_instruction = block->instructions[i + 1];

					if (next_instruction) {
						switch (next_instruction->type) {
							case TAC_IF_FALSE: {
								next_instruction->handled = true;
								jmp_op = next_instruction->op2;
								break;
							}
						}						
					}

					if (jmp_op) {
						OperandValue comparison_val = {.int_val = 0};
						Operand* comparison_op = create_operand(ctx, OP_INT_LITERAL, comparison_val);

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op1,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								jmp_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op2,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								jmp_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}

					} else {
						OperandValue false_val = {.label_name = generate_jmp_label(ctx, FALSE)};
						Operand* false_op =  create_operand(ctx, OP_LABEL, false_val);

						OperandValue end_val = {.label_name = generate_jmp_label(ctx, END)}; 
						Operand* end_op = create_operand(ctx, OP_LABEL, end_val);

						OperandValue comparison_val = {.int_val = 0};
						Operand* comparison_op = create_operand(ctx, OP_INT_LITERAL, comparison_val);

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op1,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								false_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);
						}

						{
							IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
								CMP,
								NULL,
								NULL,
								tac->op2,
								comparison_op,
								-1
							);
							store_idealized_asm_instruction(ctx, cmp_instruction);

							IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
								JMP_E,
								NULL,
								false_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, jmp_instruction);
						}
					
						{
							OperandValue result_true_val = {.int_val = 1};
							Operand* result_true_op = create_operand(ctx, OP_INT_LITERAL, result_true_val);
							IdealizedASMInstruction store_instruction = create_idealized_asm_instruction(
								STORE,
								NULL,
								tac->result,
								result_true_op,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, store_instruction);

							IdealizedASMInstruction unconditional_jmp = create_idealized_asm_instruction(
								JMP,
								NULL,
								end_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, unconditional_jmp);
						}

						{
							IdealizedASMInstruction false_label = create_idealized_asm_instruction(
								LABEL,
								NULL,
								false_op,
								NULL,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, false_label);

							IdealizedASMInstruction store_instruction = create_idealized_asm_instruction(
								STORE,
								NULL,
								tac->result,
								comparison_op,
								NULL,
								-1
							);
							store_idealized_asm_instruction(ctx, store_instruction);
						}
					}
				} 
				break;
			}

			case TAC_NOT_EQUAL:
			case TAC_LESS:
			case TAC_LESS_EQUAL:
			case TAC_GREATER:
			case TAC_GREATER_EQUAL:
			case TAC_EQUAL: {
				Operand* jmp_op = NULL;
				if (i + 1 < block->num_instructions) {
					TACInstruction* next_instruction = block->instructions[i + 1];

					if (next_instruction) {
						switch (next_instruction->type) {
							case TAC_IF_FALSE: {
								next_instruction->handled = true;
								jmp_op = next_instruction->op2;
								break;
							}
						}						
					}

					if (jmp_op) {
						IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
							CMP,
							NULL,
							NULL,
							tac->op1,
							tac->op2,
							-1
						);
						store_idealized_asm_instruction(ctx, cmp_instruction);

						IdealizedASMInstruction jmp_true_instruction = create_idealized_asm_instruction(
							get_op_code(tac),
							NULL,
							jmp_op,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_true_instruction);	
					
					} else {
						OperandValue false_val = {.label_name = generate_jmp_label(ctx, FALSE)};
						Operand* false_op =  create_operand(ctx, OP_LABEL, false_val);

						OperandValue end_val = {.label_name = generate_jmp_label(ctx, END)}; 
						Operand* end_op = create_operand(ctx, OP_LABEL, end_val);

						IdealizedASMInstruction cmp_instruction = create_idealized_asm_instruction(
							CMP,
							NULL,
							NULL,
							tac->op1,
							tac->op2,
							-1
						);
						store_idealized_asm_instruction(ctx, cmp_instruction);

						IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
							get_op_code(tac),
							NULL,
							false_op,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_instruction);

						OperandValue int_true_val = {.int_val = 1};
						Operand* int_true_op = create_operand(ctx, OP_INT_LITERAL, int_true_val);	
						IdealizedASMInstruction tac_store_true = create_idealized_asm_instruction(
							STORE,
							NULL,
							tac->result,
							int_true_op,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, tac_store_true);

						IdealizedASMInstruction unconditional_jmp = create_idealized_asm_instruction(
							JMP,
							NULL,
							end_op,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, unconditional_jmp);

						IdealizedASMInstruction false_label = create_idealized_asm_instruction(
							LABEL,
							NULL,
							false_op,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, false_label);


						OperandValue int_false_val = {.int_val = 0};
						Operand* int_false_op = create_operand(ctx, OP_INT_LITERAL, int_false_val);
						IdealizedASMInstruction tac_store_false = create_idealized_asm_instruction(
							STORE,
							NULL,
							tac->result,
							int_false_op,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, tac_store_false);

						IdealizedASMInstruction end_label = create_idealized_asm_instruction(
							LABEL,
							NULL,
							end_op,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, end_label);
					}
				}
				break;
			}

			case TAC_IF_FALSE: {
				switch (tac->op1->kind) {
					case OP_NOT_EQUAL: {
						IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
							JMP_E,
							NULL,
							tac->op2,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_instruction);
						break;
					}

					case OP_EQUAL: {
						IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
							JMP_NE,
							NULL,
							tac->op2,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_instruction);
						break;
					}

					case OP_GREATER_EQUAL:
					case OP_GREATER: {
						IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
							JMP_L,
							NULL,
							tac->op2,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_instruction);
						break;
					}

					case OP_LESS_EQUAL:
					case OP_LESS: {
						IdealizedASMInstruction jmp_instruction = create_idealized_asm_instruction(
							JMP_G,
							NULL,
							tac->op2,
							NULL,
							NULL,
							-1
						);
						store_idealized_asm_instruction(ctx, jmp_instruction);
						break;
					}

					case OP_LOGICAL_AND: {
						// jump ne to 1
						break;
					}

					case OP_LOGICAL_OR: {
						//jump equal to 0
						break;
					}

				}
				break;
			}

			case TAC_RETURN: {
				if (tac->op1) {
					printf("we have tac op1 in return with type %d\n", tac->op1->kind);
					IdealizedASMInstruction return_instruction = create_idealized_asm_instruction(
						RETURN,
						NULL,
						tac->op1,
						NULL,
						NULL,
						-1
					);
					store_idealized_asm_instruction(ctx, return_instruction);
				} else {
					IdealizedASMInstruction return_instruction = create_idealized_asm_instruction(
						RETURN,
						NULL,
						NULL,
						NULL,
						NULL,
						-1
					);
					store_idealized_asm_instruction(ctx, return_instruction);
				}
				break;
			}
		}
	}
}

void create_idealized_asm(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		
		IdealizedASMInstruction function_name = create_idealized_asm_instruction(
			LABEL, 
			info->symbol, 
			NULL, 
			NULL, 
			NULL,
			-1
		);
		store_idealized_asm_instruction(ctx, function_name);
		
		CFG* cfg = info->cfg;
		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* current_block = cfg->all_blocks[j];
			create_idealized_asm_for_block(ctx, current_block);
		}
	}
}


void reg_alloc(CompilerContext* ctx, FunctionList* function_list) {
	if (!function_list) return;

	asm_instruction_list = create_idealized_asm_instruction_list(ctx);
	if (!asm_instruction_list.instructions) return;

	create_idealized_asm(ctx, function_list);
	emit_idealized_asm_instructions();

	// create_interference_graph();
}