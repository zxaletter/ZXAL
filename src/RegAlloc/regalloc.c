#include "regalloc.h"

IdealizedASMInstruction* create_idealized_asm_instruction(CompilerContext* ctx, idealized_asm_instruction_t kind, 
	Operand* op1, Operand* op2, Operand* op3, LivenessInfo* info) {
	
	IdealizedASMInstruction* instruction = arena_allocate(ctx->ir_arena, sizeof(IdealizedASMInstruction));
	if (!instruction) return NULL;

	instruction->kind = kind;
	instruction->op1 = op1;
	instruction->op2 = op2;
	instruction->op3 = op3;
	instruction->info = info;
	return instruction;
}

void create_idealized_asm(CompilerContext* ctx, FunctionList* function_list) {
	for (int i = 0; i < function_list->size; i++) {
		Function
	}
}

void reg_alloc(CompilerContext* ctx, FunctionList* function_list) {
	if (!function_list) return;

	create_idealized_asm(ctx, function_list);
	create_interference_graph();
}