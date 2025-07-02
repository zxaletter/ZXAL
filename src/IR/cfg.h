#ifndef CFG_H
#define CFG_H

#include "compilercontext.h"
#include "tac.h"

#define INIT_FUNCTION_LIST_CAPACITY 40
#define INIT_TAC_INSTRUCTIONS_CAPACITY 50
#define INIT_PREDECESSOR_CAPACITY 20
#define INIT_SUCCESSORS_CAPACITY 20
#define INIT_TAC_LABEL_ENTRIES_CAPACITY 100
#define INIT_LEADERS_CAPACITY 100

typedef struct {
	char* name;
	int tac_index;
} TACLabel;

typedef struct {
	int size;
	int capacity;
	TACLabel** labels;
} TACLabelEntries;

typedef struct {
	int size;
	int capacity;
	int* leaders;
} TACLeaders;

typedef struct BasicBlock BasicBlock;
typedef struct BasicBlock {
	int num_instructions;
	int num_predecessors;
	int num_successors;

	int num_instructions_capacity;
	int num_predecessors_capacity;
	int num_successors_capacity;
	bool visited;
	TACInstruction** instructions;
	BasicBlock** predecessors;
	BasicBlock** successors;
} BasicBlock;

typedef struct {
	BasicBlock* head;
	int num_blocks;
	int blocks_capacity;
	BasicBlock** all_blocks;
} CFG;

typedef struct {
	char* name;
	int tac_start_index;
	int tac_end_index;
	CFG* cfg;
} FunctionInfo;

typedef struct {
	int size;
	int capacity;
	FunctionInfo** infos;
} FunctionList;

void emit_leaders();
void add_leader(CompilerContext* ctx, int leader_idx);
void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions);
void mark_labels(CompilerContext* ctx, TACTable* instructions);
void find_leaders(CompilerContext* ctx, TACTable* instructions);

bool found_label(TACInstruction* instruction);
void store_label(CompilerContext* ctx, TACInstruction* instruction, int tac_index);
bool found_leader(TACInstruction* instruction);
bool add_label_to_entries(CompilerContext* ctx, TACLabel* label);
TACLeaders crete_tac_leaders()
TACLabel* create_label_entry(CompilerContext* ctx, char* name, int tac_index);
TACLabelEntries create_tac_label_entries(CompilerContext* ctx);

void emit_function_infos();
void build_function_cfg(CompilerContext* ctx, TACTable* instructions, FunctionInfo* info);
bool found_label(TACInstruction* instruction);
bool found_function(TACInstruction* instruction);
FunctionInfo* create_function_info(CompilerContext* ctx, TACInstruction* instruction, int tac_start_index, int tac_end_index);
bool add_function_info_to_list(CompilerContext* ctx, FunctionInfo* info);
FunctionList create_function_list(CompilerContext* ctx);
BasicBlock* create_basic_block();
CFG* create_cfg();
void build_cfg(CompilerContext* ctx, TACTable* instructions);

#endif