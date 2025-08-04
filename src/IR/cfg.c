#include "cfg.h"

FunctionList* function_list = NULL;
TACLeaders leaders_list;
static int block_id = 0;

FunctionList* create_function_list(CompilerContext* ctx) {
	FunctionList* list = arena_allocate(ctx->ir_arena, sizeof(FunctionList));
	if (!list) return NULL;

	list->size = 0;
	list->capacity = INIT_FUNCTION_LIST_CAPACITY;
	list->infos = arena_allocate(ctx->ir_arena, sizeof(FunctionInfo*) * list->capacity); 
	if (!list->infos) return NULL;
	return list;
}

FunctionInfo* create_function_info(CompilerContext* ctx, TACInstruction* instruction, int tac_start_index, int tac_end_index) {
	FunctionInfo* info = arena_allocate(ctx->ir_arena, sizeof(FunctionInfo));
	if (!info) {
		return NULL;
	}

	info->symbol = instruction->result->value.sym;
	info->tac_start_index = tac_start_index;
	info->tac_end_index = tac_end_index;
	return info;
}

bool add_function_info_to_list(CompilerContext* ctx, FunctionInfo* info) {
	if (!info) return false;

	if (function_list->size >= function_list->capacity) {
		int prev_capacity = function_list->capacity;
		
		function_list->capacity *= 2;
		int new_capacity = function_list->capacity;
		void** new_infos = arena_reallocate(
			ctx->ir_arena, 
			function_list->infos, 
			prev_capacity * sizeof(FunctionInfo*), 
			new_capacity * sizeof(FunctionInfo*)
		);

		if (!new_infos) {
			return false;
		}
		function_list->infos = new_infos;
	}
	function_list->infos[function_list->size++] = info;
	return true;
}

TACLeaders create_tac_leaders(CompilerContext* ctx) {
	int* leaders = arena_allocate(ctx->ir_arena, sizeof(int) * INIT_LEADERS_CAPACITY);
	if (!leaders) {
		TACLeaders dummy_leaders = {
			.size = 0,
			.capacity = 0,
			.leaders = NULL
		};
		return dummy_leaders;
	}

	TACLeaders new_leaders = {
		.size = 0,
		.capacity = INIT_LEADERS_CAPACITY,
		.leaders = leaders
	};
	return new_leaders;
}

bool add_leader_to_leader_list(CompilerContext* ctx, int index) {
	if (index < 0) return false;

	if (leaders_list.size >= leaders_list.capacity) {
		int prev_capacity = leaders_list.capacity;

		leaders_list.capacity *= 2;
		int new_capacity = leaders_list.capacity;
		void* new_leaders = arena_reallocate(
			ctx->ir_arena, 
			leaders_list.leaders, 
			prev_capacity * sizeof(int), 
			new_capacity * sizeof(int)
		);
		
		if (!new_leaders) return false;

		leaders_list.leaders = new_leaders;
	}
	leaders_list.leaders[leaders_list.size++] = index;
	return true;
}

CFG* create_cfg(CompilerContext* ctx) {
	CFG* cfg = arena_allocate(ctx->ir_arena, sizeof(CFG));
	if (!cfg) {
		perror("In 'create_cfg', unable to allocate space for cfg\n");
		return NULL;
	}

	cfg->num_blocks = 0;
	cfg->blocks_capacity = INIT_BLOCKS_CAPACITY;
	cfg->head = NULL;
	cfg->all_blocks = arena_allocate(ctx->ir_arena, sizeof(BasicBlock*) * cfg->blocks_capacity);
	if (!cfg->all_blocks) {
		perror("In 'create_cfg', unable to initialize cfg blocks\n");
		return NULL;
	}
	return cfg;
}

bool add_block_to_cfg(CompilerContext* ctx, CFG* cfg, BasicBlock* block) {
	if (!block) return false;

	if (cfg->num_blocks >= cfg->blocks_capacity) {
		int prev_capacity = cfg->blocks_capacity;

		cfg->blocks_capacity *= 2;
		int new_capacity = cfg->blocks_capacity;
		void** new_blocks = arena_reallocate(
			ctx->ir_arena, 
			cfg->all_blocks, 
			prev_capacity * sizeof(BasicBlock*), 
			new_capacity * sizeof(BasicBlock*)
		);
		
		if (!new_blocks) {
			return false;
		} 
		cfg->all_blocks = new_blocks;
	}
	cfg->all_blocks[cfg->num_blocks++] = block;
	return true; 
}

BasicBlock* create_basic_block(CompilerContext* ctx) {
	BasicBlock* basic_block = arena_allocate(ctx->ir_arena, sizeof(BasicBlock));
	if (!basic_block) {
		perror("In 'create_basic_block', unable to allocate space for basic block\n");
		return NULL;
	}

	basic_block->id = block_id++;
	basic_block->visited = false;
	basic_block->num_instructions = 0;
	basic_block->num_successors = 0;
	basic_block->num_predecessors = 0;
	basic_block->num_instructions_capacity = INIT_TAC_INSTRUCTIONS_CAPACITY;
	basic_block->num_predecessors_capacity = INIT_PREDECESSOR_CAPACITY;
	basic_block->num_successors_capacity = INIT_SUCCESSORS_CAPACITY;

	basic_block->instructions = arena_allocate(ctx->ir_arena, sizeof(TACInstruction*) * basic_block->num_instructions_capacity);
	if (!basic_block->instructions) return NULL;

	basic_block->predecessors = arena_allocate(ctx->ir_arena, sizeof(BasicBlock*) * basic_block->num_predecessors_capacity);
	if (!basic_block->predecessors) return NULL;

	basic_block->successors = arena_allocate(ctx->ir_arena, sizeof(BasicBlock*) * basic_block->num_successors_capacity);
	if (!basic_block->successors) return NULL;

	return basic_block;
}

