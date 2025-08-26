#include "regalloc.h"
#include "assert.h"
#include "IR/tac.h"

void find_new_register(InterferenceBundle* bundle, int* remaining_registers, Operand* op) {
	for (int i = 0; i < NUM_REGISTERS; i++) {
		if (remaining_registers[i] == -1) continue;
		bool can_use = true;
	
		for (int j = 0; j < bundle->interferes_with->size; j++) {
			Operand* interfering_op = bundle->interferes_with->elements[j];
			if (interfering_op->permanent_frame_position) continue;
			if (interfering_op->assigned_register == remaining_registers[i]) {
				can_use = false;
				break;
			}
		}

		if (can_use) {
			switch (op->kind) {
				case OP_SYMBOL: {
					printf("\033[32m%s found register %d\033[0m\n", op->value.sym->name, remaining_registers[i]);
					break;
				}

				default: {
					printf("\033[32m%s found register %d\033[0m\n", op->value.label_name, remaining_registers[i]);
					break;
				}
			}
			op->assigned_register = remaining_registers[i];
			break;
		} else {
			switch (op->kind) {
				case OP_SYMBOL: {
					printf("\033[31m%s could not find register\033[0m\n", op->value.sym->name);
					break;
				}

				default: {
					printf("\033[31m%s could not find register\033[0m\n", op->value.label_name);
					break;
				}
			}
		}
	}
}

bool is_restricted(int* restricted_regs, int restricted_regs_count, int reg) {
	for (int i = 0; i < restricted_regs_count; i++) {
		if (restricted_regs[i] == reg) {
			return true;
		}
	}
	return false;
}

void application_binary_interface(TACInstruction* instruction, int* arg_index, int* param_index) {
	if (instruction) {
		switch (instruction->kind) {
			case TAC_ARG: {
				if (instruction->op1) {
					if (*arg_index < 6) {
						instruction->result->assigned_register = ARG_OFFSET + *arg_index;
						(*arg_index)++;
					} else {
						instruction->op1->permanent_frame_position = true;
					}

				}
				break;
			}

			case TAC_PARAM: {
				if (*param_index < 6) {
					switch (instruction->op1->kind) {
						case OP_SYMBOL: {
							printf("Param '%s' with address: %p assigned register=%d\n", instruction->op1->value.sym->name, (void*)instruction->op1, ARG_OFFSET + *param_index);
							break;
						}
						default: {
							printf("Param '%s' with address: %p assigned address=%d\n", instruction->op1->value.label_name, (void*)instruction->op1, ARG_OFFSET + *param_index);
							break;
						}
					}
					instruction->op1->assigned_register = ARG_OFFSET + *param_index;
					(*param_index)++;
				} else {
					instruction->op1->permanent_frame_position = true;
				}
				break;
			}

			case TAC_CALL: {
				*arg_index = 0;
				break;
			}

			// case TAC_MODULO:
			// case TAC_DIV: {
			// 	if (instruction->op1) {
			// 		instruction->op1->assigned_register = 0;
			// 	}
			// 	break;
			// }

			case TAC_RETURN: {
				if (instruction->result && instruction->op1) {
					instruction->result->assigned_register = 0;
				}
				break;
			}

			default:
				break;
		}
	}
}

void pre_color_nodes(FunctionInfo* info) {
	CFG* cfg = info->cfg;
	int arg_index = 0;
	int param_index = 0;

	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i];
		for (int j = 0; j < block->num_instructions; j++) {
			TACInstruction* instruction = block->instructions[j];
			application_binary_interface(instruction, &arg_index, &param_index);
		}
	}
}

int* get_remaining_registers(CompilerContext* ctx, int* restricted_regs, int restricted_regs_count) {
	int* remaining_registers = arena_allocate(ctx->codegen_arena, sizeof(int) * NUM_REGISTERS);
	if (!remaining_registers) return NULL;

	for (int i = 0; i < NUM_REGISTERS; i++) {
		remaining_registers[i] = i;
	}

	for (int i = 0; i < restricted_regs_count; i++) {
		for (int j = 0; j < NUM_REGISTERS; j++) {
			if (restricted_regs[i] == remaining_registers[j]) {
				remaining_registers[j] = -1;
			}
		}
	}
	return remaining_registers;
}

