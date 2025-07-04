#ifndef CFG_H
#define CFG_H

#include "compilercontext.h"
#include "tac.h"

#define INIT_BLOCKS_CAPACITY 100
#define INIT_FUNCTION_LIST_CAPACITY 40
#define INIT_TAC_INSTRUCTIONS_CAPACITY 50
#define INIT_PREDECESSOR_CAPACITY 20
#define INIT_SUCCESSORS_CAPACITY 20
#define INIT_TAC_LABEL_ENTRIES_CAPACITY 100
#define INIT_LEADERS_CAPACITY 100
#define INIT_LIVENESS_TABLE_CAPACITY 50

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

typedef union {
	Symbol* symbol;
	char* label_name;
} LiveInfoVar;

typedef struct {
	LiveInfoVar var;
	bool is_live;
	int next_use;
} LivenessInfo;

typedef struct {
	LivenessInfo** liveness_infos;
	int size;
	int capacity;
} LivenessTable;

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

	LivenessTable* table;

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

bool is_operand_label_or_symbol(Operand* op);
void emit_liveness_info(TACInstruction* instruction);

void emit_blocks();
void emit_leaders();

BasicBlock* find_matching_label_block(CFG* cfg, int starting_search_index, TACInstruction* instruction);
int find_label_index(TACTable* instructions, char* target_name, int current_index);

LivenessInfo* create_liveness_info(CompilerContext* ctx, LiveInfoVar var, bool is_live, int next_use);
LivenessTable* create_liveness_table(CompilerContext* ctx);

int hash_variable(BasicBlock* block, char* name);
void set_operand_live_info(Operand* op, LivenessInfo* info);
bool bind_or_update_live_info_to_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* info, unsigned int hash_key);
void determine_operand_liveness_and_next_use(CompilerContext* ctx, BasicBlock* block, Operand* op, operand_role role, int instruction_index);
void attach_liveness_and_next_use(CompilerContext* ctx, BasicBlock* block, int instruction_index);
void operand_contain_nonvirtual_variable(CompilerContext* ctx, BasicBlock* block, Operand* op);
void instruction_contains_nonvirtual_variables(CompilerContext* ctx, BasicBlock* block, int current_index);
void store_nonvirtual_variables(CompilerContext* ctx);
void live_analysis(CompilerContext* ctx);

void add_leader(CompilerContext* ctx, int leader_idx);
void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions);
void mark_labels(CompilerContext* ctx, TACTable* instructions);
void find_leaders(CompilerContext* ctx, TACTable* instructions);

bool found_label(TACInstruction* instruction);
bool found_leader(TACInstruction* instruction);
void add_edges(CFG* cfg, int index, BasicBlock* block);
bool add_label_to_entries(CompilerContext* ctx, TACLabel* label);
bool make_function_cfgs(CompilerContext* ctx, TACTable* instructions);
void link_function_cfgs();
TACLeaders crete_tac_leaders(CompilerContext* ctx);
TACLabel* create_label_entry(CompilerContext* ctx, TACInstruction* instruction, int tac_index);
TACLabelEntries create_tac_label_entries(CompilerContext* ctx);

void emit_function_infos();
void build_function_cfg(CompilerContext* ctx, TACTable* instructions, FunctionInfo* info);
bool found_label(TACInstruction* instruction);
bool found_function(TACInstruction* instruction);

FunctionInfo* create_function_info(CompilerContext* ctx, TACInstruction* instruction, int tac_start_index, int tac_end_index);
bool add_function_info_to_list(CompilerContext* ctx, FunctionInfo* info);
FunctionList* create_function_list(CompilerContext* ctx);

bool add_block_to_cfg(CompilerContext* ctx, CFG* cfg, BasicBlock* block);
BasicBlock* create_basic_block(CompilerContext* ctx);
CFG* create_cfg(CompilerContext* ctx);
FunctionList* build_cfg(CompilerContext* ctx, TACTable* instructions);

#endif