bool found_function(TACInstruction* instruction) {
	if (!instruction) return false;

	if (instruction->kind == TAC_NAME) {
		if (!instruction->result) return false;
		if (!instruction->result->value.sym) return false;
		if (!instruction->result->value.sym->type) return false;
		if (instruction->result->value.sym->type->kind == TYPE_FUNCTION) {
			return true;
		}
	}

	return false;
}

bool add_instruction_to_block(CompilerContext* ctx, BasicBlock* block, TACInstruction* instruction) {
	if (!block || !instruction) return false;

	if (block->num_instructions >= block->num_instructions_capacity) {
		int prev_capacity = block->num_instructions_capacity;
		
		block->num_instructions_capacity *= 2;
		int new_capacity = block->num_instructions_capacity;

		void** new_instructions = arena_reallocate(
			ctx->ir_arena, 
			block->instructions, 
			prev_capacity * sizeof(TACInstruction*), 
			new_capacity * sizeof(TACInstruction*)
		);

		if (!new_instructions) return false;

		block->instructions = new_instructions;
	}
	block->instructions[block->num_instructions++] = instruction;
	return true;
}

void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions) {
	int tac_start_index = 0;
	int tac_end_index = 0;
	int i = 0;
	while (instructions->tacs[i] && i < instructions->size) {
		if (found_function(instructions->tacs[i])) {
			TACInstruction* func_tac = instructions->tacs[i];
			
			tac_start_index = i;
			int j = i + 1;
			
			while (!found_function(instructions->tacs[j]) && j < instructions->size) { j++; }
			
			bool is_last_instruction = (j == instructions->size) ? true : false;
			if (is_last_instruction) {
				tac_end_index = j;
			} else {
				tac_end_index = j - 1;
			}

			FunctionInfo* func_info = create_function_info(ctx, func_tac, tac_start_index, tac_end_index);
			if (!add_function_info_to_list(ctx, func_info)) return;
			i = tac_end_index + 1;
		} else {
			i++;
		}
	}
}

bool found_label(TACInstruction* instruction) {
	if (!instruction) return false;
	switch (instruction->kind) {
		case TAC_LABEL:
		case TAC_GOTO: return true;
	}
	return false;
}


int find_label_index(TACTable* instructions, char* target_name, int current_index) {
	if (!target_name) return -1;

	for (int i = current_index; i < instructions->size; i++) {
		if (!instructions->tacs[i]->result) continue;
		if (instructions->tacs[i]->result->kind != OP_LABEL) continue;
		if (!instructions->tacs[i]->result->value.label_name) continue;
		if (strcmp(target_name, instructions->tacs[i]->result->value.label_name) == 0) {
			// printf("\033[31mFound label index: %d\033[0m\n", i);
			return i;
		}
	}	

	return -1;
}

void find_leaders(CompilerContext* ctx, TACTable* instructions) {
	int i = 0;

	while (function_list->infos[i] && i < function_list->size) {
		int start = function_list->infos[i]->tac_start_index; // function name index 
		int end = function_list->infos[i]->tac_end_index;

		add_leader_to_leader_list(ctx, start + 1);
		int current_index = start + 2;
		while (instructions->tacs[current_index] && ( current_index <= end )) {
			if (!instructions->tacs[current_index]) {
				current_index++;
				continue;
			}
			switch (instructions->tacs[current_index]->kind) {
				case TAC_IF_FALSE: {
					add_leader_to_leader_list(ctx, current_index + 1 );
					int label_start = find_label_index(instructions, instructions->tacs[current_index]->op1->value.label_name, current_index + 1);
					if (label_start != -1) {
						add_leader_to_leader_list(ctx, label_start);
					}
					break;
				}

				case TAC_LABEL: {
					add_leader_to_leader_list(ctx, current_index);
					break;
				}

				case TAC_GOTO: {
					int label_start = find_label_index(instructions, instructions->tacs[current_index]->result->value.label_name, current_index + 1);
					if (label_start != -1) {
						add_leader_to_leader_list(ctx, label_start);
					}
					break;
				}

				case TAC_RETURN: {
					if (current_index + 1 < instructions->size) {
						TACInstruction* next_tac = instructions->tacs[current_index + 1];
						if (next_tac) {
							switch (next_tac->kind) {
								case TAC_GOTO: break;
								case TAC_NAME: {
									if (next_tac->result &&
										next_tac->result->value.sym &&
										next_tac->result->value.sym->type &&
										next_tac->result->value.sym->type->kind == TYPE_FUNCTION) {
										break;
									}
									add_leader_to_leader_list(ctx, current_index + 1);
									break;
								}

								default: {
									add_leader_to_leader_list(ctx, current_index + 1); 
									break;

								}
							}
						}
					}
					break;
				}

				default: 
					break;
			}

			current_index++;
		}		
		i++;
	}
}

bool index_is_leader(int index) {
	for (int i = 0; i < leaders_list.size; i++) {
		if (index == leaders_list.leaders[i]) return true;
	}
	return false;
}

bool make_function_cfgs(CompilerContext* ctx, TACTable* instructions) {
	for (int i = 0; i < function_list->size; i++) {
		block_id = 0;
		FunctionInfo* info = function_list->infos[i];
		info->cfg = create_cfg(ctx);

		if (!info->cfg) {
			return false;
		}

		info->cfg->head = create_basic_block(ctx);
		if (!info->cfg->head) return false;

		BasicBlock* block = info->cfg->head;

		int start = info->tac_start_index;
		int end = info->tac_end_index;

		int leader_offset = start + 1;
		int remaining_instructions_offset = start + 2;
		add_instruction_to_block(ctx, block, instructions->tacs[leader_offset]);

		while (instructions->tacs[remaining_instructions_offset] && remaining_instructions_offset <= end) {			
			bool is_leader = index_is_leader(remaining_instructions_offset);
			if (!is_leader) {
				add_instruction_to_block(ctx, block, instructions->tacs[remaining_instructions_offset]);
			} else {
				bool added = add_block_to_cfg(ctx, info->cfg, block); 
				if (!added) {
					return false;
				}

				BasicBlock* new_block = create_basic_block(ctx);
				if (!new_block) return false;

				block = new_block;
				add_instruction_to_block(ctx, block, instructions->tacs[remaining_instructions_offset]);
			}
			remaining_instructions_offset++;
		}

		bool added = add_block_to_cfg(ctx, info->cfg, block);
		if (!added) {
			return false;
		}
	}
	return true;
}

