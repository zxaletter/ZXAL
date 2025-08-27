#include "codegen.h"
#include "assert.h"

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

 	int length = strlen(buffer);
	char* jmp_label = arena_allocate(ctx->codegen_arena, length + 1);
	if (!jmp_label) return NULL;

	strncpy(jmp_label, buffer, length);
	jmp_label[length] = '\0';
	return jmp_label;
}

char* get_op_code(tac_t type) {
	switch (type) {
		case TAC_NOT_EQUAL: return "je";
		case TAC_EQUAL: return "jne";
		
		case TAC_LESS:
		case TAC_LESS_EQUAL: return "jge";

 		case TAC_GREATER:
 		case TAC_GREATER_EQUAL: return "jle";
	}
}

void generate_corresponding_jump(ASMWriter* writer, tac_t kind, char* jmp_label) {
	char buffer[32];
	switch (kind) {
		case TAC_LESS:
		case TAC_LESS_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjge %s", jmp_label);
			break;
		}

		case TAC_GREATER:
		case TAC_GREATER_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjle %s", jmp_label);
			break;
		}

		case TAC_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tjne %s", jmp_label);
			break;
		}

		case TAC_NOT_EQUAL: {
			snprintf(buffer, sizeof(buffer), "\tje %s", jmp_label);
			break;
		}
	}
	write_asm_to_file(writer, buffer);
}

ArgumentList* find_arg_list(BasicBlock* block, TACInstruction* tac) {
	for (int i = 0; i < block->sargs->call_instr_count; i++) {
		ArgumentList* current = block->sargs->lists[i];
		CallInstruction* call_instr = current->c_instr;
		if ((call_instr && call_instr->instr_id == tac->id )&&
			(strcmp(call_instr->func_name, tac->result->value.sym->name) == 0)) {
			return current;
		}
	}
	return NULL;
}

void emit_reloads(ASMWriter* writer, ReloadBundle* bundle) {
	if (bundle) {
		char buffer[30];
		for (int i = 0; i < bundle->size; i++) {
			Reload* r = &bundle->reloads[i];
			switch (r->direction) {
				case REG_TO_FRAME: {
					snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s", r->frame_byte_offset, registers[r->assigned_register]);
					break;
				}
				case FRAME_TO_REG: {
					snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]", registers[r->assigned_register], r->frame_byte_offset);
					break;
				}
				case POP_FROM_STACK: {
					snprintf(buffer, sizeof(buffer), "\tpop %s", registers[r->assigned_register]);
					break;
				}
			}
			write_asm_to_file(writer, buffer);
		}
	}
}

void emit_spills(ASMWriter* writer, SpillBundle* bundle) {
	if (bundle) {
		char buffer[30];
		for (int i = 0; i < bundle->size; i++) {
			Spill* s = &bundle->spills[i];
			switch (s->direction) {
				case REG_TO_FRAME: {
					snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s", s->frame_byte_offset, registers[s->assigned_register]);
					break;
				}

				case FRAME_TO_REG: {
					snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]", registers[s->assigned_register], s->frame_byte_offset);
					break;
				}

				case PUSH_TO_STACK: {
					snprintf(buffer, sizeof(buffer), "\tpush %s", registers[s->assigned_register]);
					break;
				}
			}
			write_asm_to_file(writer, buffer); 
		}
	}
}

SpillBundle* create_spill_bundle(CompilerContext* ctx, int size) {
	SpillBundle* bundle = arena_allocate(ctx->codegen_arena, sizeof(SpillBundle));
	if (!bundle) return NULL;

	bundle->size = size;
	bundle->spills = arena_allocate(ctx->codegen_arena, sizeof(Spill) * bundle->size);
	if (!bundle->spills) return NULL;
	return bundle;
}

SpillBundle* gather_matching_spills(CompilerContext* ctx, SpillSchedule* spill_schedule, int push_index) {
	int spill_count = 0;
	for (int i = 0; i < spill_schedule->size; i++) {
		if (spill_schedule->spills[i].push_index == push_index) {
			spill_count++;
		}
	}

	SpillBundle* bundle = NULL;
	if (spill_count > 0) {
		bundle = create_spill_bundle(ctx, spill_count);
		assert(bundle);
		
		int count = 0;
		for (int i = 0; i < spill_schedule->size && count < bundle->size; i++) {
			if (spill_schedule->spills[i].push_index == push_index) {
				bundle->spills[count] = spill_schedule->spills[i];
				count++;
			}
		}
	}
	return bundle;
}

ReloadBundle* gather_matching_reloads(CompilerContext* ctx, ReloadSchedule* reload_schedule, int pop_index) {
	int reload_count = 0;
	for (int i = 0; i < reload_schedule->size; i++) {
		if (reload_schedule->reloads[i].pop_index == pop_index) {
			reload_count++;
		}
	}

	ReloadBundle* bundle = NULL;
	if (reload_count > 0) {
		bundle = create_reload_bundle(ctx, reload_count);
		assert(bundle);
		
		int count = 0;
		for (int i = reload_schedule->size - 1; i >= 0 && count < bundle->size; i--) {
			if (reload_schedule->reloads[i].pop_index == pop_index) {
				bundle->reloads[count] = reload_schedule->reloads[i];
				count++;
			}
		}
	}
	return bundle;
}

