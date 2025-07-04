#include "regalloc.h"

static int virtual_register_index = 1;
IdealizedASMInstructionList asm_instruction_list;

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
		void* new_asm_instructions = arena_reallocate(ctx->ir_arena, asm_instruction_list.instructions, prev_capacity, new_capacity);
		if (!new_asm_instructions) {
			printf("In 'store_idealized_asm_instruction', unable to reallocate space for new asm instructions\n");
			return false;
		}
	}
	asm_instruction_list.instructions[asm_instruction_list.size++] = instruction;
	return true;
}

IdealizedASMInstruction create_idealized_asm_instruction(idealized_asm_instruction_t kind, 
	VirtualRegister dest, VirtualRegister src1, VirtualRegister src2) {
	
	IdealizedASMInstruction instruction = {
		.kind = kind,
		.dest = dest,
		.src1 = src1,
		.src2 = src2
	};
	return instruction;
}

char* create_virtual_register_name(CompilerContext* ctx) {
	char* virtual_register_name = arena_allocate(ctx->ir_arena, 50);
	if (!virtual_register_name) {
		return NULL;
	}
	snprintf(virtual_register_name, 50, "vr%d", virtual_register_index++);
	return virtual_register_name;
}

VirtualRegister create_virtual_register(CompilerContext* ctx, Operand* op) {
	VirtualRegister dummy_vreg = {
		.name = NULL,
		.operand = NULL
	};

	if (!op) return dummy_vreg;

	char* virtual_register_name = create_virtual_register_name(ctx);
	if (!virtual_register_name) return dummy_vreg;
	
	VirtualRegister vreg = {
		.name = virtual_register_name,
		.operand = op
	};
	return vreg;
}

idealized_asm_instruction_t get_idealized_asm_instruction_type(TACInstruction* tac) {
	switch (tac->type) {
		case TAC_ADD: return ADD;
		case TAC_SUB: return SUB;
		case TAC_MUL: return MUL;
		case TAC_DIV: return DIV;
	}
}

void create_idealized_asm_for_tac_instruction(CompilerContext* ctx, TACInstruction* tac) {
	switch (tac->type) {
		case TAC_ADD:
		case TAC_SUB:
		case TAC_MUL:
		case TAC_DIV: {
			VirtualRegister dest = create_virtual_register(ctx, tac->result);
			VirtualRegister src1 = create_virtual_register(ctx, tac->op1);
			VirtualRegister src2 = create_virtual_register(ctx, tac->op2);

			idealized_asm_instruction_t kind = get_idealized_asm_instruction_type(tac);

			IdealizedASMInstruction arithmetic_instruction = create_idealized_asm_instruction(kind, dest, src1, src2);
			if (!store_idealized_asm_instruction(ctx, arithmetic_instruction)) return;
			break;
		}
	}
}

void create_idealized_asm_for_block(CompilerContext* ctx, BasicBlock* block) {
	for (int i = 0; i < block->num_instructions; i++) {
		create_idealized_asm_for_tac_instruction(ctx, block->instructions[i]);
	}
}

void create_idealized_asm_for_functions(CompilerContext* ctx, FunctionList* function_list) {
	if (function_list) {
		printf("In 'create_idealized_asm_for_functions', we have a valid function list\n");
	}
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
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

	create_idealized_asm_for_functions(ctx, function_list);
	// create_interference_graph();
}