BasicBlock* find_matching_label_block(CFG* cfg,  char* target_name) {
	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* current_block = cfg->all_blocks[i];
		if (current_block->num_instructions > 0) {
			TACInstruction* first = current_block->instructions[0];
			if (first->kind == TAC_LABEL &&
				first->result &&
				first->result->value.label_name &&
				strcmp(first->result->value.label_name, target_name) == 0) {
				return current_block;
			}
		}
	}
	return NULL;
}

void add_edges(CompilerContext* ctx, CFG* cfg, int index, BasicBlock* block) {
	if (!block || index >= cfg->num_blocks) return;

	if (cfg->all_blocks[index]->num_successors >= cfg->all_blocks[index]->num_successors_capacity) {
		int prev_capacity = cfg->all_blocks[index]->num_successors_capacity;

		cfg->all_blocks[index]->num_successors_capacity *= 2;
		int new_capacity = cfg->all_blocks[index]->num_successors_capacity;
		void** reallocated_successors = arena_reallocate(ctx->ir_arena, 
			cfg->all_blocks[index]->successors, 
			prev_capacity * sizeof(BasicBlock*), 
			new_capacity * sizeof(BasicBlock*)
		);

		if (!reallocated_successors) {
			perror("In 'add_edges', unable to reallocate space for successors\n");
			return;
		}
		cfg->all_blocks[index]->successors = reallocated_successors;
	}

	if (block->num_predecessors >= block->num_predecessors_capacity) {
		int prev_capacity = block->num_predecessors_capacity;

		block->num_predecessors_capacity *= 2;
		int new_capacity = block->num_predecessors_capacity;
		void** reallocated_predecessors = arena_reallocate(
			ctx->ir_arena, 
			block->predecessors, 
			prev_capacity * sizeof(BasicBlock*), 
			new_capacity * sizeof(BasicBlock*)
		);

		if (!reallocated_predecessors) return;

		block->predecessors = reallocated_predecessors;
	}

	cfg->all_blocks[index]->successors[cfg->all_blocks[index]->num_successors++] = block;
	block->predecessors[block->num_predecessors++] = cfg->all_blocks[index];
}