void generate_function_body(CompilerContext* ctx, ASMWriter* writer, FunctionInfo* info) {
	char buffer[100];
	CFG* cfg = info->cfg;
	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i]; 
		for (int j = 0; j < block->num_instructions; j++) {		
			TACInstruction* tac = block->instructions[j];
			if (tac && tac->handled) continue;

			SpillBundle* spill_bundle = gather_matching_spills(ctx, block->spill_schedule, j);
			ReloadBundle* reload_bundle = gather_matching_reloads(ctx, block->reload_schedule, j);

			if (spill_bundle && spill_bundle->size > 0) {
				emit_spills(writer, spill_bundle);
			}
			
			if (reload_bundle && reload_bundle->size > 0) {
				emit_reloads(writer, reload_bundle);
			}
			
			switch (tac->kind) {
				case TAC_BOOL:
				case TAC_CHAR:
				case TAC_INTEGER: {
					if (tac->result->permanent_frame_position) {
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

				case TAC_UNARY_ADD:
					break;

				case TAC_UNARY_SUB: {
					if (tac->op1->permanent_frame_position) {
						snprintf(buffer, sizeof(buffer), "\tneg [rbp - %zu]", tac->op1->frame_byte_offset);
					}else {
						snprintf(buffer, sizeof(buffer), "\tneg %s", registers[tac->op1->assigned_register]);
					}
					write_asm_to_file(writer, buffer);

					if (tac->result->permanent_frame_position) {
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->temp_register]);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->assigned_register]);
						}
					} else {
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
								registers[tac->result->assigned_register],
								tac->op1->frame_byte_offset);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s", 
								registers[tac->result->assigned_register],
								registers[tac->op1->assigned_register]);
						}
					}
					write_asm_to_file(writer, buffer);
					break;
				}

				case TAC_RETURN: {
					if (tac->result) {
						if (tac->op1) {
							bool can_emit = false;
							if (tac->op1->permanent_frame_position) {
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
						
						emit_reloads(writer, block->reload_bundle);

						snprintf(buffer, sizeof(buffer), "\tleave\n\tret");
						write_asm_to_file(writer, buffer);
					} 
					break;
				}

				case TAC_ASSIGNMENT: {
					if (tac->op2->kind == OP_RETURN) {
						if (tac->result->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], rax", tac->result->frame_byte_offset);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov %s, rax", registers[tac->result->assigned_register]);
						}
						write_asm_to_file(writer, buffer);
						break;
					}

					if (tac->result->permanent_frame_position) {
						if (tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op2->temp_register]
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op2->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					} else {
						if (tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
								registers[tac->result->assigned_register],
								tac->op2->frame_byte_offset
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
								registers[tac->result->assigned_register],
								registers[tac->op2->assigned_register]
							);
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
					if (tac->op1->permanent_frame_position && tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], %s",
								tac->op1->frame_byte_offset,
								registers[tac->op2->temp_register]
							);
					} else if (tac->op1->assigned_register != -1 && tac->op2->permanent_frame_position) {
						snprintf(buffer, sizeof(buffer), "\tcmp %s, [rbp - %zu]",
							registers[tac->op1->assigned_register],
							tac->op2->frame_byte_offset
						);
					} else if (tac->op1->permanent_frame_position && tac->op2->assigned_register != -1) {
						snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], %s",
							tac->op1->frame_byte_offset,
							registers[tac->op2->assigned_register]
						);
					} else {
						snprintf(buffer, sizeof(buffer), "\tcmp %s, %s",
							registers[tac->op1->assigned_register],
							registers[tac->op2->assigned_register]
						);
					}
					write_asm_to_file(writer, buffer);
						
					if (j + 1 < block->num_instructions) {
						Operand* jmp_op = NULL;
						TACInstruction* next_tac = block->instructions[j + 1];
						
						switch (next_tac->kind) {
							case TAC_IF_FALSE: {
								jmp_op = next_tac->op2;
								next_tac->handled = true;
								break;
							}
						}

						
						if (jmp_op) {
							generate_corresponding_jump(writer, tac->kind, jmp_op->value.label_name);							
						} else {
							char* label_true = generate_jmp_label(ctx, TRUE);
							char* label_false = generate_jmp_label(ctx, FALSE);
							char* label_end = generate_jmp_label(ctx, END);
							assert(label_true && label_false && label_end);

							char* jmp_instr = get_op_code(tac->kind);
							assert(jmp_instr);

							snprintf(buffer, sizeof(buffer), "\t%s %s", jmp_instr, label_false);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_true);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 1", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], 1", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tjmp %s", label_end);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_false);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 0", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], 0", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_end);
							write_asm_to_file(writer, buffer);
						}
					}
					break;
				}

				case TAC_LOGICAL_OR: {
					if (j + 1 < block->num_instructions) {
						Operand* jmp_op = NULL;
						TACInstruction* next_tac = block->instructions[j + 1];

						switch (next_tac->kind) {
							case TAC_IF_FALSE: {
								jmp_op = next_tac->op2;
								next_tac->handled = true;
								break;
							}
						}

						if (jmp_op) {
							if (tac->op1->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", registers[tac->op1->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 1", tac->op1->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", jmp_op->value.label_name);
							write_asm_to_file(writer, buffer);

							if (tac->op2->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", registers[tac->op1->assigned_register]);								
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 1", tac->op2->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", jmp_op->value.label_name);
							write_asm_to_file(writer, buffer);
						} else {
							char* label_true = generate_jmp_label(ctx, TRUE);
							char* label_false = generate_jmp_label(ctx, FALSE);
							char* label_end = generate_jmp_label(ctx, END);
							assert(label_true && label_false && label_end);

							if (tac->op1->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", registers[tac->op1->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 1", tac->op1->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", label_true);
							write_asm_to_file(writer, buffer);

							if (tac->op2->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 1", registers[tac->op2->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 1", tac->op2->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", label_true);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_false);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 0", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], 0", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tjmp %s", label_end);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_true);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 1", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu]", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_end);
							write_asm_to_file(writer, buffer);
						}
					} 
					break;
				}

				case TAC_LOGICAL_AND: {
					if (j + 1 < block->num_instructions) {
						Operand* jmp_op = NULL;
						TACInstruction* next_tac = block->instructions[j + 1];

						switch (next_tac->kind) {
							case TAC_IF_FALSE: {
								jmp_op = next_tac->op2;
								next_tac->handled = true;
								break;
							}
						}

						if (jmp_op) {
							if (tac->op1->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", registers[tac->op1->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp -  %zu], 0", tac->op1->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", jmp_op->value.label_name);
							write_asm_to_file(writer, buffer);

							if (tac->op2->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", registers[tac->op2->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 0", tac->op2->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", jmp_op->value.label_name);
							write_asm_to_file(writer, buffer);
						} else {
							char* label_false = generate_jmp_label(ctx, FALSE);
							char* label_true = generate_jmp_label(ctx, TRUE);
							char* label_end = generate_jmp_label(ctx, END);
							assert(label_false && label_true && label_end);

							if (tac->op1->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", registers[tac->op1->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 0", tac->op1->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", label_false);
							write_asm_to_file(writer, buffer);

							if (tac->op2->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tcmp %s, 0", registers[tac->op2->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tcmp [rbp - %zu], 0", tac->op2->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tje %s", label_false);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_true);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 1", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], 1", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "\tjmp %s", label_end);
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_false);
							write_asm_to_file(writer, buffer);

							if (tac->result->assigned_register != -1) {
								snprintf(buffer, sizeof(buffer), "\tmov %s, 0", registers[tac->result->assigned_register]);
							} else {
								snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], 0", tac->result->frame_byte_offset);
							}
							write_asm_to_file(writer, buffer);

							snprintf(buffer, sizeof(buffer), "%s:", label_end);
							write_asm_to_file(writer, buffer);
						}
					} 
					break;
				}

				case TAC_ADD:
				case TAC_SUB: {					
					if (tac->op1->permanent_frame_position) {
						assert(tac->op1->temp_register != -1);
						if (tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\t%s %s, [rbp - %zu]",
								operator_to_string(tac->kind),
								registers[tac->op1->temp_register],
								tac->op2->frame_byte_offset
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\t%s %s, %s",
								operator_to_string(tac->kind),
								registers[tac->op1->temp_register],
								registers[tac->op2->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					} else {
						if (tac->op2->permanent_frame_position) {
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
					}
					
					if (tac->result->permanent_frame_position) {
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->temp_register]
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					} else {
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
								registers[tac->result->assigned_register],
								registers[tac->op1->temp_register]
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
								registers[tac->result->assigned_register],
								registers[tac->op1->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					}
					break;
				}

				case TAC_MUL: {
					if (tac->op1->permanent_frame_position) {
						assert(tac->op1->temp_register != -1);
						if (tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\timul %s, [rbp - %zu]", 
								registers[tac->op1->temp_register], 
								tac->op2->frame_byte_offset
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\timul %s, %s",
								registers[tac->op1->temp_register],
								registers[tac->op2->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					} else {
						if (tac->op2->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\timul %s, [rbp - %zu]",
								registers[tac->op1->assigned_register],
								tac->op2->frame_byte_offset
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\timul %s, %s",
								registers[tac->op1->assigned_register],
								registers[tac->op2->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					}
										
					if (tac->result->permanent_frame_position) {	
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->temp_register]
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], %s",
								tac->result->frame_byte_offset,
								registers[tac->op1->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					} else {
						if (tac->op1->permanent_frame_position) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
								registers[tac->result->assigned_register],
								registers[tac->op1->temp_register]
							);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
								registers[tac->result->assigned_register],
								registers[tac->op1->assigned_register]
							);
						}
						write_asm_to_file(writer, buffer);
					}
					break;
				}

				case TAC_MODULO: {
					if (tac->op1->permanent_frame_position) {
						snprintf(buffer, sizeof(buffer), "\tmov rax, [rbp - %zu]", tac->op1->frame_byte_offset);
					} else if (tac->op1->assigned_register != -1) {
						snprintf(buffer, sizeof(buffer), "\tmov rax, %s", registers[tac->op1->assigned_register]);
					}
					write_asm_to_file(writer, buffer);
					
					snprintf(buffer, sizeof(buffer), "\txor rdx, rdx");
					write_asm_to_file(writer, buffer);

					if (tac->op2->assigned_register != -1) {
						snprintf(buffer, sizeof(buffer), "\tdiv %s", registers[tac->op2->assigned_register]);
					} else {
						snprintf(buffer, sizeof(buffer), "\tdiv [rbp - %zu]", tac->op2->frame_byte_offset);
					}
					write_asm_to_file(writer, buffer);

					if (tac->result) {
						if (tac->result->assigned_register != -1) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, rdx", registers[tac->result->assigned_register]);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], rdx", tac->result->frame_byte_offset);
						}
						write_asm_to_file(writer, buffer);
					}
					break;
				}

				case TAC_DIV: {
					if (tac->op1->permanent_frame_position) {
						snprintf(buffer, sizeof(buffer), "\tmov rax, [rbp - %zu]", tac->op1->frame_byte_offset);
					} else if (tac->op1->assigned_register != -1) {
						snprintf(buffer, sizeof(buffer), "\tmov rax, %s", registers[tac->op1->assigned_register]);
					}
					write_asm_to_file(writer, buffer);

					snprintf(buffer, sizeof(buffer), "\txor rdx, rdx");
					write_asm_to_file(writer, buffer);

					if (tac->op2->assigned_register != -1) {
						snprintf(buffer, sizeof(buffer), "\tdiv %s", registers[tac->op2->assigned_register]);
					} else {
						snprintf(buffer, sizeof(buffer), "\tdiv [rbp - %zu]", tac->op2->frame_byte_offset);
					}
					write_asm_to_file(writer, buffer);
					
					if (tac->result) {
						if (tac->result->assigned_register != -1) {
							snprintf(buffer, sizeof(buffer), "\tmov %s, rax", registers[tac->result->assigned_register]);
						} else {
							snprintf(buffer, sizeof(buffer), "\tmov [rbp - %zu], rax", tac->result->frame_byte_offset);
						}
						write_asm_to_file(writer, buffer);
					}
					break;
				}

				case TAC_CALL: {
					ArgumentList* corresponding_list = NULL;
					if (block->sargs) {
						corresponding_list = find_arg_list(block, tac);
					}

					if (corresponding_list) {
						for (int i = 0; i < tac->live_out->size; i++) {
							Operand* op = tac->live_out->elements[i];
							bool caller = is_caller_saved(op->assigned_register);
							if (caller) continue; 
							
						}

						for (int i = 0; i < corresponding_list->size; i++) {
							ArgumentInfo* arg = corresponding_list->args[i];						
							TACInstruction* arg_instr = arg->tac;	

							if (arg->loc == REG) {
								if (arg_instr->op1->assigned_register != -1) {
									snprintf(buffer, sizeof(buffer), "\tmov %s, %s",
										registers[arg_instr->result->assigned_register],
										registers[arg_instr->op1->assigned_register]
									);								
								} else {
									snprintf(buffer, sizeof(buffer), "\tmov %s, [rbp - %zu]",
										registers[arg_instr->result->assigned_register],
										arg_instr->op1->frame_byte_offset
									);
								}
							} else {
								snprintf(buffer, sizeof(buffer), "\tpush [rbp - %zu]",arg_instr->op1->frame_byte_offset);
							}

							write_asm_to_file(writer, buffer);
						}


						snprintf(buffer, sizeof(buffer), "\tcall %s", tac->result->value.sym->name);
						write_asm_to_file(writer, buffer);

						write_asm_to_file(writer, "");

					} else {
						printf("\033[31mNo list\033[0m\n");
					}
					break;
				}
			}		
		}
	}
}

