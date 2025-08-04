#include "regalloc.h"

void ensure_calling_convention(TACInstruction* instruction, int* arg_index, int* param_index) {
	if (instruction) {
		switch (instruction->kind) {
			case TAC_ARG: {
				if (*arg_index < 6) {
					instruction->op1->assigned_register = ARG_OFFSET + *arg_index;
					(*arg_index)++;
				} else {
					instruction->op1->needs_spill = true;
				}
				break;
			}

			case TAC_PARAM: {
				if (*param_index < 6) {
					instruction->op1->assigned_register = ARG_OFFSET + *param_index;
					(*param_index)++;
				} else {
					instruction->op1->needs_spill = true;
				}
				break;
			}

			case TAC_CALL: {
				*arg_index = 0;
				break;
			}

			case TAC_ASSIGNMENT: {
				if (!instruction->result || !instruction->op2) break;
				if (instruction->result->kind != OP_STORE) break;
				if (instruction->op2->kind != OP_RETURN) break;
				instruction->op2->assigned_register = 0;
				break;
			}

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
			ensure_calling_convention(instruction, &arg_index, &param_index);
		}
	}
}

void color_interference_graphs(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		InterferenceGraph* graph = info->graph;

		pre_color_nodes(info);

		for (int j = 0; j < graph->size; j++) {
			InterferenceBundle* bundle = graph->bundles[j];
			Operand* op = bundle->operand;

			// from interference graph construction
			if (op->live_on_exit == 0) continue;
			if (op->precedes_conditional) continue;

			// pre colored
			if (op->assigned_register != -1 || op->needs_spill) continue;

			for (int reg = 0; reg < NUM_REGISTERS; reg++) {
				bool can_use = true;

				for (int k = 0; k < bundle->interferes_with->size; k++) {
					Operand* interfering_op = bundle->interferes_with->elements[k];

					if (interfering_op->needs_spill) continue;
					if (interfering_op->precedes_conditional) continue;
					if (interfering_op->assigned_register == reg) {
						can_use = false;
						break;
					}
				}

				if (can_use) {
					op->assigned_register = reg;
					break;
				}
			}

			if (op->assigned_register == -1) {
				op->needs_spill = true;
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