void link_function_cfgs(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		if (!cfg) continue;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* block = cfg->all_blocks[j];
			TACInstruction* last_instruction_in_current_block = block->instructions[block->num_instructions - 1];
			if (!last_instruction_in_current_block) continue;

			switch (last_instruction_in_current_block->kind) {
				case TAC_GOTO: {
					BasicBlock* matching_block = find_matching_label_block(cfg, last_instruction_in_current_block->result->value.label_name);
					if (matching_block) {
						add_edges(ctx, cfg, j, matching_block);
					}
					break;
				}

				case TAC_IF_FALSE: {					
					if (j + 1 < cfg->num_blocks) {
						add_edges(ctx, cfg, j, cfg->all_blocks[j + 1]);
					}
					
					BasicBlock* matching_block = find_matching_label_block(cfg, last_instruction_in_current_block->op2->value.label_name);
					if (matching_block) {
						add_edges(ctx, cfg, j, matching_block);
					}
					break;
				}
				
				case TAC_RETURN: 
					break;

				default: {
					if (j + 1 < cfg->num_blocks) {
						add_edges(ctx, cfg, j, cfg->all_blocks[j + 1]);
					}
					break;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////
// live analysis functions 
LivenessTable* create_liveness_table(CompilerContext* ctx) {
	LivenessTable* table = arena_allocate(ctx->ir_arena, sizeof(LivenessTable));
	if (!table) return NULL;

	table->size = 0;
	table->capacity = INIT_LIVENESS_TABLE_CAPACITY;
	table->liveness_infos = arena_allocate(ctx->ir_arena, table->capacity * sizeof(LivenessInfo*));
	if (!table->liveness_infos) return NULL;

	return table;
}

LivenessInfo* create_liveness_info(CompilerContext* ctx, operand_t type, LiveInfoVar var, bool is_live, int next_use) {
	LivenessInfo* live_info = arena_allocate(ctx->ir_arena, sizeof(LivenessInfo));
	if (!live_info) return NULL;

	live_info->type = type;
	live_info->var = var;
	live_info->is_live = is_live;
	live_info->next_use = next_use;
	live_info->link = NULL;
	return live_info;
}

bool add_liveinfo_to_liveness_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* live_info, int hash_key) {
	if (!live_info) return false;

	if (table->size >= table->capacity) {
		int prev_capacity = table->capacity;

		LivenessInfo** prev_infos = arena_allocate(ctx->ir_arena, prev_capacity * sizeof(LivenessInfo*));
		if (!prev_infos) return false;
		for (int i = 0; i < prev_capacity; i++) {
			prev_infos[i] = table->liveness_infos[i]; 
		}

		table->capacity *= 2;
		void** new_live_infos = arena_reallocate(
			ctx->ir_arena,
			table->liveness_infos,
			prev_capacity * sizeof(LivenessInfo*),
			table->capacity * sizeof(LivenessInfo*)
		);

		if (!new_live_infos) return false;

		table->liveness_infos = new_live_infos;
		for (int i = 0; i < table->capacity; i++) {
			table->liveness_infos[i] = NULL;
		}

		table->size = 0;

		for (int i = 0; i < prev_capacity; i++) {
			LivenessInfo* current_prev_info = prev_infos[i];
			while (current_prev_info) {
				LivenessInfo* next_info = current_prev_info->link;
				int updated_hash_key = -1;

				switch (current_prev_info->type) {
					case OP_SYMBOL: {
						if (current_prev_info->var.symbol && current_prev_info->var.symbol->name) {
							updated_hash_key = hash_variable(table, current_prev_info->var.symbol->name);
						}
						break;
					}

					case OP_EQUAL:
					case OP_NOT_EQUAL:
					case OP_LOGICAL_OR:
					case OP_LOGICAL_AND:
					case OP_LESS:
					case OP_GREATER:
					case OP_LESS_EQUAL:
					case OP_GREATER_EQUAL:
					case OP_NOT:
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MODULO:
					case OP_STORE: {
						if (current_prev_info->var.label_name) {
							updated_hash_key = hash_variable(table, current_prev_info->var.label_name);
						}
						break;
					}
				}

				if (updated_hash_key == -1) return false;

				current_prev_info->link = table->liveness_infos[updated_hash_key];
				table->liveness_infos[updated_hash_key] = current_prev_info;
				if (!current_prev_info->link) {
					table->size++;
				}
				current_prev_info = next_info;
			}
		}

		switch (live_info->type) {
			case OP_SYMBOL: {
				if (live_info->var.symbol && live_info->var.symbol->name) {
					hash_key = hash_variable(table, live_info->var.symbol->name);
				}
				break;
			}

			case OP_UNARY_ADD:
			case OP_UNARY_SUB:
			case OP_EQUAL:
			case OP_NOT_EQUAL:
			case OP_LOGICAL_OR:
			case OP_LOGICAL_AND:
			case OP_LESS:
			case OP_GREATER:
			case OP_LESS_EQUAL:
			case OP_GREATER_EQUAL:
			case OP_NOT:
			case OP_ADD:
			case OP_SUB:
			case OP_MUL:
			case OP_DIV:
			case OP_MODULO:
			case OP_STORE: {
				if (live_info->var.label_name) {
					hash_key = hash_variable(table, live_info->var.label_name);
				}
				break;
			}
		}
	}

	if (hash_key == -1) return false;

	if (table->liveness_infos[hash_key]) {
		live_info->link = table->liveness_infos[hash_key];
		table->liveness_infos[hash_key] = live_info;
	} else {
		table->liveness_infos[hash_key] = live_info;
		live_info->link = NULL;
		table->size++;
	}

	return true;
}

OperandSet* copy_set(CompilerContext* ctx, OperandSet* original_set) {
	if (!original_set) return NULL;

	OperandSet* duplicate_set = arena_allocate(ctx->ir_arena, sizeof(OperandSet));
	if (!duplicate_set) return NULL;

	duplicate_set->size = original_set->size;
	duplicate_set->capacity = original_set->capacity;
	duplicate_set->elements = arena_allocate(ctx->ir_arena, sizeof(Operand*) * duplicate_set->capacity);
	if (!duplicate_set->elements) return NULL;

	for (int i = 0; i < duplicate_set->size; i++) {
		duplicate_set->elements[i] = original_set->elements[i];
	}
	return duplicate_set;
}

bool init_block_sets(CompilerContext* ctx, BasicBlock* block) {
	if (!block) return false;

	block->use_set = create_operand_set(ctx);
	block->def_set = create_operand_set(ctx);
	block->in_set = create_operand_set(ctx);
	block->out_set = create_operand_set(ctx);

	if (!block->use_set || !block->def_set ||
		!block->in_set || !block->out_set) {
		return false;
	}
	return true;
}

bool operands_equal(Operand* op1, Operand* op2) {
	if (!op1 || !op2) return false;

	if (op1->kind != op2->kind) return false;

	switch (op1->kind) {
		case OP_SYMBOL: {
			return (op1->value.sym && op2->value.sym) && 
				   (op1->value.sym == op2->value.sym) &&
				    op1->value.sym->scope_level == op2->value.sym->scope_level &&
				    strcmp(op1->value.sym->name, op2->value.sym->name) == 0;
		}

		case OP_STORE:
		case OP_NOT:
		case OP_UNARY_SUB:
		case OP_UNARY_ADD:
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
		case OP_LOGICAL_AND:
		case OP_LOGICAL_OR: {
			return (op1->value.label_name && op2->value.label_name) && 
				   (strcmp(op1->value.label_name, op2->value.label_name) == 0);
		}
		
		default: 
			return false;
	}

}

bool contains_operand(OperandSet* op_set, Operand* operand) {
	if (!op_set || !operand) return false;

	for (int i = 0; i < op_set->size; i++) {
		if (operands_equal(op_set->elements[i], operand)) {
			return true;
		}
	}

	return false;
}

int get_operand_index(OperandSet* op_set, Operand* operand) {
	if (!op_set || !operand) return -1;

	for (int i = 0; i < op_set->size; i++) {
		if (op_set->elements[i] == operand) {
			return i;
		}
	}
	return -1;
}

void remove_from_operand_set(OperandSet* op_set, Operand* operand) {
	int operand_index = get_operand_index(op_set, operand);
	
	if (operand_index != -1) {
		op_set->elements[operand_index] = NULL;
		for (int j = operand_index; j < op_set->size; j++) {
			op_set->elements[j] = op_set->elements[j + 1];
		}
		op_set->size--;
	}
}

void populate_use_and_def_sets(CompilerContext* ctx, BasicBlock* block) {
	if (!block) return;

	OperandSet* ops_defined = create_operand_set(ctx);
	for (int i = 0; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		if (!tac) continue;

		switch (tac->kind) {
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV:
			case TAC_MODULO:
			case TAC_LESS:
			case TAC_GREATER:
			case TAC_LESS_EQUAL:
			case TAC_GREATER_EQUAL:
			case TAC_EQUAL:
			case TAC_NOT_EQUAL:
			case TAC_LOGICAL_OR:
			case TAC_LOGICAL_AND: {
				bool op1_already_def = contains_operand(ops_defined, tac->op1);
				if (!op1_already_def) {
					add_to_operand_set(ctx, block->use_set, tac->op1);
				}

				bool op2_already_def = contains_operand(ops_defined, tac->op2);
				if (!op2_already_def) {
					add_to_operand_set(ctx, block->use_set, tac->op2);
				}

				add_to_operand_set(ctx, ops_defined, tac->result);
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_IF_FALSE: {
				if (tac->op1) {
					bool op1_already_def = contains_operand(ops_defined, tac->op1);
					if (!op1_already_def) {
						add_to_operand_set(ctx, block->use_set, tac->op1);
					}
				}
				break;
			}

			case TAC_RETURN: {
				if (tac->op1) {
					bool op1_already_def = contains_operand(ops_defined, tac->op1);
					if (!op1_already_def) {
						add_to_operand_set(ctx, block->use_set, tac->op1);
					}
				}
				break;
			}

			case TAC_CHAR:
			case TAC_BOOL:
			case TAC_INTEGER: {
				add_to_operand_set(ctx, ops_defined, tac->result);
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_PARAM: break;

			case TAC_ARG: {
				bool op1_already_def = contains_operand(ops_defined, tac->op1);
				if (!op1_already_def) {
					add_to_operand_set(ctx, ops_defined, tac->op1);
				}
				break;
			}

			case TAC_STORE:
			case TAC_DEREFERENCE_AND_ASSIGN:
			case TAC_DEREFERENCE: 
			case TAC_UNARY_ADD:
			case TAC_UNARY_SUB:
			case TAC_NOT: {
				if (tac->op1) {
					bool op1_already_def = contains_operand(ops_defined, tac->op1);
					if (!op1_already_def) {
						add_to_operand_set(ctx, block->use_set, tac->op1);
					}
				}
				add_to_operand_set(ctx, ops_defined, tac->result);
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_ASSIGNMENT: {
				if (tac->op2 && tac->op2->kind == OP_RETURN) {
					add_to_operand_set(ctx, block->def_set, tac->result);
				} else {
					bool op2_already_def = contains_operand(ops_defined, tac->op2);
					if (!op2_already_def) {
						add_to_operand_set(ctx, block->use_set, tac->op2);
					}
					add_to_operand_set(ctx, block->def_set, tac->result);
				}
				add_to_operand_set(ctx, ops_defined, tac->result);
				break;
			}
		}
	}
}

void union_sets(CompilerContext* ctx, OperandSet* dest, OperandSet* src) {
	for (int i = 0; i < src->size; i++) {
		if (!contains_operand(dest, src->elements[i])) {
			add_to_operand_set(ctx, dest, src->elements[i]);
		}
	}
}

bool sets_equal(OperandSet* set1, OperandSet* set2) {
	if (set1->size != set2->size) return false;

	for (int i = 0; i < set1->size; i++) {
		if (!contains_operand(set2, set1->elements[i])) {
			return false;
		}
	}
	return true;
}

OperandSet* difference_sets(CompilerContext* ctx, OperandSet* set1, OperandSet* set2) {
	if (!set1 || !set2) return NULL;

	OperandSet* diff_set = create_operand_set(ctx);
	for (int i = 0; i < set1->size; i++) {
		if (!contains_operand(set2, set1->elements[i])) {
			add_to_operand_set(ctx, diff_set, set1->elements[i]);
		}
	}

	return diff_set;
}

void fixed_point_iteration(CompilerContext* ctx, CFG* cfg) {
	bool changed = true;
	while (changed) {
		changed = false;

		for (int i = cfg->num_blocks - 1; i >= 0; i--) {
			BasicBlock* block = cfg->all_blocks[i];

			OperandSet* old_out = copy_set(ctx, block->out_set);
			OperandSet* old_in = copy_set(ctx, block->in_set);

			OperandSet* new_out = create_operand_set(ctx);
			for (int j = 0; j < block->num_successors; j++) {			
				BasicBlock* successor = block->successors[j];
				union_sets(ctx, new_out, successor->in_set);
			}

			OperandSet* out_minus_def = difference_sets(ctx, new_out, block->def_set);
			OperandSet* new_in = create_operand_set(ctx);
			union_sets(ctx, new_in, block->use_set);
			union_sets(ctx, new_in, out_minus_def);

			if (!sets_equal(old_in, new_in) || !sets_equal(old_out, new_out)) {
				changed = true;
			}

			block->out_set = new_out;
			block->in_set = new_in;
		}		
	}
}

int hash_variable(LivenessTable* table, char* name) {
	if (!table || !name) return -1;
	
	int hash = 0;
	
	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % table->capacity;
}

LivenessInfo* retrieve_livenessinfo(LivenessTable* table, int hash_key, char* target_name, operand_t type) {
	if (!table || hash_key == -1 || !target_name) return NULL;

	bool is_symbol = (type == OP_SYMBOL) ? true : false;

	LivenessInfo* current = table->liveness_infos[hash_key];
	if (!current) return NULL;
	
	while (current) {
		LivenessInfo* link = current->link;
		if (current->type == type) {
			char* info_name = NULL;
			if (is_symbol) {
				if (current->var.symbol && current->var.symbol->name) {
					info_name = current->var.symbol->name;
				}
			} else {
				info_name = current->var.label_name;
			}

			if (info_name && strcmp(target_name, info_name) == 0) {
				return current;
			}

		}
		current = link;
	}
	
	return NULL;
}

void determine_operand_liveness_and_next_use(CompilerContext* ctx, LivenessTable* live_variables, 
	Operand* operand, operand_role role, int instruction_index) {
	if (!operand) return;
	
	switch (operand->kind) {
		case OP_SYMBOL: {
			if (!operand->value.sym || (operand->value.sym && !operand->value.sym->name)) return;
			
			if (operand->value.sym->type && operand->value.sym->type->kind == TYPE_FUNCTION) return;
			
			LiveInfoVar var = { .symbol = operand->value.sym}; 

			int hash_key = hash_variable(live_variables, var.symbol->name);	
			if (hash_key == -1) return; 

			LivenessInfo* stored_info = NULL;
			switch (role) {	
				case OP_RESULT: {
					// printf("Before Retrieving Liveness Info:\nHash Key: \033[32m%d\033[0m -> Name: \033[32m%s\033[0m\n\n", 
					// 	hash_key, operand->value.sym->name);
					
					stored_info = retrieve_livenessinfo(live_variables, hash_key, operand->value.sym->name, OP_SYMBOL); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, OP_SYMBOL, var, false, -1);
						add_liveinfo_to_liveness_table(ctx, live_variables, new_info, hash_key);
						
						operand->next_use = new_info->next_use;
						operand->is_live = new_info->is_live;
						stored_info = new_info;
					} else {
						operand->next_use = stored_info->next_use;
						operand->is_live = stored_info->is_live;

						stored_info->next_use = -1;
						stored_info->is_live = false;
					}
					
					break;
				}
				case OP_USE: {
					stored_info = retrieve_livenessinfo(live_variables, hash_key, operand->value.sym->name, OP_SYMBOL); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, OP_SYMBOL, var, true, instruction_index);
						add_liveinfo_to_liveness_table(ctx, live_variables, new_info, hash_key);	
						operand->is_live = new_info->is_live;
						operand->next_use = new_info->next_use;
						stored_info = new_info;
					} else {
						operand->next_use = stored_info->next_use;
						operand->is_live = stored_info->is_live; 
						
						stored_info->next_use = instruction_index;
						stored_info->is_live = true;
					}
					break;
				}
			}

			break;
		}

		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MODULO:
		case OP_LESS:
		case OP_GREATER:
		case OP_GREATER_EQUAL:
		case OP_LESS_EQUAL:
		case OP_EQUAL:
		case OP_NOT_EQUAL:
		case OP_NOT:
		case OP_UNARY_ADD:
		case OP_UNARY_SUB:
		case OP_LOGICAL_OR:
		case OP_LOGICAL_AND:
		case OP_STORE: {
			if (!operand->value.label_name) return;

			LiveInfoVar var = {.label_name = operand->value.label_name};
			// printf("\033[32mIn OP_BINARY/STORE/UNARY case in 'determine_operand_liveness_and_next_use', variable name is '%s' with address: %p\033[0m\n", operand->value.label_name, operand->value.label_name);
			int hash_key = hash_variable(live_variables, operand->value.label_name);
			

			LivenessInfo* stored_info = NULL;
			switch (role) {
				case OP_RESULT: {
					stored_info = retrieve_livenessinfo(live_variables, hash_key, operand->value.label_name, operand->kind); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, operand->kind, var, false, -1);
						add_liveinfo_to_liveness_table(ctx, live_variables, new_info, hash_key);
						operand->is_live = new_info->is_live;
						operand->next_use = new_info->next_use;
						stored_info = new_info;
					} else {
						operand->is_live = false;
						operand->next_use = stored_info->next_use;

						stored_info->is_live = false;
					}
					break;
				}
				case OP_USE: {
					stored_info = retrieve_livenessinfo(live_variables, hash_key, operand->value.label_name, operand->kind); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, operand->kind, var, true, instruction_index);
						add_liveinfo_to_liveness_table(ctx, live_variables, new_info, hash_key);
						operand->is_live = new_info->is_live;
						operand->next_use = new_info->next_use;
						stored_info = new_info;
					} else {
						operand->next_use = stored_info->next_use;
						operand->is_live = stored_info->is_live;

						stored_info->next_use = instruction_index;
						stored_info->is_live = true;
					} 
					break;
				} 
			}

			break;
		}
		default: break;
	}
}

