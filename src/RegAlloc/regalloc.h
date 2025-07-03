#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "compilercontext.h"
#include "IR/cfg.h"

typedef enum {
	ADD,
	SUB,
	MUL,
	DIV,
	CMP
} idealized_asm_instruction_t;


typedef struct {
	idealized_asm_instruction_t kind;
	Operand* op1;
	Operand* op2;
	Operand* op3;
	LivenessInfo* info;
} IdealizedASMInstruction;

typedef struct {

}

typedef struct {
	int size;
	int capacity;
	IdealizedASMInstructions** instructions;	
} IdealizedASMInstructionList;

void create_interference_graph(CompilerContext* ctx);
void create_idealized_asm(CompilerContext* ctx)
void reg_alloc(CompilerContext* ctx, FunctionList* function_list);

#endif