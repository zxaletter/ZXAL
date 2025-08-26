#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "compilercontext.h"
#include "IR/cfg.h"

#define INIT_IDEALIZED_ASM_INSTRUCTIONS_CAPACITY 500
#define NUM_REGISTERS 14
#define ARG_OFFSET 2

bool is_restricted(int* restricted_regs, int restricted_regs_count, int reg);
void find_new_register(InterferenceBundle* bundle, int* remaining_registers, Operand* op);
void application_binary_interface(TACInstruction* tac, int* arg_index, int* param_index);
void pre_color_nodes(FunctionInfo* info);
int* get_remaining_registers(CompilerContext* ctx, int* restricted_regs, int restricted_regs_count);
void color_interference_graphs(CompilerContext* ctx, FunctionList* list);
void reg_alloc(CompilerContext* ctx, FunctionList* function_list);
void check_regs(FunctionList* function_list);

#endif