void determine_instruction_liveness_info(CompilerContext* ctx, LivenessTable* live_variables, TACInstruction* instruction) {
	if (!instruction) return;
	determine_operand_liveness_and_next_use(ctx, live_variables, instruction->result, OP_RESULT, instruction->id);
	determine_operand_liveness_and_next_use(ctx, live_variables, instruction->op1, OP_USE, instruction->id);
	determine_operand_liveness_and_next_use(ctx, live_variables, instruction->op2, OP_USE, instruction->id);
}

void determine_next_use(CompilerContext* ctx, CFG* cfg) {
	for (int i = cfg->num_blocks - 1; i >= 0; i--) {
		BasicBlock* current_block = cfg->all_blocks[i];

		LivenessTable* live_variables = create_liveness_table(ctx);

		for (int j = 0; j < current_block->out_set->size; j++) {
			Operand* op = current_block->out_set->elements[j];
			int hash_key = 0;
			switch (op->kind) {
				case OP_SYMBOL: {
					LiveInfoVar var = {.symbol = op->value.sym};
					if (op->value.sym && op->value.sym->name) {
						hash_key = hash_variable(live_variables, op->value.sym->name);
					} 

					if (hash_key != -1) {
						LivenessInfo* live_info = create_liveness_info(ctx, OP_SYMBOL, var, true, -1);
						add_liveinfo_to_liveness_table(ctx, live_variables, live_info, hash_key);
					} 
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
				case OP_EQUAL:
				case OP_NOT_EQUAL:
				case OP_LOGICAL_OR:
				case OP_LOGICAL_AND:
				case OP_NOT:
				case OP_UNARY_ADD:
				case OP_UNARY_SUB:
				case OP_STORE: {
					LiveInfoVar var = {.label_name = op->value.label_name};
					if (op->value.label_name) {
						hash_key = hash_variable(live_variables, op->value.label_name);  
					} else { 
						// printf("Error: operand has NULL label name\n");
						return; 
					}

					if (hash_key != -1) {
						LivenessInfo* live_info = create_liveness_info(ctx, op->kind, var, true, -1);
						add_liveinfo_to_liveness_table(ctx, live_variables, live_info, hash_key);
					}
					break;
				}

				default: break;
			}
		}

		for (int k = current_block->num_instructions - 1; k>= 0; k--) {
			determine_instruction_liveness_info(ctx, live_variables, current_block->instructions[k]);
		}
	}
}

void compute_instruction_live_out(CompilerContext* ctx, CFG* cfg) {
	printf("==================================\n");
	printf("Displaying Out sets for each block\n\n");
	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i];
		printf("Block \033[32m%d\033[0m\n", i);
		printf("Out: {");
		for (int k = 0; k < block->out_set->size; k++) {
			Operand* out_op = block->out_set->elements[k];
			bool last_op = (k == block->out_set->size - 1);
			if (out_op) {
				switch (out_op->kind) {
					case OP_SYMBOL: {
						if (last_op) {
							printf("%s", out_op->value.sym->name);
						} else {
							printf("%s, ", out_op->value.sym->name);
						}
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
					case OP_EQUAL:
					case OP_NOT_EQUAL:
					case OP_NOT:
					case OP_LOGICAL_AND:
					case OP_LOGICAL_OR:
					case OP_UNARY_ADD:
					case OP_UNARY_SUB:
					case OP_STORE: {
						if (last_op) {
							printf("%s", out_op->value.label_name);
						} else {
							printf("%s, ", out_op->value.label_name);
						}
						break;
					}
				}
			}
		}
		printf("}\n");

		printf("In: {");
		for (int r = 0; r < block->in_set->size; r++) {
			Operand* in_op = block->in_set->elements[r];
			bool last_op = (r == block->in_set->size - 1);
			if (in_op) {
				switch (in_op->kind) {
					case OP_SYMBOL: {
						if (last_op) {
							printf("%s", in_op->value.sym->name);
						} else {
							printf("%s, ", in_op->value.sym->name);
						}
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
					case OP_EQUAL:
					case OP_NOT_EQUAL:
					case OP_NOT:
					case OP_LOGICAL_AND:
					case OP_LOGICAL_OR:
					case OP_UNARY_ADD:
					case OP_UNARY_SUB:
					case OP_STORE: {
						if (last_op) {
							printf("%s", in_op->value.label_name);
						} else {
							printf("%s, ", in_op->value.label_name);
						}
						break;
					}
				}
			}
		}
		printf("}\n\n");

		OperandSet* current_live = copy_set(ctx, block->out_set);

		for (int j = block->num_instructions - 1; j >= 0; j--) {
			TACInstruction* instruction = block->instructions[j];

			instruction->live_out = copy_set(ctx, current_live);

			if (instruction->result && is_operand_label_or_symbol(instruction->result)) {
				remove_from_operand_set(instruction->live_out, instruction->result);
			}

			if (instruction->op1 && is_operand_label_or_symbol(instruction->op1)) {
				add_to_operand_set(ctx, instruction->live_out, instruction->op1);
			}

			if (instruction->op2 && is_operand_label_or_symbol(instruction->op2)) {
				add_to_operand_set(ctx, instruction->live_out, instruction->op2);
			}

			
		}
	}
	printf("==================================\n");
}

void live_analysis(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* block = cfg->all_blocks[j];
			
			bool has_block_sets = init_block_sets(ctx, block);
			if (!has_block_sets) return;

			populate_use_and_def_sets(ctx, block);
			printf("Block \033[32m%d\033[0m\n", j);
			printf("Def: {");
			for (int k = 0; k < block->def_set->size; k++) {
				Operand* def_op = block->def_set->elements[k];
				bool last_op = (k == block->def_set->size - 1);
				if (def_op) {
					switch (def_op->kind) {
						case OP_SYMBOL: {
							if (last_op) {
								printf("%s", def_op->value.sym->name);
							} else {
								printf("%s, ", def_op->value.sym->name);
							}
							break;
						}
						default: {
							if (last_op) {
								printf("%s", def_op->value.label_name);
							} else {
								printf("%s, ", def_op->value.label_name);
							}
							break;
						}
					}
				}
			}
			printf("}\n");

			printf("Use: {");
			for (int u = 0; u < block->use_set->size; u++) {
				Operand* use_op = block->use_set->elements[u];
				bool last_op = (u == block->use_set->size - 1);
				if (use_op) {
					switch (use_op->kind) {
						case OP_SYMBOL: {
							if (last_op) {
								printf("%s", use_op->value.sym->name);
							} else {
								printf("%s, ", use_op->value.sym->name);
							}
							break;
						}

						default: {
							if (last_op) {
								printf("%s", use_op->value.label_name);
							} else {
								printf("%s, ", use_op->value.label_name);
							}
							break;
						}
					}
				}
			}
			printf("}\n\n");
		}

		fixed_point_iteration(ctx, cfg);
		compute_instruction_live_out(ctx, cfg);
		determine_next_use(ctx, cfg);
	}
}
/// END
//////////////////////////////////////

