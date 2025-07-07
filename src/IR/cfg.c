#include "cfg.h"

FunctionList* function_list = NULL;
TACLabelEntries tac_entries;
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

	info->name = instruction->result->value.sym->name;
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

TACLabelEntries create_tac_label_entries(CompilerContext* ctx) {
	TACLabel** labels = arena_allocate(ctx->ir_arena, sizeof(TACLabel*) * INIT_TAC_LABEL_ENTRIES_CAPACITY);
	if (!labels) {
		TACLabelEntries dummy_entries = {
			.size = 0,
			.capacity = 0,
			.labels = NULL
		};
		return dummy_entries;
	}

	TACLabelEntries new_label_entries = {
		.size = 0,
		.capacity = INIT_TAC_LABEL_ENTRIES_CAPACITY,
		.labels = labels
	};
	return new_label_entries;
}

TACLabel* create_label_entry(CompilerContext* ctx, TACInstruction* instruction, int tac_index) {
	if (!instruction) return NULL;

	TACLabel* new_label = arena_allocate(ctx->ir_arena, sizeof(TACLabel));
	if (!new_label) return NULL;

	new_label->tac_index = tac_index;
	new_label->name = NULL;
	switch (instruction->type) {
		case TAC_GOTO:
		case TAC_LABEL: {
			new_label->name = instruction->result->value.label_name;
			break;
		}

		case TAC_IF_FALSE: {
			new_label->name = instruction->op1->value.label_name;
			break;
		}
		default: {
			printf("unknown instruction type\n");
			break;
		}
	}
	return new_label;
}