void color_interference_graphs(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		printf("\nAbout to color interference graph for function \033[32m%s\033[0m\n", info->symbol->name);
		InterferenceGraph* graph = info->graph;

		pre_color_nodes(info);

		for (int j = 0; j < graph->size; j++) {
			InterferenceBundle* bundle = graph->bundles[j];
			Operand* op = bundle->operand;
			if (strcmp(info->symbol->name, "classify_numbers") == 0) {
				switch (op->kind) {
					case OP_SYMBOL: {
						printf("Processing '%s' with address: %p\n", op->value.sym->name, (void*)op);
						break;
					}
					default: {
						printf("Processing '%s'\n", op->value.label_name);
						break;
					}
				}
			}
			// pre colored
			if (op->assigned_register != -1 || op->permanent_frame_position) {
				switch (op->kind) {
					case OP_SYMBOL: {
						printf("%s was precolored\n", op->value.sym->name);
						if (op->permanent_frame_position) {
							printf("%s has permanent_frame_position\n", op->value.sym->name);
						} else if (op->assigned_register != -1) {
							printf("%s has assigned register=%d\n", op->value.sym->name, op->assigned_register);
						}
						break;
					}
					default: {
						printf("%s was precolored\n", op->value.label_name);
						if (op->permanent_frame_position) {
							printf("%s has permanent_frame_position\n", op->value.label_name);
						} else if (op->assigned_register != -1) {
							printf("%s has assigned register=%d\n",op->value.label_name, op->assigned_register);
						}
						break;
					}
				}

				continue;
			}
			for (int reg = 0; reg < NUM_REGISTERS; reg++) {
				bool can_use = true;

				for (int k = 0; k < bundle->interferes_with->size; k++) {
					Operand* interfering_op = bundle->interferes_with->elements[k];

					if (interfering_op->permanent_frame_position) continue;
					if (interfering_op->assigned_register == reg) {
						can_use = false;
						break;
					}
				}

				if (can_use) {
					if (op->restricted && is_restricted(op->restricted_regs, op->restricted_regs_count, reg)) {
						int* remaining_registers = get_remaining_registers(ctx, op->restricted_regs, op->restricted_regs_count);
						assert(remaining_registers);
						find_new_register(bundle, remaining_registers, op);	
					} else {
						op->assigned_register = reg;
						switch (op->kind) {
							case OP_SYMBOL: {
								printf("'%s' has been assigned register=%d\n", op->value.sym->name, reg);
								break;
							}
							default: {
								printf("'%s' has been assigned register=%d\n", op->value.label_name, reg);
								break;
							}
						}
					}
					break;
				}
			}

			if (op->assigned_register == -1) {
				op->permanent_frame_position = true;
			}
		}
	}
}

void reg_alloc(CompilerContext* ctx, FunctionList* function_list) {
	if (!function_list) return;

	color_interference_graphs(ctx, function_list);
	check_regs(function_list);
}

void check_regs(FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;
		printf("\nFunction: \033[32m%s\033[0m\n", info->symbol->name);
		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* block = cfg->all_blocks[j];
			for (int k = 0; k < block->num_instructions; k++) {
				TACInstruction* tac = block->instructions[k];
				if (tac) {
					switch (tac->kind) {
						case TAC_ASSIGNMENT: {
							if (tac->result) {
								if (tac->result->assigned_register != -1) {
									switch (tac->result->kind) {
										case OP_SYMBOL: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->result->value.sym->name, tac->result->assigned_register);
											break;
										}

										case OP_STORE: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->result->value.label_name, tac->result->assigned_register);
											break;
										}
									}
								} else {
									switch (tac->result->kind) {
										case OP_SYMBOL: {
											printf("'%s' needs spill\n", tac->result->value.sym->name);
											break;
										}

										case OP_STORE: {
											printf("'%s' needs spill\n", tac->result->value.label_name);
											break;
										}
									}
								}
							}
							break;
						}

						case TAC_RETURN: {
							if (tac->op1) {
								if (tac->op1->assigned_register != -1) {
									switch (tac->op1->kind) {
										case OP_SYMBOL: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->op1->value.sym->name, tac->op1->assigned_register);
											break;
										}

										default: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->op1->value.label_name, tac->op1->assigned_register);
											break;
										}
									}
								}
							}
							break;
						}

						case TAC_DEREFERENCE:
						case TAC_DEREFERENCE_AND_ASSIGN:
						case TAC_STORE:
						case TAC_UNARY_SUB:
						case TAC_UNARY_ADD:
						case TAC_NOT:
						case TAC_CHAR:
						case TAC_BOOL:
						case TAC_INTEGER:
						case TAC_ADD:
						case TAC_SUB:
						case TAC_MUL:
						case TAC_DIV:
						case TAC_MODULO:
						case TAC_LOGICAL_AND:
						case TAC_LOGICAL_OR:
						case TAC_LESS_EQUAL:
						case TAC_GREATER_EQUAL:
						case TAC_GREATER:
						case TAC_LESS:
						case TAC_EQUAL:
						case TAC_NOT_EQUAL: {
							if (tac->result) {
								if (tac->result->assigned_register != -1) {
									printf("'%s' has register \033[32m%d\033[0m\n", tac->result->value.label_name, tac->result->assigned_register);
								} else {
									printf("'%s' needs spill\n", tac->result->value.label_name);
								}
							}
							break;
						}

						case TAC_PARAM:
						case TAC_ARG: {
							if (tac->op1) {
								if (tac->op1->assigned_register != -1) {
									switch (tac->op1->kind) {
										case OP_SYMBOL: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->op1->value.sym->name, tac->op1->assigned_register);
											break;
										}

										case OP_STORE:
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
										case OP_LOGICAL_AND:
										case OP_LOGICAL_OR:
										case OP_MODULO:
										case OP_NOT: {
											printf("'%s' has register \033[32m%d\033[0m\n", tac->op1->value.label_name, tac->op1->assigned_register);
											break;
										}
									}
								} else {
									switch (tac->op1->kind) {
										case OP_SYMBOL: {
											printf("'%s' needs spill\n", tac->op1->value.sym->name);
											break;
										}

										case OP_STORE:
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
										case OP_LOGICAL_AND:
										case OP_LOGICAL_OR:
										case OP_MODULO:
										case OP_NOT: {
											printf("'%s' needs spill\n", tac->op1->value.label_name);
											break;
										}
									}
								}
							}
							break;
						}

						case TAC_CALL:
						case TAC_LABEL:
						case TAC_IF_FALSE:
						case TAC_GOTO: {
							break;
						}

						default: {
							printf("missing tac type is \033[31m%d\033[0m\n", tac->kind);
							break;
						}
					}
				}
			}
		}
	}
}