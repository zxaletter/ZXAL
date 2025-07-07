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

IdealizedASMInstruction create_idealized_asm_instruction(OpCode op, char* function_name, 
	VirtualRegister* dest, VirtualRegister* src1, VirtualRegister* src2) {
	
	IdealizedASMInstruction instruction = {
		.op = op,
		.function_name = function_name,
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

VirtualRegister* create_virtual_register(CompilerContext* ctx, Operand* operand) {
	VirtualRegister* vreg = arena_allocate(ctx->ir_arena, sizeof(VirtualRegister));
	if (!vreg) {
		perror("In 'create_virtual_register', unable to allocate space for virtual register\n");
		return NULL;
	}

	char* virtual_register_name = create_virtual_register_name(ctx);
	if (!virtual_register_name) NULL;
	

	vreg->name = virtual_register_name;
	vreg->operand = operand;
	return vreg;
}

OpCode get_op_code(TACInstruction* tac) {
	switch (tac->type) {
		case TAC_ADD: return ADD;
		case TAC_SUB: return SUB;
		case TAC_MUL: return MUL;
		case TAC_DIV: return DIV;
	}
}

VirtualRegister* create_idealized_asm_for_tac_instruction(CompilerContext* ctx, TACInstruction* tac) {
	if (!tac) return;

	VirtualRegister* result = NULL;

	switch (tac->type) {
		case TAC_ADD:
		case TAC_SUB:
		case TAC_MUL:
		case TAC_DIV: {
			VirtualRegister* src1 = create_idealized_asm_for_tac_instruction(ctx, tac->op1);
			VirtualRegister* src2 = create_idealized_asm_for_tac_instruction(ctx, tac->op2);
			
			VirtualRegister* dest = create_virtual_register(ctx, tac->result);
			OpCode op = get_op_code(tac);

			IdealizedASMInstruction arithmetic_instruction = create_idealized_asm_instruction(op, NULL, dest, src1, src2);
			if (!store_idealized_asm_instruction(ctx, arithmetic_instruction)) return;
			result = dest;
			break;
		}

		case TAC_INTEGER:
		case TAC_BOOL:
		case TAC_CHAR: {

			VirtualRegister* src1 = create_virtual_register(ctx, tac->op1);

			
			VirtualRegister* dest = create_virtual_register(ctx, tac->result);
			IdealizedASMInstruction mov_instruction = create_idealized_asm_instruction(MOV, NULL, dest, src1, NULL);
			if (!store_idealized_asm_instruction(ctx, mov_instruction)) return;
			result = dest;
			break;
		}
	}

	return result;
}

void create_idealized_asm_for_block(CompilerContext* ctx, BasicBlock* block) {
	for (int i = 0; i < block->num_instructions; i++) {
		create_idealized_asm_for_tac_instruction(ctx, block->instructions[i]);
	}
}

char* get_idealized_asm_instruction_type_name(OpCode op) {
	switch (op) {
		case ADD: return "ADD";
		case SUB: return "SUB";
		case MUL: return "MUL";
		case DIV: return "DIV";
	}
}

void emit_idealized_asm_instructions() {
	for (int i = 0; i < asm_instruction_list.size; i++) {
		IdealizedASMInstruction current_instruction = asm_instruction_list.instructions[i];
		switch (current_instruction.op) {
			case ADD: 
			case SUB: 
			case MUL: 
			case DIV: {
				char* idealized_asm_instruction_type_name = get_idealized_asm_instruction_type_name(current_instruction.op);
				printf("\t%s %s, %s, %s\n",
					idealized_asm_instruction_type_name,
					current_instruction.dest->name,
					current_instruction.src1->name,
					current_instruction.src2->name);


				break;
			}

			case MOV: {
				printf("\tMOV %s, %s\n",
					current_instruction.dest->name,
					current_instruction.src1->name);
				break;
			}
		}
	}
}

void create_idealized_asm_for_functions(CompilerContext* ctx, FunctionList* function_list) {
	if (function_list) {
		printf("In 'create_idealized_asm_for_functions', we have a valid function list\n");
	}
	for (int i = 0; i < function_list->size; i++) {
		FunctionInfo* info = function_list->infos[i];
		
		IdealizedASMInstruction function_name = create_idealized_asm_instruction(LABEL, info->name, NULL, NULL, NULL);
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

	create_idealized_asm_for_functions(ctx, function_list);
	emit_idealized_asm_instructions();

	// create_interference_graph();
}