void add_bundle_to_interference_graph(CompilerContext* ctx, InterferenceGraph* graph, InterferenceBundle* bundle) {
	if (!bundle) return;

	if (graph->size >= graph->capacity) {
		int prev_capacity = graph->capacity;

		graph->capacity *= 2;
		int new_capacity = graph->capacity;
		void** new_bundles = arena_reallocate(
			ctx->ir_arena,
			graph->bundles,
			prev_capacity * sizeof(InterferenceBundle*),
			new_capacity * sizeof(InterferenceBundle*)
		);

		if (!new_bundles) return;

		graph->bundles = new_bundles;
	}

	graph->bundles[graph->size++] = bundle;
}

InterferenceBundle* create_inteference_bundle(CompilerContext* ctx, Operand* operand, BasicBlock* associated_block) {
	InterferenceBundle* bundle = arena_allocate(ctx->ir_arena, sizeof(InterferenceBundle));
	if (!bundle) return NULL;

	bundle->operand = operand;
	bundle->associated_block = associated_block;
	bundle->interferes_with = create_operand_set(ctx);
	if (!bundle->interferes_with) return NULL;

	return bundle;
}

InterferenceGraph* create_inteference_graph(CompilerContext* ctx) {
	InterferenceGraph* graph = arena_allocate(ctx->ir_arena, sizeof(InterferenceGraph));
	if (!graph) return NULL;

	graph->size = 0;
	graph->capacity = INIT_INTERFERENCE_BUNDLE_CAPACITY;
	graph->bundles = arena_allocate(ctx->ir_arena, sizeof(InterferenceBundle*) * graph->capacity);
	if (!graph->bundles) return NULL;

	return graph;
}

