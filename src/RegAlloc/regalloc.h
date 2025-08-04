#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "compilercontext.h"
#include "IR/cfg.h"

#define INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY 500
#define NUM_REGISTERS 14
#define ARG_OFFSET 2

void ensure_calling_convention(TACInstruction* tac, int* arg_index, int* param_index);
void pre_color_nodes(FunctionInfo* info);
void color_interference_graphs(CompilerContext* ctx, FunctionList* list);
void check_regs(FunctionList* function_list);


void reg_alloc(CompilerContext* ctx, FunctionList* function_list);

#endif