bool add_label_to_entries(CompilerContext* ctx, TACLabel* label) {
	if (!label) return false;

	if (tac_entries.size >= tac_entries.capacity) {
		int prev_capacity = tac_entries.capacity;
		
		tac_entries.capacity *= 2;
		int new_capacity = tac_entries.capacity;
		void** new_labels = arena_reallocate(
			ctx->ir_arena, 
			tac_entries.labels, 
			prev_capacity * sizeof(TACLabel*), 
			new_capacity * sizeof(TACLabel*)
		);
		
		if (!new_labels) {
			printf("Error: unable to reallocate space for new labels\n");
			return false;
		}
		tac_entries.labels = new_labels;
	}
	tac_entries.labels[tac_entries.size++] = label;
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
	cfg->head = create_basic_block(ctx);
	if (!cfg->head) {
		perror("In 'create_cfg', unable to initialize cfg->head\n");
		return NULL;
	}
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

	if (instruction->type == TAC_NAME) {
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
	switch (instruction->type) {
		case TAC_LABEL:
		case TAC_GOTO: return true;
	}
	return false;
}

void mark_labels(CompilerContext* ctx, TACTable* instructions) {
	int i = 0;
	while (instructions->tacs[i] && i < instructions->size) {
		if (found_label(instructions->tacs[i])) {
			TACLabel* label = create_label_entry(ctx, instructions->tacs[i], i);
			add_label_to_entries(ctx, label);
		}
		i++;
	}
}

int find_label_index(TACTable* instructions, char* target_name, int current_index) {
	if (!target_name) return -1;

	for (int i = current_index; i < instructions->size; i++) {
		if (!instructions->tacs[i]->result) continue;
		if (instructions->tacs[i]->result->kind != OP_LABEL) continue;
		if (!instructions->tacs[i]->result->value.label_name) continue;
		if (strcmp(target_name, instructions->tacs[i]->result->value.label_name) == 0) {
			printf("\033[31mFound label index: %d\033[0m\n", i);
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
			switch (instructions->tacs[current_index]->type) {
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
					int label_start = find_label_index(instructions, instructions->tacs[start + 2]->result->value.label_name, current_index + 1);
					if (label_start != -1) {
						add_leader_to_leader_list(ctx, label_start);
					}
					break;
				}

				case TAC_RETURN: {
					add_leader_to_leader_list(ctx, current_index + 1);
				}
				default: break;
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
		FunctionInfo* info = function_list->infos[i];
		info->cfg = create_cfg(ctx);

		if (!info->cfg) {
			return false;
		}

		BasicBlock* block = info->cfg->head;

		int start = info->tac_start_index;
		int end = info->tac_end_index;

		int leader_offset = start + 1;
		int remaining_instructions_offset = start + 2;
		add_instruction_to_block(ctx, block, instructions->tacs[leader_offset]);

		while (instructions->tacs[remaining_instructions_offset] && remaining_instructions_offset <= end) {			
			if (!index_is_leader(remaining_instructions_offset)) {
				add_instruction_to_block(ctx, block, instructions->tacs[remaining_instructions_offset]);
			} else {
				if (!add_block_to_cfg(ctx, info->cfg, block)) {
					return false;
				}

				BasicBlock* new_block = create_basic_block(ctx);
				block = new_block;
				add_instruction_to_block(ctx, block, instructions->tacs[remaining_instructions_offset]);
			}
			remaining_instructions_offset++;
		}

		if (!add_block_to_cfg(ctx, info->cfg, block)) {
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
			if (first->type == TAC_LABEL &&
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
		void ** reallocated_predecessors = arena_reallocate(
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
			TACInstruction* last_instruction_in_current_block = cfg->all_blocks[j]->instructions[cfg->all_blocks[j]->num_instructions - 1];
			if (!last_instruction_in_current_block) continue;

			switch (last_instruction_in_current_block->type) {
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
					
					BasicBlock* matching_block = find_matching_label_block(cfg, last_instruction_in_current_block->op1->value.label_name);
					if (matching_block) {
						add_edges(ctx, cfg, j, matching_block);
					}

					break;
				}
				
				case TAC_RETURN: {
					break;
				}

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
	table->liveness_infos = arena_allocate(
		ctx->ir_arena, 
		table->capacity * sizeof(LivenessInfo*)
	);

	if (!table->liveness_infos) return NULL;

	return table;
}

LivenessInfo* create_liveness_info(CompilerContext* ctx, LiveInfoVar var, bool is_live, int next_use) {
	LivenessInfo* live_info = arena_allocate(ctx->ir_arena, sizeof(LivenessInfo));
	if (!live_info) return NULL;

	live_info->var = var;
	live_info->is_live = is_live;
	live_info->next_use = next_use;
	return live_info;
}

bool add_liveinfo_to_liveness_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* live_info) {
	if (!live_info) return false;

	if (table->size >= table->capacity) {
		int prev_capacity = table->capacity;

		table->capacity *= 2;
		int new_capacity = table->capacity;
		void** new_live_infos = arena_reallocate(
			ctx->ir_arena,
			table->liveness_infos,
			prev_capacity * sizeof(LivenessInfo*),
			new_capacity * sizeof(LivenessInfo*)
		);

		if (!new_live_infos) return false;

		table->liveness_infos = new_live_infos;
	}

	table->liveness_infos[table->size++] = live_info;
	return true;

}

OperandSet* create_operand_set(CompilerContext* ctx) {
	OperandSet* op_set = arena_allocate(ctx->ir_arena, sizeof(OperandSet));
	if (!op_set) return NULL;

	op_set->size = 0;
	op_set->capacity = INIT_OP_SET_CAPACITY;
	op_set->elements = arena_allocate(ctx->ir_arena, sizeof(Operand*) * op_set->capacity);
	if (!op_set->elements) {
		perror("In 'create_op_set', unable to allocate space and initialize op set elements\n");
		return NULL;
	}
	return op_set;
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

void add_to_operand_set(CompilerContext* ctx, OperandSet* op_set, Operand* operand) {
	if (!op_set || !operand) return;

	if (op_set->size >= op_set->capacity) {
		int prev_capacity = op_set->capacity;

		op_set->capacity *= 2;
		int new_capacity = op_set->capacity;
		void** new_elements = arena_reallocate(
			ctx->ir_arena,
			op_set->elements,
			prev_capacity * sizeof(Operand*),
			new_capacity * sizeof(Operand*)
		);

		if (!new_elements) {
			perror("In 'add_to_operand_set', unable to reallocate space for new elements\n");
			return;
		}
		op_set->elements = new_elements;
	}
	op_set->elements[op_set->size++] = operand;
}

void populate_use_and_def_sets(CompilerContext* ctx, BasicBlock* block) {
	if (!block) return;

	for (int i = 0; i < block->num_instructions; i++) {
		TACInstruction* tac = block->instructions[i];
		if (!tac) return;

		switch (tac->type) {
			case TAC_ADD:
			case TAC_SUB:
			case TAC_MUL:
			case TAC_DIV:
			case TAC_ADD_EQUAL:
			case TAC_SUB_EQUAL:
			case TAC_MUL_EQUAL:
			case TAC_DIV_EQUAL:
			case TAC_MODULO:
			case TAC_LESS:
			case TAC_GREATER:
			case TAC_LESS_EQUAL:
			case TAC_GREATER_EQUAL:
			case TAC_EQUAL:
			case TAC_NOT_EQUAL:
			case TAC_LOGICAL_OR:
			case TAC_LOGICAL_AND: {
				add_to_operand_set(ctx, block->use_set, tac->op1);
				add_to_operand_set(ctx, block->use_set, tac->op2);
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_CHAR:
			case TAC_BOOL:
			case TAC_INTEGER: {
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_STORE: {
				add_to_operand_set(ctx, block->use_set, tac->result);
				add_to_operand_set(ctx, block->use_set, tac->op1);
				break;
			}

			case TAC_PARAM:
			case TAC_ARG: {
				add_to_operand_set(ctx, block->use_set, tac->op1);
				break;
			}

			case TAC_DEREFERENCE: 
			case TAC_NOT: {
				add_to_operand_set(ctx, block->use_set, tac->op1);
				add_to_operand_set(ctx, block->def_set, tac->result);
				break;
			}

			case TAC_ASSIGNMENT: {
				if (tac->op2 && tac->op2->kind == OP_RETURN) {
					add_to_operand_set(ctx, block->def_set, tac->result);
				} else {
					add_to_operand_set(ctx, block->use_set, tac->op2);
					add_to_operand_set(ctx, block->def_set, tac->result);
				}
				break;
			}
		}
	}

}

bool contains_operand(OperandSet* op_set, Operand* operand) {
	if (!op_set || !operand) return false;

	for (int i = 0; i < op_set->size; i++) {
		if (op_set->elements[i] == operand) return true;
	}

	return false;
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

OperandSet* copy_set(CompilerContext* ctx, OperandSet* original_set) {
	if (!original_set) return NULL;

	OperandSet* copy = create_operand_set(ctx);
	for (int i = 0; i < original_set->size; i++) {
		copy->elements[i] = original_set->elements[i];
	}
	copy->size = original_set->size;
	return copy;
}

void fixed_point_iteration(CompilerContext* ctx, CFG* cfg) {
	bool changed = true;
	while (changed) {
		changed = false;

		for (int i = cfg->num_blocks - 1; i >= 0; i--) {
			BasicBlock* current_block = cfg->all_blocks[i];
			
			current_block->out_set->size = 0;

			for (int j = 0;j < current_block->num_successors; j++) {			
				BasicBlock* successor = current_block->successors[j];
				union_sets(ctx, current_block->out_set, successor->in_set);
			}

			OperandSet* old_in = copy_set(ctx, current_block->in_set);

			OperandSet* out_minus_def = difference_sets(ctx, current_block->out_set, current_block->def_set);
			current_block->in_set->size = 0;
			union_sets(ctx, current_block->in_set, current_block->use_set);
			union_sets(ctx, current_block->in_set, out_minus_def);

			if (!sets_equal(old_in, current_block->in_set)) {
				changed = true;
			}
		}		
	}
}


int hash_variable(LivenessTable* table, char* name) {
	if (!table || !name) return -1;

	unsigned int hash = 0;
	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % table->capacity;

}


LivenessInfo* retrieve_livenessinfo(LivenessTable* table, int hash_key) {
	if (!table || hash_key == -1) return NULL;

	return table->liveness_infos[hash_key];
}

void determine_operand_liveness_and_next_use(CompilerContext* ctx, CFG* cfg, 
	BasicBlock* block, Operand* operand, operand_role role, int current_index) {

	switch (operand->kind) {
		case OP_SYMBOL: {
			LiveInfoVar var = {.symbol = operand->value.sym};

			int hash_key = hash_variable(cfg->table, operand->value.sym->name);	

			LivenessInfo* stored_info = NULL;
			switch (role) {	
				case OP_RESULT: {
					stored_info = retrieve_livenessinfo(block->table, hash_key); 
					if (stored_info) {
						stored_info->is_live = false;
					}
					break;
				}
				case OP_USE: {
					stored_info = retrieve_livenessinfo(block->table, hash_key); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, var, false, -1);
						add_liveinfo_to_liveness_table(ctx, block->table, new_info);	
						stored_info = new_info;
					} else {
						stored_info->next_use = current_index;
						stored_info->is_live = true;
					}

					operand->next_use = stored_info->next_use;
					operand->is_live = true; 

					
					break;
				}
			}

			break;
		}

		case OP_BINARY:
		case OP_UNARY:
		case OP_STORE: {
			if (!operand->value.label_name) return;

			LiveInfoVar var = {.label_name = operand->value.label_name};

			int hash_key = hash_variable(cfg->table, operand->value.label_name);
			

			LivenessInfo* stored_info = NULL;
			switch (role) {
				case OP_RESULT: {
					stored_info = retrieve_livenessinfo(block->table, hash_key); 
					if (stored_info) {
						stored_info->is_live = false;
					}
					break;
				case OP_USE: {
					stored_info = retrieve_livenessinfo(block->table, hash_key); 
					if (!stored_info) {
						LivenessInfo* new_info = create_liveness_info(ctx, var, false, -1);
						add_liveinfo_to_liveness_table(ctx, block->table, new_info);
						stored_info = new_info;
					} else {
						stored_info->next_use = current_index;
						stored_info->is_live = true;
					} 
					operand->next_use = stored_info->next_use;
					operand->is_live = true;
					
					stored_info->next_use = current_index;
					stored_info->is_live = true;
					break;
				} 
			}

			break;
		}

		default: break;
	}
	}
}

void determine_instruction_liveness_info(CompilerContext* ctx, CFG* cfg, BasicBlock* current_block, int current_index) {
	if (!current_block) return;

	TACInstruction* current_instruction = current_block->instructions[current_index];
	
	determine_operand_liveness_and_next_use(ctx, cfg, current_block, current_instruction->result, OP_RESULT, current_index);
	determine_operand_liveness_and_next_use(ctx, cfg, current_block, current_instruction->op1, OP_USE, current_index);
	determine_operand_liveness_and_next_use(ctx, cfg, current_block, current_instruction->op2, OP_USE, current_index);
	determine_operand_liveness_and_next_use(ctx, cfg, current_block, current_instruction->op3, OP_USE, current_index);
}


void determine_next_use(CompilerContext* ctx, CFG* cfg) {
	for (int i = cfg->num_blocks - 1; i >= 0; i--) {
		BasicBlock* current_block = cfg->all_blocks[i];
		current_block->table = create_liveness_table(ctx);
		if (!current_block->table) return;

		for (int j = 0; j < current_block->out_set->size; j++) {
			Operand* op = current_block->out_set->elements[j];
			if (!op) continue;

			switch (op->kind) {
				case OP_SYMBOL: {
					LiveInfoVar var = {.symbol = op->value.sym};
					LivenessInfo* live_info = create_liveness_info(ctx, var, true, -1);
					add_liveinfo_to_liveness_table(ctx, current_block->table, live_info);
					break;
				}

				case OP_UNARY:
				case OP_BINARY:
				case OP_STORE: {
					LiveInfoVar var = {.label_name = op->value.label_name};
					LivenessInfo* live_info = create_liveness_info(ctx, var, true, -1);
					add_liveinfo_to_liveness_table(ctx, current_block->table, live_info);
					break;
				}
			}
		}

		for (int k = current_block->num_instructions - 1; k>= 0; k--) {
			determine_instruction_liveness_info(ctx, cfg, current_block, k);
		}
		
	}
}

void live_analysis(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* current_block = cfg->all_blocks[j];
			if (!init_block_sets(ctx, current_block)) return;

			populate_use_and_def_sets(ctx, current_block);
		}

		fixed_point_iteration(ctx, cfg);
		determine_next_use(ctx, cfg);
	}
}
/// END
//////////////////////////////////////

bool is_operand_label_or_symbol(Operand* op) {
	if (!op) return false;

	switch (op->kind) {
		case OP_SYMBOL: return true;
		case OP_BINARY: return true;
		case OP_UNARY: return true;
		case OP_STORE: return true;
		default: return false;
	}
}

void emit_liveness_info(TACInstruction* instruction) {
	if (!instruction) return;

	char* live_state = NULL;

	if (is_operand_label_or_symbol(instruction->result)) {
		switch (instruction->result->kind) {
			case OP_SYMBOL: {
				live_state = (instruction->result->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->result->value.sym->name,
					live_state,
					instruction->result->next_use);
				break;
			}

			case OP_BINARY:
			case OP_UNARY:
			case OP_STORE: {
				live_state = (instruction->result->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->result->value.label_name, 
					live_state,
					instruction->result->next_use);
				break;
			}

		}
	}

	if (is_operand_label_or_symbol(instruction->op1)) {
		switch (instruction->op1->kind) {
			case OP_SYMBOL: {
				live_state = (instruction->op1->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->op1->value.sym->name,
					live_state,
					instruction->op1->next_use);
				break;
			}

			case OP_LABEL: {
				live_state = (instruction->op1->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->op1->value.label_name,
					live_state,
					instruction->op1->next_use);
				break;
			}
		}
	}

	if (is_operand_label_or_symbol(instruction->op2)) {
		switch (instruction->op2->kind) {
			case OP_SYMBOL: {
				live_state = (instruction->op2->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->op2->value.sym->name,
					live_state,
					instruction->op2->next_use);
				break;
			}

			case OP_LABEL: {
				live_state = (instruction->op2->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					instruction->op2->value.label_name,
					live_state,
					instruction->op2->next_use);
				break;
			}
		}
	}

	if (is_operand_label_or_symbol(instruction->op3)) {
		switch (instruction->op3->kind) {
			case OP_SYMBOL: {
				live_state = (instruction->op3->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n}",
					instruction->op3->value.sym->name,
					live_state,
					instruction->op3->next_use);
				break;
			}

			case OP_LABEL: {
				live_state = (instruction->op3->is_live) ? "true" : "false";
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n}",
					instruction->op3->value.label_name,
					live_state,
					instruction->op3->next_use);
				break;
			}
		}
	}
}

FunctionList* build_cfg(CompilerContext* ctx, TACTable* instructions) {
	if (!instructions) return;

	function_list = create_function_list(ctx);
	tac_entries = create_tac_label_entries(ctx);
	leaders_list = create_tac_leaders(ctx);
	if (!function_list || !tac_entries.labels || !leaders_list.leaders) return;
	
	mark_function_boundaries(ctx, instructions);
	mark_labels(ctx, instructions);
	find_leaders(ctx, instructions);
	make_function_cfgs(ctx, instructions);
	link_function_cfgs(ctx);

	// store_nonvirtual_variables(ctx); 
	live_analysis(ctx);
	printf("\n\n");
	printf("\033[31mLast Liveness check\033[0m\n");
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* current_block = cfg->all_blocks[j];

			for (int k = 0; k < current_block->num_instructions; k++) {
				emit_liveness_info(current_block->instructions[k]);
			}
		}
	}
	// emit_function_infos();
	// emit_leaders();
	// emit_blocks();

	return function_list;
}

void emit_function_infos() {
	for (int i = 0; i < function_list->size; i++) {
		printf("Function name: \033[32m%s\033[0m -> Start Index: %d -> End Index: %d\n", function_list->infos[i]->name, function_list->infos[i]->tac_start_index, function_list->infos[i]->tac_end_index);
	}
}

void emit_leaders() {
	for (int i = 0; i < leaders_list.size; i++) {
		printf("Leader index: \033[32m%d\033[0m\n", leaders_list.leaders[i]);
	}
}

void emit_blocks() {
	for (int i = 0; i < function_list->size; i++) {
		printf("\nFunction: \033[32m%s\033[0m\n", function_list->infos[i]->name);
		if (function_list->infos[i]->cfg) {
			for (int j = 0; j < function_list->infos[i]->cfg->num_blocks; j++) {
				printf("\tBlock %d:\n", j);
				BasicBlock* current_block = function_list->infos[i]->cfg->all_blocks[j];
				for (int k = 0; k < current_block->num_instructions; k++) {
					printf("\t\tTAC type %d\n", current_block->instructions[k]->type);
				}

			}			
		}
	}
}