#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "compilercontext.h"
#include "IR/cfg.h"

#define INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY 500

typedef enum {
	TRUE,
	FALSE,
	END
} label_t;

typedef enum {
	ADD,
	SUB,
	MUL,
	DIV,
	MODULO,
	STORE,
	DEREFERENCE,
	ASSIGN,
	MOV,
	MOV_ARG,
	CMP,
	CALL,
	RETURN,
	ARG,
	PARAM,
	JMP,
	JMP_L,
	JMP_G,
	JMP_E,
	JMP_NE,

	LABEL
} OpCode;

typedef struct {
	char* name;
	Operand* operand;
} VirtualRegister;

typedef struct {
	OpCode op;
	Symbol* symbol;
	Operand* dest;
	Operand* src1;
	Operand* src2;
	int immediate_val;
} IdealizedASMInstruction;

typedef struct {
	int size;
	int capacity;
	IdealizedASMInstruction* instructions;	
} IdealizedASMInstructionList;

char* generate_jmp_label(CompilerContext* ctx, label_t type);
bool is_int_literal(operand_t type);
bool is_symbol(operand_t type);
bool store_idealized_asm_instruciton(CompilerContext* ctx, IdealizedASMInstruction instruciton);
VirtualRegister* create_virtual_register(CompilerContext* ctx, Operand* op);
OpCode get_op_code(TACInstruction* tac);
IdealizedASMInstruction create_idealized_asm_instruction(
	OpCode op, 
	Symbol* symbol, 
	Operand* dest,
	Operand* src1,
	Operand* src2,
	int immediate_val
);
IdealizedASMInstructionList create_idealized_asm_instruction_list(CompilerContext* ctx);

void create_interference_graph(CompilerContext* ctx);
VirtualRegister* create_idealized_asm_for_tac_instruction(CompilerContext* ctx, TACInstruction* tac);
void create_idealized_asm_for_block(CompilerContext* ctx, BasicBlock* block);
void create_idealized_asm(CompilerContext* ctx, FunctionList* function_list);
void reg_alloc(CompilerContext* ctx, FunctionList* function_list);

#endif