void schedule_callee_reloads(CompilerContext* ctx, SpillSchedule* spill_schedule, ReloadBundle* bundle) {
	for (int i = spill_schedule->size - 1; i >= 0; i--) {
		Spill* s = &spill_schedule->spills[i];
		Reload r = {
			.assigned_register = s->assigned_register,
			.frame_byte_offset = -1,
			.pop_index = -1,
			.block_index = -1,
			.direction = POP_FROM_STACK
		};
		bundle->reloads[bundle->size - i - 1] = r;
	}
}

void add_reload(CompilerContext* ctx, ReloadSchedule* reload_schedule, Reload r) {
	if (reload_schedule->size >= reload_schedule->capacity) {
		int prev_capacity = reload_schedule->capacity;

		reload_schedule->capacity *= 2;
		int new_capacity = reload_schedule->capacity;
		void* new_reloads = arena_reallocate(
			ctx->codegen_arena,
			reload_schedule->reloads,
			prev_capacity * sizeof(Reload),
			new_capacity * sizeof(Reload)
		);

		assert(new_reloads);
		reload_schedule->reloads = new_reloads;
	}
	reload_schedule->reloads[reload_schedule->size++] = r;
}

bool contains_spill(SpillSchedule* schedule, Spill s) {
	if (!schedule) return false;

	if (s.assigned_register != -1) {
		for (int i = 0; i < schedule->size; i++) {
			if (s.assigned_register == schedule->spills[i].assigned_register) {
				return true;
			}
		}
	}
	return false;
}