void populate_interference_graph(CompilerContext* ctx, CFG* cfg, InterferenceGraph* graph) {
	for (int i = 0; i < cfg->num_blocks; i++) {
		BasicBlock* block = cfg->all_blocks[i];

		for (int j = 0; j < block->num_instructions; j++) {
			TACInstruction* instruction = block->instructions[j];
			if (instruction) {
				switch (instruction->kind) {
					case TAC_ADD:
					case TAC_SUB: {
						bool live_on_exit = contains_operand(block->out_set, instruction->result);
						if (!live_on_exit) {
							instruction->result->live_on_exit = 0;
						}
						break;
					}	

					case TAC_RETURN: {
						if (instruction->op1) {
							instruction->op1->live_on_exit = 1;
						}
						break;
					}

					case TAC_LESS:
					case TAC_GREATER:
					case TAC_LESS_EQUAL:
					case TAC_GREATER_EQUAL:
					case TAC_EQUAL:
					case TAC_NOT_EQUAL: {
						if (j + 1 < block->num_instructions) {
							TACInstruction* next_instruction = block->instructions[j + 1];
							if ((next_instruction && next_instruction->result) && 
								next_instruction->kind == TAC_IF_FALSE &&
								strcmp(next_instruction->result->value.label_name, instruction->result->value.label_name) == 0) {
								instruction->precedes_conditional = true;	
							}
						}
						break;
					}
				}
			}
			
			// if (instruction && instruction->precedes_conditional) continue;

			if (instruction && instruction->result) {
				InterferenceBundle* bundle = create_inteference_bundle(ctx, instruction->result, block);
				if (!bundle) continue; 
				
				for (int k = 0; k < instruction->live_out->size; k++) {
					if (instruction->result != instruction->live_out->elements[k]) {
						add_to_operand_set(ctx, bundle->interferes_with, instruction->live_out->elements[k]); 
					}
				}

				add_bundle_to_interference_graph(ctx, graph, bundle);	 
			}
		}
	}
}

