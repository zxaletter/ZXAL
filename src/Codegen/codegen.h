#ifndef CODEGEN_H
#define CODEGEN_H

#include "compilercontext.h"
#include "IR/cfg.h"
#include "symbols.h"
#include "string.h"

#define EIGHT_BYTE_ALIGNMENT 8
#define SIXTEEN_BYTE_ALIGNMENT 16

typedef enum {
	TRUE_LBL,
	FALSE_LBL,
	END_LBL
} jmp_label_t;

typedef struct {
	FILE* file;
	char* filename;
	size_t current_text_pos;
	size_t current_data_pos;
} ASMWriter;

char* generate_jmp_label(CompilerContext* ctx, jmp_label_t type);

void write_asm_to_file(ASMWriter* writer, char* text);

bool is_caller_saved(int reg);
bool is_callee_saved(int reg);

char* operator_to_string(tac_t type);

char* get_op_code(tac_t type);
void generate_jmp_asm(TACInstruction* tac, char* label, ASMWriter* writer);
void generate_logical_operator_asm(TACInstruction* instruction, Operand* jmp_op, ASMWriter* writer);
void generate_function_body(CompilerContext* ctx, CFG* cfg, ASMWriter* writer);
void generate_function_prologue(CompilerContext* ctx, FunctionInfo* info, ASMWriter* writer);

char* create_function_label(CompilerContext* ctx, char* func_name);
void emit_asm_for_functions(CompilerContext* ctx, FunctionList* function_list, ASMWriter* writer);
char* get_full_text(CompilerContext* ctx, char* func_name);
void generate_globals(CompilerContext* ctx, ASMWriter* writer);

void ensure_alignment(int* op_size, int alignment);
bool contains_operand_symbol(OperandSet* op_set, Symbol* target_symbol);
void get_bytes_from_operand(CompilerContext* ctx, OperandSet* symbols_set, Operand* op);
size_t get_size(Operand* operand);
void adjust_bytes_for_frame(CompilerContext* ctx, OperandSet* symbols_set, TACInstruction* instruction);
void accumulate_bytes(CompilerContext* ctx, CFG* cfg, OperandSet* symbols_set, Symbol* func_symbol);
void get_bytes_for_stack_frames(CompilerContext* ctx, FunctionList* function_list);

char* get_filename(CompilerContext* ctx, char* file);
ASMWriter* create_asm_writer(CompilerContext* ctx, char* file);
void codegen(CompilerContext* ctx, FunctionList* function_list, char* file);

#endif