void add_spill(CompilerContext* ctx, SpillSchedule* schedule, Spill s) {
	if (schedule->size >= schedule->capacity) {
		int prev_capacity = schedule->capacity;

		schedule->capacity *= 2;
		int new_capacity = schedule->capacity;
		void* new_spills = arena_reallocate(
			ctx->codegen_arena,
			schedule->spills,
			prev_capacity * sizeof(Spill),
			new_capacity * sizeof(Spill)
		);

		assert(new_spills);
		schedule->spills = new_spills;
	}
	schedule->spills[schedule->size++] = s;
}

ReloadBundle* create_reload_bundle(CompilerContext* ctx, int size) {
	ReloadBundle* bundle = arena_allocate(ctx->codegen_arena, sizeof(ReloadBundle));
	if (!bundle) {
		printf("unable to allocate space for bundle\n");
		return NULL;
	}
	bundle->size = size;
	bundle->reloads = arena_allocate(ctx->codegen_arena, sizeof(Reload) * bundle->size);
	if (!bundle->reloads) {
		return NULL;
	}
	return bundle;
}

void schedule_callee_register_spills(CompilerContext* ctx, FunctionList* function_list) {
	for (int k = 0; k < function_list->size; k++) {
		FunctionInfo* info = function_list->infos[k];
		CFG* cfg = info->cfg;

		bool main_function = (strcmp(info->symbol->name, "main") == 0);
		if (main_function) {
			for (int i = 0; i < cfg->num_blocks; i++) {
				BasicBlock* block = cfg->all_blocks[i];
				block->spill_schedule = create_spill_schedule(ctx);
				block->reload_schedule = create_reload_schedule(ctx);
				assert(block->spill_schedule && block->reload_schedule);
			}
		} else {
			bool callee_saved_regs = false;
			for (int i = 0; i < cfg->num_blocks; i++) {
				BasicBlock* block = cfg->all_blocks[i];
				block->spill_schedule = create_spill_schedule(ctx);
				block->reload_schedule = create_reload_schedule(ctx);
				assert(block->spill_schedule && block->reload_schedule);

				for (int j = 0; j < block->num_instructions; j++) {
					TACInstruction* tac = block->instructions[j];
					Operand* op = tac->result;
					if (op) {
						bool save_by_callee = is_callee_saved(op->assigned_register);
						if (save_by_callee) {
							callee_saved_regs = true;
							break;
						}
					}
				}

				if (callee_saved_regs) {
					for (int j = 0; j < block->num_instructions; j++) {
						TACInstruction* tac = block->instructions[j];
						switch (tac->kind) {
							case TAC_INTEGER:
							case TAC_CHAR:
							case TAC_BOOL:
							case TAC_ADD:
							case TAC_SUB:
							case TAC_MUL: 
							case TAC_DIV:
							case TAC_MODULO:
							case TAC_LESS:
							case TAC_GREATER:
							case TAC_LESS_EQUAL:
							case TAC_GREATER_EQUAL:
							case TAC_NOT:
							case TAC_EQUAL:
							case TAC_NOT_EQUAL:
							case TAC_LOGICAL_AND:
							case TAC_LOGICAL_OR:
							case TAC_ASSIGNMENT:
							case TAC_STORE:
							case TAC_DEREFERENCE:
							case TAC_DEREFERENCE_AND_ASSIGN:
							case TAC_UNARY_ADD:
							case TAC_UNARY_SUB: {
								Operand* op = tac->result;
								bool save_by_callee = is_callee_saved(op->assigned_register);
								if (save_by_callee) {
									Spill s = {
										.assigned_register = op->assigned_register,
										.frame_byte_offset = -1,
										.push_index = 0,
										.block_index = 0,
										.direction = PUSH_TO_STACK
									};

									bool have_spill = contains_spill(cfg->all_blocks[0]->spill_schedule, s);
									if (!have_spill) {
										add_spill(ctx, cfg->all_blocks[0]->spill_schedule, s);
									}
								}
								break;
							}

							default:
								break;
						}
					}
				}
			}

			if (callee_saved_regs) {
				for (int i = 0; i < cfg->num_blocks; i++) {
					BasicBlock* block = cfg->all_blocks[i];
					block->reload_bundle = create_reload_bundle(ctx, cfg->all_blocks[0]->spill_schedule->size);
					assert(block->reload_bundle);
					for (int j = 0; j < block->num_instructions; j++) {
						TACInstruction* tac = block->instructions[j];
						switch (tac->kind) {
							case TAC_RETURN: {
								schedule_callee_reloads(ctx, cfg->all_blocks[0]->spill_schedule, block->reload_bundle);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void generate_function_prologue(CompilerContext* ctx, ASMWriter* writer, FunctionInfo* info) {
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

void emit_asm_for_functions(CompilerContext* ctx, ASMWriter* writer, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];

		if (info->symbol) {
			char* func_label = create_function_label(ctx, info->symbol->name);
			write_asm_to_file(writer, func_label);
		}

		collect_args(ctx, function_list);
		generate_function_prologue(ctx, writer, info);
		generate_function_body(ctx, writer, info);
	}
}

int find_corrupting_instr_index(BasicBlock* block, op_check_t type, Operand* op, int start) {
	for (int i = start + 1; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		switch (tac->kind) {
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL: {
				switch (type) {
					case FIRST_OPERAND: {
						bool equal = operands_equal(tac->op1, op);
						if (equal) return i;
						break;
					}

					case SECOND_OPERAND: {
						bool equal = operands_equal(tac->op2, op);
						if (equal) return i;
						break;
					}
				}
				break;
			}
		}
	}
}

int verify_liveness(BasicBlock* block, Operand* op, int start) {
	for (int i = start + 1; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		if (tac->kind == TAC_CALL) {
			bool live_out = contains_operand(tac->live_out, op);
			if (live_out) {
				return i;
			}
		}
	}
	return -1;
}

ReloadSchedule* create_reload_schedule(CompilerContext* ctx) {
	ReloadSchedule* schedule = arena_allocate(ctx->codegen_arena, sizeof(ReloadSchedule));
	if (!schedule) return NULL;

	schedule->size = 0;
	schedule->capacity = INIT_POP_SCHEDULE_CAPACITY;
	schedule->reloads = arena_allocate(ctx->codegen_arena, sizeof(Reload) * schedule->capacity);
	if (!schedule->reloads) return NULL;

	return schedule;
}

SpillSchedule* create_spill_schedule(CompilerContext* ctx) {
	SpillSchedule* s = arena_allocate(ctx->codegen_arena, sizeof(SpillSchedule));
	if (!s) return NULL;

	s->size = 0;
	s->capacity = INIT_SPILL_QUEUE_CAPACITY;
	s->spills = arena_allocate(ctx->codegen_arena, sizeof(Spill) * s->capacity);
	if (!s->spills) return NULL;
	return s;
}

ArgumentInfo* create_arg_info(CompilerContext* ctx, arg_location loc, TACInstruction* tac) {
	ArgumentInfo* arg = arena_allocate(ctx->codegen_arena, sizeof(ArgumentInfo));
	if (!arg) return NULL;

	arg->loc = loc;
	arg->tac = tac;
	return arg;
}

void add_to_arg_list(CompilerContext* ctx, ArgumentList* arg_list, ArgumentInfo* arg) {
	assert(arg_list && arg);

	if (arg_list->size >= arg_list->capacity) {
		int prev_capacity = arg_list->capacity;

		arg_list->capacity *= 2;
		int new_capacity = arg_list->capacity;
		void* new_args = arena_reallocate(
			ctx->codegen_arena,
			arg_list->args,
			prev_capacity * sizeof(ArgumentInfo*),
			new_capacity * sizeof(ArgumentInfo*)
		);

		assert(new_args);
		arg_list->args = new_args;
	}
	arg_list->args[arg_list->size++] = arg;
}

ArgumentList* create_arg_list(CompilerContext* ctx, CallInstruction* call_instr) {
	ArgumentList* list = arena_allocate(ctx->codegen_arena, sizeof(ArgumentList));
	if (!list) return NULL;

	list->size = 0;
	list->capacity = 12;
	list->c_instr = call_instr;
	list->args = arena_allocate(ctx->codegen_arena, sizeof(ArgumentInfo*) * list->capacity);
	if (!list->args) return NULL;
	return list;
}

StructuredArgs* create_sargs(CompilerContext* ctx, CallInstructionList* call_instr_list) {
	StructuredArgs* sargs = arena_allocate(ctx->codegen_arena, sizeof(StructuredArgs));
	if (!sargs) return NULL;

	sargs->call_instr_count = call_instr_list->call_instr_count;
	sargs->lists = arena_allocate(ctx->codegen_arena, sizeof(ArgumentList*) * sargs->call_instr_count);
	if (!sargs->lists) return NULL;

	for (int i = 0; i < sargs->call_instr_count; i++) {
		sargs->lists[i] = create_arg_list(ctx, &call_instr_list->c_instructions[i]);
		if (!sargs->lists[i]) {
			printf("Could not create list at index=%d\n", i);
			return NULL;
		}
	}
	return sargs;
}

CallInstructionList* create_call_instr_list(CompilerContext* ctx, int call_instr_count) {
	CallInstructionList* call_instr_list = arena_allocate(ctx->codegen_arena, sizeof(CallInstructionList));
	if (!call_instr_list) return NULL;

	call_instr_list->call_instr_count = call_instr_count;
	call_instr_list->c_instructions = arena_allocate(ctx->codegen_arena, sizeof(CallInstruction) * call_instr_count);
	if (!call_instr_list->c_instructions) return NULL;

	return call_instr_list;
}

CallInstructionList* populate_call_instr_list(CompilerContext* ctx, BasicBlock* block) {
	int call_instr_count = 0;
	for (int i = 0; i < block->num_instructions; i++) {
		if (block->instructions[i]->kind == TAC_CALL) {
			call_instr_count++;
		}
	}

	int* call_ids = NULL;
	int* call_indices = NULL;
	if (call_instr_count > 0) {
		call_ids = arena_allocate(ctx->codegen_arena, sizeof(int) * call_instr_count);
		call_indices = arena_allocate(ctx->codegen_arena, sizeof(int) * call_instr_count);
		assert(call_ids && call_indices);
	}

	int call_index = 0;
	CallInstructionList* call_instr_list = NULL;
	if (call_ids) {
		for (int i = 0; i < block->num_instructions; i++) {
			if (block->instructions[i]->kind == TAC_CALL) {
				call_ids[call_index] = block->instructions[i]->id;
				call_indices[call_index] = i;
				call_index++;
			}
		}

		call_instr_list = create_call_instr_list(ctx, call_instr_count);
		if (!call_instr_list) return NULL;

		int current_instr = 0; 
		int start = 0;
		int j = 0;
		while (j < block->num_instructions) {
			if (block->instructions[j]->kind == TAC_CALL) {
				CallInstruction call_instr = {
					.func_name = block->instructions[j]->result->value.sym->name,
					.start = start,
					.end = j,
					.instr_id = block->instructions[j]->id
				};
				call_instr_list->c_instructions[current_instr++] = call_instr;
				start = j + 1;
			}
			j++;
		}
	}
	return call_instr_list;
}

bool find_call_instr(BasicBlock* block) {
	for (int i = 0; i < block->num_instructions; i++) {
		if (block->instructions[i]->kind == TAC_CALL) {
			return true;
		}
	}
	return false;
}

void collect_args(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;
		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* block = cfg->all_blocks[j];

			bool has_call_instr = find_call_instr(block);
			if (!has_call_instr) {
				continue;
			} 
			CallInstructionList* call_instr_list = populate_call_instr_list(ctx, block);
			assert(call_instr_list);

			block->sargs = create_sargs(ctx, call_instr_list);
			assert(block->sargs);
			bool processed = false;
			for (int k = 0; k < call_instr_list->call_instr_count; k++) {
				CallInstruction* call_instr = &call_instr_list->c_instructions[k];
				for (int c_start = call_instr->start; c_start < call_instr->end; c_start++) {
					int arg_count = 0;
					
					TACInstruction* tac = block->instructions[c_start];
					if (tac->kind == TAC_ARG) {
						ArgumentInfo* info = NULL;
						if (arg_count < 6) {
							info = create_arg_info(ctx, REG, tac);
							add_to_arg_list(ctx, block->sargs->lists[k], info);
							arg_count++;
						} else {
							if (c_start + 1 < call_instr->end) {
								for (int n = c_start; n < call_instr->end; n++) {
									if (block->instructions[n]->kind == TAC_CALL) {
										for (int start = n - 1; n >= c_start; n--) {
											info = create_arg_info(ctx, STACK, block->instructions[start]);
											add_to_arg_list(ctx, block->sargs->lists[k], info);
										}
										processed = true;
										break;
									}
								}
							}
							break;
						}
					}
				}

				if (processed) {
					continue;
				}
			}
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
	write_asm_to_file(writer, "format ELF64 executable\n");
	// write_asm_to_file(writer, "section '.text'\n");

	// for (int i = 0; i < ctx->global_table->capacity; i++) {
	// 	Symbol* func_symbol = ctx->global_table->symbols[i];
	// 	if (func_symbol) {
	// 		char* full_text = get_full_text(ctx, func_symbol->name);
	// 		if (full_text) {
				
	// 			write_asm_to_file(writer, full_text);
	// 		}			
	// 	}
	// }

	write_asm_to_file(writer, "entry _start\n_start:");

	char buffer[60];

	snprintf(buffer, sizeof(buffer), "\tcall main\n\tmov rdi, rax\n\tmov rax, 60\n\tsyscall\n");
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

void get_bytes_from_operand(CompilerContext* ctx, OperandSet* symbols_set, Operand* op, int* total_frame_bytes) {
	assert(op);

	switch (op->kind) {
		case OP_SYMBOL: {
			if (op->permanent_frame_position) {
				bool sym_exists = contains_operand_symbol(symbols_set, op->value.sym);
				if (!sym_exists) {
					add_to_operand_set(ctx, symbols_set, op);
					size_t op_size = get_size(op);
					ensure_alignment(&op_size, EIGHT_BYTE_ALIGNMENT);

					*total_frame_bytes += op_size;
					op->value.sym->frame_byte_offset = total_frame_bytes;
				}	
			}
			break;
		}

		default: {		
			if (op->permanent_frame_position) {
				size_t op_size = get_size(op);
				ensure_alignment(&op_size, EIGHT_BYTE_ALIGNMENT);
					
				*total_frame_bytes += op_size;
				op->frame_byte_offset = total_frame_bytes;				
			}	
			break;
		}
	}
}

int determine_operand_use(BasicBlock* block, Operand* op, int start) {
	for (int i = start; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		switch (tac->kind) {
			case TAC_NOT:
			case TAC_LOGICAL_OR:
			case TAC_LOGICAL_AND:
			case TAC_LESS:
			case TAC_GREATER:
			case TAC_LESS_EQUAL:
			case TAC_GREATER_EQUAL:
			case TAC_EQUAL:
			case TAC_NOT_EQUAL:
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV: {
				bool op1_equivalent = operands_equal(tac->op1, op);
				bool op2_equivalent = operands_equal(tac->op2, op);
				if (op1_equivalent || op2_equivalent) {
					return i;
				}
				break;
			}

			case TAC_ASSIGNMENT: {
				if (tac->op2->kind != OP_RETURN) {
					bool op2_equivalent = operands_equal(tac->op2, op);
					if (op2_equivalent) {
						return i;
					}
				}
				break;
			}

			case TAC_RETURN: {
				if (tac->op1) {
					bool op1_equivalent = operands_equal(tac->op1, op);
					if (op1_equivalent) {
						return i;
					}
				}
				break;
			}
		}
	}
	return -1;
}

int determine_operand_use_within_call_boundary(BasicBlock* block, Operand* op, int start, int end) {
	for (int i = start; i < end; i++) {
		TACInstruction* tac = block->instructions[i];
		switch (tac->kind) {
			case TAC_NOT:
			case TAC_LOGICAL_OR:
			case TAC_LOGICAL_AND:
			case TAC_LESS:
			case TAC_GREATER:
			case TAC_LESS_EQUAL:
			case TAC_GREATER_EQUAL:
			case TAC_EQUAL:
			case TAC_NOT_EQUAL:
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV: {
				bool op1_equivalent = operands_equal(tac->op1, op);
				bool op2_equivalent = operands_equal(tac->op2, op);
				if (op1_equivalent || op2_equivalent) {
					return i;
				}
				break;
			}

			case TAC_ASSIGNMENT: {
				if (tac->op2->kind != OP_RETURN) {
					bool op2_equivalent = operands_equal(tac->op2, op);
					if (op2_equivalent) {
						return i;
					}
				}
				break;
			}
		}   		
	}
	return -1;
}

int find_call_instr_index(BasicBlock* block, int start) {
	for (int i = start; i < block->num_instructions; i++) {
		if (block->instructions[i]->kind == TAC_CALL) {
			return i;
		}
	}
	return -1;
}

Operand* find_matching_register(OperandSet* op_set, int reg) {
	for (int i = 0; i < op_set->size; i++) {
		if (op_set->elements[i]->assigned_register == reg) {
			return op_set->elements[i];
		}
	}
	return NULL;
}

Operand* operand_with_furthest_use(OperandSet* op_set) {
	Operand* furthest_op = NULL;
	int max = 0;

	for (int i = 0; i < op_set->size; i++) {
		Operand* current_op = op_set->elements[i];
		
		if ((current_op->assigned_register != -1) && 
			(current_op->next_use > max)) 
		{
			max = op_set->elements[i]->next_use;
			furthest_op = op_set->elements[i];
		} 
	}
	return furthest_op;
}

void get_bytes_for_stack_frames(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;
		// printf("Function: \033[32m%s\033[0m\n", info->symbol->name);
		OperandSet* symbols_set = create_operand_set(ctx);
		assert(symbols_set);

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* block = cfg->all_blocks[j];
			for (int k = 0; k < block->num_instructions; k++) {
				TACInstruction* tac = block->instructions[k];
				switch (tac->kind) {
					case TAC_ASSIGNMENT: {
						if (tac->result->permanent_frame_position) {
							get_bytes_from_operand(ctx, symbols_set, tac->result, &info->total_frame_bytes);
						}

						if (tac->op2->kind == OP_RETURN) break;

						bool both_in_frame = tac->result->permanent_frame_position && tac->op2->permanent_frame_position;
						if (both_in_frame) {
							Operand* furthest_op = operand_with_furthest_use(tac->live_out);
							if (furthest_op) {
								Spill s1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.push_index = k,
									.block_index = j,
									.direction = PUSH_TO_STACK
								};
								add_spill(ctx, block->spill_schedule, s1);

								Spill s2 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = tac->op2->frame_byte_offset,
									.push_index = k,
									.block_index = j,
									.direction = FRAME_TO_REG
								};
								add_spill(ctx, block->spill_schedule, s2);

								tac->op2->temp_register = furthest_op->assigned_register; 

								Reload r1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.pop_index = k + 1,
									.block_index = j,
									.direction = POP_FROM_STACK
								};
								add_reload(ctx, block->reload_schedule, r1);
							}
							break;	
						}

						int call_index = find_call_instr_index(block, k + 1);
						
						if (call_index > 0) {
							int next_use_index = determine_operand_use_within_call_boundary(block, tac->result, k + 1, call_index);
							
							if (next_use_index == -1) {
								bool caller_saved = is_caller_saved(tac->result->assigned_register);
								bool still_live = contains_operand(block->instructions[call_index]->live_out, tac->result);
								
								if (caller_saved && still_live) {									
									Spill s = {
										.assigned_register = tac->result->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k + 1,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);

									// unloading from frame
									int first_use_after_call = determine_operand_use(block, tac->result, call_index + 1);
									assert(first_use_after_call != -1);

									Reload r = {
										.assigned_register = tac->result->assigned_register,
										.frame_byte_offset = -1,
										.pop_index = first_use_after_call,
										.block_index = j,
										.direction = POP_FROM_STACK
									};
									add_reload(ctx, block->reload_schedule, r);
								}

								bool op2_caller_saved = is_caller_saved(tac->op2->assigned_register);
								bool op2_still_live = contains_operand(block->instructions[call_index]->live_out, tac->result);
								
								if (op2_caller_saved && op2_still_live) {
									Spill s = {
										.assigned_register = tac->op2->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k + 1,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);
									
									int first_use_after_call = determine_operand_use(block, tac->op2, call_index + 1);
									assert(first_use_after_call != -1);

									Reload r = {
										.assigned_register = tac->op2->assigned_register,
										.frame_byte_offset = -1,
										.pop_index = first_use_after_call,
										.block_index = j,
										.direction = POP_FROM_STACK
									};
									add_reload(ctx, block->reload_schedule, r);
									
								}
							}
						}
						break;
					}

					case TAC_ADD:
					case TAC_SUB:
					case TAC_MUL: {
						if (tac->result->permanent_frame_position) {
							get_bytes_from_operand(ctx, symbols_set, tac->result, &info->total_frame_bytes);
						}

						if (tac->op1->permanent_frame_position) {	
							Operand* furthest_op = operand_with_furthest_use(tac->live_out);
							if (furthest_op) {
								Spill s1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.push_index = k,
									.block_index = j,
									.direction = PUSH_TO_STACK
								};
								add_spill(ctx, block->spill_schedule, s1);

								Spill s2 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.push_index = k,
									.block_index = j,
									.direction = FRAME_TO_REG
								};
								add_spill(ctx, block->spill_schedule, s2);

								tac->op1->temp_register = furthest_op->assigned_register;

								Reload r1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.pop_index = k + 1,
									.block_index = j,
									.direction = POP_FROM_STACK
								};
								add_reload(ctx, block->reload_schedule, r1);
							}
							break;
						}

						int call_index = find_call_instr_index(block, k + 1);
						if (call_index > 0) {
							int next_use_index = determine_operand_use_within_call_boundary(block, tac->op1, k + 1, call_index);  							
							if (next_use_index != -1) {			
								if (!tac->op1->permanent_frame_position) {
									get_bytes_from_operand(ctx, symbols_set, tac->op1, &info->total_frame_bytes);

									Spill s = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = tac->op1->frame_byte_offset,
										.push_index = k,
										.block_index = j,
										.direction = REG_TO_FRAME
									};
									add_spill(ctx, block->spill_schedule, s);

									Reload next_use = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = tac->op1->frame_byte_offset,
										.pop_index = next_use_index,
										.block_index = j,
										.direction = FRAME_TO_REG
									};
									add_reload(ctx, block->reload_schedule, next_use);	
								}
							} else {
								bool caller_saved = is_caller_saved(tac->op1->assigned_register);
								bool still_live = contains_operand(block->instructions[call_index]->live_out, tac->op1);
								
								if (caller_saved && still_live) {
									Spill s = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);

									int next_use_for_op1 = determine_operand_use(block, tac->op1, call_index + 1);
									if (next_use_for_op1 != -1) {
										Reload r = {
											.assigned_register = tac->op1->assigned_register,
											.frame_byte_offset = -1,
											.pop_index = next_use_for_op1,
											.block_index = j,
											.direction = POP_FROM_STACK
										};
										add_reload(ctx, block->reload_schedule, r);
									}
								}

								bool op2_still_live = contains_operand(block->instructions[call_index]->live_out, tac->op2);
								bool assigned_caller_saved_reg = is_caller_saved(tac->op2->assigned_register);

								if (op2_still_live && assigned_caller_saved_reg) {
									Spill s = {
										.assigned_register = tac->op2->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);

									int next_use_of_op2 = determine_operand_use(block, tac->op2, call_index + 1);
									assert(next_use_of_op2 != -1);

									Reload r = {
										.assigned_register = tac->op2->assigned_register,
										.frame_byte_offset = -1,
										.pop_index = next_use_of_op2,
										.block_index = j,
										.direction = POP_FROM_STACK
									};
									add_reload(ctx, block->reload_schedule, r);									
								}
							}
						} else if (call_index == -1) {
							int next_use_index = determine_operand_use(block, tac->op1, k + 1);
							if (next_use_index != -1) {
								if (!tac->op1->permanent_frame_position) {
									get_bytes_from_operand(ctx, symbols_set, tac->op1, &info->total_frame_bytes);
									
									Spill s = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = tac->op1->frame_byte_offset,
										.push_index = k,
										.block_index = j,
										.direction = FRAME_TO_REG
									};
									add_spill(ctx, block->spill_schedule, s);

									Reload r = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = tac->op1->frame_byte_offset,
										.pop_index = next_use_index,
										.block_index = j,
										.direction = REG_TO_FRAME
									};
									add_reload(ctx, block->reload_schedule, r);
								}					
							}
						}
						break;
					}

					case TAC_MODULO:
					case TAC_DIV: {
						// determining if any variables in live out set have been assigned rdx
						// if so, they need to be preserved
						Operand* first_matching_op = find_matching_register(tac->live_out, 0);
						if (first_matching_op) {
							int next_use_index = determine_operand_use(block, first_matching_op, k + 1);
							if (next_use_index != -1) {
								Spill s = {
									.assigned_register = first_matching_op->assigned_register,
									.frame_byte_offset = -1,
									.push_index = k,
									.block_index = j,
									.direction = PUSH_TO_STACK
								};
								add_spill(ctx, block->spill_schedule, s);

								Reload r = {
									.assigned_register = first_matching_op->assigned_register,
									.frame_byte_offset = -1,
									.pop_index = next_use_index,
									.block_index = j,
									.direction = POP_FROM_STACK
								};
								add_reload(ctx, block->reload_schedule, r);
							}
						}

						Operand* matching_op = find_matching_register(tac->live_out, 4);
						if (matching_op) {
							int next_use_index = determine_operand_use(block, matching_op, k + 1);
							if (next_use_index != -1) {
								Spill s = {
									.assigned_register = matching_op->assigned_register,
									.frame_byte_offset = -1,
									.push_index = k,
									.block_index = j,
									.direction = PUSH_TO_STACK
								};
								add_spill(ctx, block->spill_schedule, s);

								Reload r = {
									.assigned_register = matching_op->assigned_register,
									.frame_byte_offset = -1,
									.pop_index = next_use_index,
									.block_index = j,
									.direction = POP_FROM_STACK
								};
								add_reload(ctx, block->reload_schedule, r);
							}
						}

						int call_index = find_call_instr_index(block, k + 1);
						if (call_index > 0) {
							int next_use_index = determine_operand_use_within_call_boundary(block, tac->op1, k + 1, call_index);  
							if (next_use_index != -1) {
								tac->op1->frame_byte_offset = info->total_frame_bytes + 8; 
								ensure_alignment(&tac->op1->frame_byte_offset, EIGHT_BYTE_ALIGNMENT);
								info->total_frame_bytes += (tac->op1->frame_byte_offset - info->total_frame_bytes);
								// need to preserve value in frame as well
							
								Spill s = {
									.assigned_register = tac->op1->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.push_index = k,
									.block_index = j,
									.direction = REG_TO_FRAME
								};
								add_spill(ctx, block->spill_schedule, s);

									// reload bundle for next use within call boundary
								Reload r = {
									.assigned_register = tac->op1->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.pop_index = next_use_index,
									.block_index = j,
									.direction = FRAME_TO_REG
								};
								add_reload(ctx, block->reload_schedule, r);
							} else {
								bool op1_still_live = contains_operand(block->instructions[call_index]->live_out, tac->op1);
								if (op1_still_live) {
									Spill s = {
										.assigned_register = tac->op1->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);

									int next_use_index = determine_operand_use(block, tac->op1, call_index + 1);
									if (next_use_index != -1) {
										Reload r = {
											.assigned_register = tac->op1->assigned_register,
											.frame_byte_offset = -1,
											.pop_index = next_use_index,
											.block_index = j,
											.direction = POP_FROM_STACK
										};
										add_reload(ctx, block->reload_schedule, r);
									}
								}

								bool op2_still_live = contains_operand(block->instructions[call_index]->live_out, tac->op2);
								bool assigned_caller_saved_reg = is_caller_saved(tac->op2->assigned_register);

								if (op2_still_live && assigned_caller_saved_reg) {
									Spill s = {
										.assigned_register = tac->op2->assigned_register,
										.frame_byte_offset = -1,
										.push_index = k,
										.block_index = j,
										.direction = PUSH_TO_STACK
									};
									add_spill(ctx, block->spill_schedule, s);

									int next_use_index = determine_operand_use(block, tac->op2, call_index + 1);
									if (next_use_index != -1) {
										Reload r = {
											.assigned_register = tac->op2->assigned_register,
											.frame_byte_offset = -1,
											.pop_index = next_use_index,
											.block_index = j,
											.direction = POP_FROM_STACK
										};
										add_reload(ctx, block->reload_schedule, r);
									}
								}
							}
						} else if (call_index == -1) {
							int next_use_index = determine_operand_use(block, tac->op1, k + 1);
							if (next_use_index != -1) {
								tac->op1->frame_byte_offset = info->total_frame_bytes + 8; 
								ensure_alignment(&tac->op1->frame_byte_offset, EIGHT_BYTE_ALIGNMENT);
								info->total_frame_bytes += (tac->op1->frame_byte_offset - info->total_frame_bytes);
								
								Spill s = {
									.assigned_register = tac->op1->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.push_index = k,
									.block_index = j,
									.direction = REG_TO_FRAME
								};
								add_spill(ctx, block->spill_schedule, s);

								Reload r = {
									.assigned_register = tac->op1->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.pop_index = next_use_index,
									.block_index = j,
									.direction = FRAME_TO_REG
								};
								add_reload(ctx, block->reload_schedule, r);
							} 
						}
						break;
					}

					case TAC_UNARY_SUB: {
						if (tac->result->permanent_frame_position) {
							get_bytes_from_operand(ctx, symbols_set, tac->result, &info->total_frame_bytes);
						}

						bool both_in_frame = tac->result->permanent_frame_position && tac->op1->permanent_frame_position;
						if (both_in_frame) {
							Operand* furthest_op = operand_with_furthest_use(tac->live_out);
							if (furthest_op) {
								Spill s1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.push_index = k,
									.block_index = j,
									.direction = PUSH_TO_STACK
								};
								add_spill(ctx, block->spill_schedule, s1);

								Spill s2 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = tac->op1->frame_byte_offset,
									.push_index = k,
									.block_index = j,
									.direction = FRAME_TO_REG
								};
								add_spill(ctx, block->spill_schedule, s2);

								tac->op1->temp_register = furthest_op->assigned_register; 

								Reload r1 = {
									.assigned_register = furthest_op->assigned_register,
									.frame_byte_offset = -1,
									.pop_index = k + 1,
									.block_index = j,
									.direction = POP_FROM_STACK
								};
								add_reload(ctx, block->reload_schedule, r1);
							}
						}
						break;
					}

					case TAC_LESS:
					case TAC_GREATER:
					case TAC_LESS_EQUAL:
					case TAC_GREATER_EQUAL:
					case TAC_EQUAL:
					case TAC_NOT_EQUAL: {
						if (tac->result->permanent_frame_position) {
							get_bytes_from_operand(ctx, symbols_set, tac->result, &info->total_frame_bytes);
						}

						bool both_in_frame = tac->op1->permanent_frame_position && tac->op2->permanent_frame_position;
						if (both_in_frame) {
							Operand* furthest_op = operand_with_furthest_use(tac->live_out);
							Spill s1 = {
								.assigned_register = furthest_op->assigned_register,
								.frame_byte_offset = -1,
								.push_index = k,
								.block_index = j,
								.direction = PUSH_TO_STACK
							};
							add_spill(ctx, block->spill_schedule, s1);

							Spill s2 = {
								.assigned_register = furthest_op->assigned_register,
								.frame_byte_offset = tac->op2->frame_byte_offset,
								.push_index = k,
								.block_index = j,
								.direction = FRAME_TO_REG
							};
							add_spill(ctx, block->spill_schedule, s2);

							tac->op2->temp_register = furthest_op->assigned_register;

							Reload r1 = {
								.assigned_register = furthest_op->assigned_register,
								.frame_byte_offset = -1,
								.pop_index = k + 1,
								.block_index = j,
								.direction = POP_FROM_STACK
							};
							add_reload(ctx, block->reload_schedule, r1);
						}
						break;	
					}

					case TAC_CHAR:
					case TAC_BOOL:
					case TAC_INTEGER:
					case TAC_NOT:
					case TAC_LOGICAL_AND:
					case TAC_LOGICAL_OR:
					case TAC_STORE:
					case TAC_DEREFERENCE:
					case TAC_DEREFERENCE_AND_ASSIGN:
					case TAC_UNARY_ADD: {
						get_bytes_from_operand(ctx, symbols_set, tac->result, &info->total_frame_bytes);
						break;
					}
				}
			}
		}
		ensure_alignment(&info->total_frame_bytes, SIXTEEN_BYTE_ALIGNMENT);
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
	assert(writer);

	schedule_callee_register_spills(ctx, function_list);
	get_bytes_for_stack_frames(ctx, function_list);
	generate_globals(ctx, writer);
	emit_asm_for_functions(ctx, writer, function_list);

	fclose(writer->file);
}