void populate_interference_graphs(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		info->graph = create_inteference_graph(ctx);

		if (info->graph) {
			populate_interference_graph(ctx, info->cfg, info->graph);
		}
	}
}

bool is_operand_label_or_symbol(Operand* op) {
	if (!op) return false;

	switch (op->kind) {
		case OP_SYMBOL: 
		case OP_STORE: 
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
		case OP_DIV:
		case OP_MODULO:
		case OP_LESS:
		case OP_GREATER:
		case OP_GREATER_EQUAL:
		case OP_LESS_EQUAL:
		case OP_NOT_EQUAL:
		case OP_EQUAL:
		case OP_NOT:
		case OP_UNARY_SUB:
		case OP_UNARY_ADD:
		case OP_LOGICAL_OR:
		case OP_LOGICAL_AND: {
			return true;
		}
		
		default: return false;
	}
}

FunctionList* build_cfg(CompilerContext* ctx, TACTable* instructions) {
	if (!instructions) return NULL;

	function_list = create_function_list(ctx);
	leaders_list = create_tac_leaders(ctx);
	if (!function_list || !leaders_list.leaders) return NULL;
	
	mark_function_boundaries(ctx, instructions);
	find_leaders(ctx, instructions);
	make_function_cfgs(ctx, instructions);
	link_function_cfgs(ctx);
 
	live_analysis(ctx);
	populate_interference_graphs(ctx);

	return function_list;
}
	// emit_function_infos();
	// emit_blocks();
	// emit_leaders();

void emit_function_infos() {
	// printf("\n\n");
	// printf("\033[31mLast Liveness check\033[0m\n");
	// for (int i = 0; i < function_list->size; i++) {
	// 	FunctionInfo* info = function_list->infos[i];
	// 	CFG* cfg = info->cfg;

	// 	for (int j = 0; j < cfg->num_blocks; j++) {
	// 		BasicBlock* current_block = cfg->all_blocks[j];

	// 		for (int k = 0; k < current_block->num_instructions; k++) {
	// 			emit_liveness_info(current_block->instructions[k]);
	// 		}
	// 	}
	// }
	for (int i = 0; i < function_list->size; i++) {
		printf("Function name: \033[32m%s\033[0m -> Start Index: %d -> End Index: %d\n", function_list->infos[i]->symbol->name, function_list->infos[i]->tac_start_index, function_list->infos[i]->tac_end_index);
	}
}

void emit_leaders() {
	for (int i = 0; i < leaders_list.size; i++) {
		printf("Leader index: \033[32m%d\033[0m\n", leaders_list.leaders[i]);
	}
}

void emit_blocks() {
	for (int i = 0; i < function_list->size; i++) {
		printf("\nFunction: \033[32m%s\033[0m\n", function_list->infos[i]->symbol->name);
		if (function_list->infos[i]->cfg) {
			for (int j = 0; j < function_list->infos[i]->cfg->num_blocks; j++) {
				printf("\tBlock %d:\n", j);
				BasicBlock* current_block = function_list->infos[i]->cfg->all_blocks[j];
				for (int k = 0; k < current_block->num_instructions; k++) {
					printf("\t\tTAC type %d\n", current_block->instructions[k]->kind);
				}

			}			
		}
	}
}
