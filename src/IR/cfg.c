#include "cfg.h"

FunctionList* function_list = NULL;
TACLabelEntries tac_entries;
TACLeaders leaders_list;

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
		void** new_infos = arena_reallocate(ctx->ir_arena, function_list->infos, prev_capacity, new_capacity);
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
		void** new_labels = arena_reallocate(ctx->ir_arena, tac_entries.labels, prev_capacity, new_capacity);
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
		void* new_leaders = arena_reallocate(ctx->ir_arena, leaders_list.leaders, prev_capacity, new_capacity);
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
		void** new_blocks = arena_reallocate(ctx->ir_arena, cfg->all_blocks, prev_capacity, new_capacity);
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

		void** new_instructions = arena_reallocate(ctx->ir_arena, block->instructions, prev_capacity, new_capacity);
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

BasicBlock* find_matching_label_block(CFG* cfg, int starting_search_index, TACInstruction* instruction) {
	for (int i = starting_search_index; i < cfg->num_blocks; i++) {
		BasicBlock* current_block = cfg->all_blocks[i];
		if (!current_block->instructions[0]->result) continue;
		if (!current_block->instructions[0]->result->value.label_name) continue;
		if (strcmp(instruction->result->value.label_name, current_block->instructions[0]->result->value.label_name) == 0) {
			return current_block;
		}
	}
	return NULL;
}

void add_edges(CFG* cfg, int index, BasicBlock* block) {
	if (!block) return;
	cfg->all_blocks[index]->successors[cfg->all_blocks[index]->num_successors++] = block;
	block->predecessors[block->num_predecessors++] = cfg->all_blocks[index];
}

void link_function_cfgs() {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		if (!cfg) return;

		for (int j = 0; j < cfg->num_blocks; j++) {
			TACInstruction* last_instruction_in_current_block = cfg->all_blocks[j]->instructions[cfg->all_blocks[j]->num_instructions - 1];
			if (!last_instruction_in_current_block) continue;

			switch (last_instruction_in_current_block->type) {
				case TAC_GOTO: {
					BasicBlock* matching_block = find_matching_label_block(cfg, j + 1, last_instruction_in_current_block);
					add_edges(cfg, j, matching_block);
					break;
				}

				case TAC_IF_FALSE: {
					BasicBlock* matching_block = find_matching_label_block(cfg, j + 1, last_instruction_in_current_block);
					add_edges(cfg, j, matching_block);
					add_edges(cfg, j, cfg->all_blocks[j + 1]);
					break;
				}
				
				case TAC_RETURN: {
					break;
				}

				default: {
					add_edges(cfg, j, cfg->all_blocks[j + 1]);
					break;
				}
			}
		}
	}
}


// live analysis functions
LivenessTable* create_liveness_table(CompilerContext* ctx) {
	LivenessTable* live_table = arena_allocate(ctx->ir_arena, sizeof(LivenessTable));
	if (!live_table) {
		perror("In 'create_liveness_table', unable to allocate space and initialize live table\n");
		return NULL;
	}
	live_table->size = 0;
	live_table->capacity = INIT_LIVENESS_TABLE_CAPACITY;
	live_table->liveness_infos = arena_allocate(ctx->ir_arena, sizeof(LivenessInfo*) * live_table->capacity);
	if (!live_table->liveness_infos) {
		perror("In 'create_liveness_table', unable to allocate space for liveness infos\n");
		return NULL;
	}
	return live_table;
}

LivenessInfo* create_liveness_info(CompilerContext* ctx, LiveInfoVar var, bool is_live, int next_use) {
	LivenessInfo* live_info = arena_allocate(ctx->ir_arena, sizeof(LivenessInfo));
	if (!live_info) return NULL;

	live_info->var = var;
	live_info->is_live = is_live;
	live_info->next_use = next_use;
	return live_info;
}

int hash_variable(BasicBlock* block, char* name) { 
	if (!name) return -1; 

	int hash = 0;

	while (*name) {
		hash = (hash << 1) + *name++;
	}
	return hash % block->table->capacity;
}

bool bind_or_update_live_info_to_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* info, unsigned int hash_key) {
	if (!table || !info) return false;

	if (table->size >= table->capacity) {
		int prev_capacity = table->capacity;

		table->capacity *= 2;
		int new_capacity = table->capacity;
		void** new_liveness_infos = arena_reallocate(ctx->ir_arena, table->liveness_infos, prev_capacity, new_capacity);
		if (!new_liveness_infos) {
			return false;
		}
		table->liveness_infos = new_liveness_infos;
	}
	
	if (table->liveness_infos[hash_key]) {
		table->liveness_infos[hash_key] = info;
	} else {
		table->liveness_infos[hash_key] = info;
		table->size++;
	}
	return true;
}

void set_operand_live_info(Operand* op, LivenessInfo* live_info) {
	op->is_live = live_info->is_live;
	op->next_use = live_info->next_use;
}

