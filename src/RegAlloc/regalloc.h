#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "compilercontext.h"
#include "IR/cfg.h"

#define INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY 500

typedef enum {
	ADD,
	SUB,
	MUL,
	DIV,
	STORE,
	CMP
} idealized_asm_instruction_t;

typedef struct {
	char* name;
	Operand* operand;
} VirtualRegister;

typedef struct {
	idealized_asm_instruction_t kind;
	VirtualRegister dest;
	VirtualRegister src1;
	VirtualRegister src2
} IdealizedASMInstruction;

typedef struct {
	int size;
	int capacity;
	IdealizedASMInstruction* instructions;	
} IdealizedASMInstructionList;

bool store_idealized_asm_instruciton(CompilerContext* ctx, IdealizedASMInstruction instruciton);
char* create_virtual_register_name(CompilerContext* ctx);
VirtualRegister create_virtual_register(CompilerContext* ctx, Operand* op);
idealized_asm_instruction_t get_idealized_asm_instruction_type(TACInstruction* tac);
IdealizedASMInstruction create_idealized_asm_instruction(idealized_asm_instruction_t kind, VirtualRegister dest, VirtualRegister src1, VirtualRegister src2);
IdealizedASMInstructionList create_idealized_asm_instruction_list(CompilerContext* ctx);

void create_interference_graph(CompilerContext* ctx);
void create_idealized_asm_for_tac_instruction(CompilerContext* ctx, TACInstruction* tac);
void create_idealized_asm_for_block(CompilerContext* ctx, BasicBlock* block);
void create_idealized_asm_for_functions(CompilerContext* ctx, FunctionList* function_list);
void reg_alloc(CompilerContext* ctx, FunctionList* function_list);

#endif