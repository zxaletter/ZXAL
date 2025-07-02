#include "cfg.h"

FunctionList function_list;
TACLabelEntries tac_entries;
TACLeaders leaders_list;

FunctionList create_function_list(CompilerContext* ctx) {
	FunctionInfo** infos = arena_allocate(ctx->ir_arena, sizeof(FunctionInfo*) * INIT_FUNCTION_LIST_CAPACITY);
	if (!infos) {
		FunctionList dummy_list = {
			.size = 0,
			.capacity = 0,
			.infos = NULL
		};
		return dummy_list;
	}

	FunctionList new_func_list = {
		.size = 0,
		.capacity = INIT_FUNCTION_LIST_CAPACITY,
		.infos = infos
	};
	return new_func_list;
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

	if (list.size >= list.capacity) {
		int prev_capacity = list.capacity;
		list.capacity *= 2;
		int new_capacity = list.capacity;
		void* new_infos = arena_reallocate(ctx->ir_arena, list.infos, prev_capacity, new_capacity);
		if (!new_infos) {
			return false;
		}
		list.infos = new_infos;
	}
	list.infos[list.size++] = info;
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

	if (entries.size >= entries.capacity) {
		int prev_capacity = entries.capacity;
		
		entries.capacity *= 2;
		int new_capacity = entries.capacity;
		void* new_labels = arena_reallocate(ctx->ir_arena, entries.labels, prev_capacity, new_capacity);
		if (!new_labels) {
			printf("Error: unable to reallocate space for new labels\n");
			return false;
		}
		entries.labels = new_labels;
	}
	entries.labels[entries.size++] = label;
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

	if (leader_list.size >= leaders_list.capacity) {
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

bool store_label(CompilerContext* ctx, TACInstruction* instruction, int tac_index) {
	if (!instruction) return false;

	switch (instruction->type) {
		case TAC_IF_FALSE: {
			TACLabel* new_label_entry = create_label_entry(ctx, instruction->op1->value.label_name, tac_index);
			if (!add_label_to_entries(ctx, new_label_entry)) return false; 
			break;
		}
		case TAC_LABEL:
		case TAC_GOTO: {
			TACLabel* new_label_entry = create_label_entry(ctx, instruction->result->value.label_name, tac_index);
			if (!add_label_to_entries(ctx, new_label_entry)) return false;
			break;
		}
		default: {
			printf("Unknown tac instruction type\n");
			break;
		}
	}
	return true;
}

bool add_instruction_to_block(CompilerContext* ctx, BasicBlock* block, TACInstruction* instruction) {
	if (!block || !instruction) return false;

	if (block->num_instructions >= block->num_instructions_capacity) {
		int prev_capacity = block->num_instructions_capacity;
		
		block->num_instructions_capacity *= 2;
		int new_capacity = block->num_instructions_capacity;

		void* new_instructions = arena_reallocate(ctx->ir_arena, block->instructions, prev_capacity, new_capacity);
		if (!new_instructions) return false;

		block->instructions = new_instructions;
	}
	block->instructions[block->num_instructions++] = instruction;
	return true;
}

void build_function_cfg(CompilerContext* ctx, TACTable* instructions, FunctionInfo* info) {
	if (!info) return;

	info->cfg = create_cfg(ctx);
	if (!info->cfg) return;

	BasicBlock* block = info->cfg->head;
	if (!add_instruction_to_block(ctx, block, instructions[info->tac_start_index])) return;
	
	for (int i = info->tac_start_index + 1; i < info->tac_end_index; i++) {
		switch (instructions->tacs[i]->type) {
			case TAC_IF_FALSE: {

				break;
			}

			default: {

			}
		}
	}	
}

void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions) {
	int i = 0;
	while (instructions->tacs[i] && i < instructions->size) {
		if (found_function(instructions->tacs[i])) {
			TACInstruction* func_tac = instructions->tacs[i];
			int tac_start_index = i;
			int j = i + 1;
			while (!found_function(instructions->tacs[j]) && j < instructions->size) { j++; }
			int tac_end_index = j - 1;
		
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

void find_leaders(CompilerContext* ctx, TACTable* instructions) {
	int i = 0;

	while (list.infos[i] && i < list.size) {
		int start = list.infos[i]->tac_start_index;
		int end = list.infos[i]->tac_end_index;

		add_leader(ctx, start + 1);
		while (instructions->tacs[start + 2] && ( start + 2 <= end )) {
			switch (instructions->tacs[start + 2]) {
				case TAC_IF_FALSE:				
				case TAC_GOTO:
				case TAC_LABEL: {
					add_leader(ctx, start + 1);
					break;
				}
				default: break;
			}

			start++;
		}		
		i++;
	}
}

void build_cfg(CompilerContext* ctx, TACTable* instructions) {
	if (!instructions) return;

	function_list = create_function_list(ctx);
	tac_entries = create_tac_label_entries(ctx);
	leaders_list = create_tac_leaders(ctx);
	if (!function_list.infos || !tac_entries.labels || !leaders_list.leaders) return;
	
	mark_function_boundaries(ctx, instructions);
	mark_labels(ctx, instructions);
	find_leaders(ctx, instructions);


	for (int k = 0; k < list->size; k++) {
		build_function_cfg(ctx, instructions, list->infos[k]);
	}
	
	emit_function_infos();
	emit_leaders();
}

void emit_function_infos() {
	for (int i = 0; i < list.size; i++) {
		printf("Function name: \033[32m%s\033[0m -> Start Index: %d -> End Index: %d\n", list.infos[i]->name, list.infos[i]->tac_start_index, list.infos[i]->tac_end_index);
	}
}

void emit_leaders() {
	for (int i = 0; i < leaders_list.size; i++) {
		printf("Leader index: \033[32m%d\033[0m\n", leaders_list.leaders[i]);
	}
}