void determine_operand_liveness_and_next_use(CompilerContext* ctx, BasicBlock* block, Operand* operand, operand_role role, int instruction_index) {
	if (!block || !operand) return;

	switch (operand->kind) {
		case OP_SYMBOL: {
			if (!operand->value.sym) return;
			if (!operand->value.sym->name) return;

			LiveInfoVar var = {
				.symbol = operand->value.sym
			};

			LivenessInfo* live_info = NULL;

			switch (role) {
				case OP_RESULT: live_info = create_liveness_info(ctx, var, false, -1); break;
				case OP_USE: live_info = create_liveness_info(ctx, var, true, instruction_index); break;
			}

			if (!live_info) return;

			int hash_index = hash_variable(block, operand->value.sym->name);
			if (hash_index != -1) {
				set_operand_live_info(operand, live_info);
				bind_or_update_live_info_to_table(ctx, block->table, live_info, hash_index);
			}
			break;
		}

		case OP_LABEL: {
			if (!operand->value.label_name) return;

			LiveInfoVar var = {
				.label_name = operand->value.label_name
			};

			LivenessInfo* live_info = NULL;

			switch (role) {
				case OP_RESULT: live_info = create_liveness_info(ctx, var, false, -1); break;
				case OP_USE: live_info = create_liveness_info(ctx, var, true, instruction_index); break;
			}

			if (!live_info) return;
			
			int hash_index = hash_variable(block, operand->value.label_name);
			if (hash_index != -1) {
				set_operand_live_info(operand, live_info);
				bind_or_update_live_info_to_table(ctx, block->table, live_info, hash_index);
			}
			break;
		}
	}
}

void attach_liveness_and_next_use(CompilerContext* ctx, BasicBlock* block, int instruction_index) {
	if (!block) return;


	TACInstruction* current_instruction = block->instructions[instruction_index];
	if (!current_instruction) return;

	determine_operand_liveness_and_next_use(ctx, block, current_instruction->result, OP_RESULT, instruction_index);
	determine_operand_liveness_and_next_use(ctx, block, current_instruction->op1, OP_USE, instruction_index);
	determine_operand_liveness_and_next_use(ctx, block, current_instruction->op2, OP_USE, instruction_index);
	determine_operand_liveness_and_next_use(ctx, block, current_instruction->op3, OP_USE, instruction_index);
}

void live_analysis(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* current_block = cfg->all_blocks[j];

			for (int k = current_block->num_instructions; k >= 0; k--) {
				attach_liveness_and_next_use(ctx, current_block, k);
			}
		}
	}
}
// 

void operand_contain_nonvirtual_variable(CompilerContext* ctx, BasicBlock* block, Operand* op) {
	if (!op) return;

	switch (op->kind) {
		case OP_SYMBOL: {
			LiveInfoVar var = { .symbol = op->value.sym }; 
			LivenessInfo* live_info = create_liveness_info(ctx, var, true, -1);
			if (!live_info) {
				printf("In 'operand_contain_nonvirtual_variable', unable to create live info\n");
				return;
			}
			int live_info_key = hash_variable(block, live_info->var.symbol->name);
			set_operand_live_info(op, live_info);
			char* live_state = (op->is_live) ? "true" : "false";
			printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n",
					op->value.sym->name,
					live_state,
					op->next_use); 
			break;
		}

		default: break;
	}
}

void instruction_contains_nonvirtual_variables(CompilerContext* ctx, BasicBlock* block, int current_index) {
	if (!block) return;

	operand_contain_nonvirtual_variable(ctx, block, block->instructions[current_index]->result);
	operand_contain_nonvirtual_variable(ctx, block, block->instructions[current_index]->op1);
	operand_contain_nonvirtual_variable(ctx, block, block->instructions[current_index]->op2);
	operand_contain_nonvirtual_variable(ctx, block, block->instructions[current_index]->op3);
}

void store_nonvirtual_variables(CompilerContext* ctx) {
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		CFG* cfg = info->cfg;

		for (int j = 0; j < cfg->num_blocks; j++) {
			BasicBlock* current_block = cfg->all_blocks[j];	
			current_block->table = create_liveness_table(ctx);
			if (!current_block->table) return;

			for (int k = 0; k < current_block->num_instructions; k++) {
				instruction_contains_nonvirtual_variables(
					ctx, 
					current_block,
					k
				);
			}
		}
	}

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
}

bool is_operand_label_or_symbol(Operand* op) {
	if (!op) return false;

	switch (op->kind) {
		case OP_SYMBOL: return true;
		case OP_LABEL: return true;
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

			case OP_LABEL: {
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
				printf("{Variable: \033[32m%s\033[0m, live=\033[32m%s\033[0m, Next use=\033[32m%d\033[0m}\n}",
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
	link_function_cfgs();

	store_nonvirtual_variables(ctx); 
	
	live_analysis(ctx);

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