#ifndef CODEGEN_H
#define CODEGEN_H

#include "compilercontext.h"
#include "IR/cfg.h"
#include "symbols.h"
#include <string.h>

#define EIGHT_BYTE_ALIGNMENT 8
#define SIXTEEN_BYTE_ALIGNMENT 16
#define INIT_ARG_LIST_CAPACITY 20
#define INIT_SPILL_SCHEDULE_CAPACITY 20
#define INIT_POP_SCHEDULE_CAPACITY 20

typedef enum {
	TRUE,
	FALSE,
	END
} jmp_label_t;

typedef enum {
	FIRST_OPERAND,
	SECOND_OPERAND
} op_check_t;

typedef struct {
	int call_instr_count;
	CallInstruction* c_instructions;
} CallInstructionList;

typedef struct {
	FILE* file;
	char* filename;
} ASMWriter;

void emit_reloads(ASMWriter* writer, ReloadBundle* bundle);
void emit_spills(ASMWriter* writer, SpillBundle* bundle);

SpillBundle* create_spill_bundle(CompilerContext* ctx, int size);
SpillBundle* gather_matching_spills(CompilerContext* ctx, SpillSchedule* spill_schedule, int push_index);
ReloadBundle* gather_matching_reloads(CompilerContext* ctx, ReloadSchedule* reload_schedule, int pop_index);
void add_reload(CompilerContext* ctx, ReloadSchedule* reload_schedule, Reload r);
void add_spill(CompilerContext* ctx, SpillSchedule* spill_schedule, Spill s);
bool contains_spill(SpillSchedule* schedule, Spill s);
ReloadBundle* create_reload_bundle(CompilerContext* ctx, int size);

void schedule_callee_reloads(CompilerContext* ctx, SpillSchedule* spill_schedule, ReloadBundle* reload_bundle);
int find_corrupting_instr_index(BasicBlock* block, op_check_t type, Operand* op, int start);
int verify_liveness(BasicBlock* block, Operand* op, int start);

ReloadSchedule* create_reload_schedule(CompilerContext* ctx);
SpillSchedule* create_spill_schedule(CompilerContext* ctx);
void schedule_spills_and_reloads(CompilerContext* ctx, FunctionList* function_list);

bool find_call_instr(BasicBlock* block);
ArgumentList* find_arg_list(BasicBlock* block, TACInstruction* tac);
CallInstruction* create_call_instr(CompilerContext* ctx, char* func_name, int instr_id);
CallInstructionList* populate_call_instr_list(CompilerContext* ctx, BasicBlock* block);
CallInstructionList* create_call_instr_list(CompilerContext* ctx, int call_instr_count);

void add_to_arg_list(CompilerContext* ctx, ArgumentList* arg_list, ArgumentInfo* arg);
ArgumentInfo* create_arg_info(CompilerContext* ctx, arg_location loc, TACInstruction* tac);
ArgumentList* create_arg_list(CompilerContext* ctx, CallInstruction* call_instr);
StructuredArgs* create_sargs(CompilerContext* ctx, CallInstructionList* c_instr_list);

char* generate_jmp_label(CompilerContext* ctx, jmp_label_t type);

void write_asm_to_file(ASMWriter* writer, char* text);

bool is_caller_saved(int reg);
bool is_callee_saved(int reg);

char* operator_to_string(tac_t type);
char* get_op_code(tac_t type);

// void add_site(CompilerContext* ctx, CallGraph* call_graph, CallSite site);
// CallGraph* create_call_graph(CompilerContext* ctx);
// void form_call_sites(CompilerContext* ctx, FunctionList* function_list);

void collect_args(CompilerContext* ctx, FunctionInfo* info);
void generate_corresponding_jump(ASMWriter* writer, tac_t kind, char* jmp_label);
void generate_function_body(CompilerContext* ctx, ASMWriter* writer, FunctionInfo* info);
void schedule_callee_register_spills(CompilerContext* ctx, FunctionList* function_list);
void generate_function_prologue(CompilerContext* ctx, ASMWriter* writer, FunctionInfo* info);

char* create_function_label(CompilerContext* ctx, char* func_name);
void emit_asm_for_functions(CompilerContext* ctx, ASMWriter* writer,FunctionList* function_list);
char* get_full_text(CompilerContext* ctx, char* func_name);
void generate_globals(CompilerContext* ctx, ASMWriter* writer);
void ensure_main_function_exists(CompilerContext* ctx);

Operand* find_non_restricted_operand(OperandSet* op_set, Operand* arg_op);
Operand* operand_with_furthest_use(OperandSet* op_set);
Operand* find_matching_register(OperandSet* op_set, int reg);
void ensure_alignment(int* op_size, int alignment);
bool contains_operand_symbol(OperandSet* op_set, Symbol* target_symbol);
void get_bytes_from_operand(CompilerContext* ctx, OperandSet* symbols_set, Operand* op, int* total_frame_bytes);
size_t get_size(Operand* operand);

int determine_operand_use(BasicBlock* block, Operand* op, int start);
int determine_operand_use_within_call_boundary(BasicBlock* block, Operand* op, int start, int end);
int find_call_instr_index(BasicBlock* block, int start);
void get_bytes_for_stack_frames(CompilerContext* ctx, FunctionList* function_list);

char* get_filename(CompilerContext* ctx, char* file);
ASMWriter* create_asm_writer(CompilerContext* ctx, char* file);
void generate_executable(CompilerContext* ctx, char* asm_file);
void codegen(CompilerContext* ctx, FunctionList* function_list, char* file);

#endif