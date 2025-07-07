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
#define INIT_OP_SET_CAPACITY 50

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

typedef enum {
	SETS_EQAUL,
	MIN_OUT_SET,
	MIN_IN_SET
} set_t;

typedef struct {
	Operand** elements;
	int size;
	int capacity;
} OperandSet;

typedef struct BasicBlock BasicBlock;
typedef struct BasicBlock {
	int num_instructions;
	int num_predecessors;
	int num_successors;

	int num_instructions_capacity;
	int num_predecessors_capacity;
	int num_successors_capacity;
	
	bool visited;
	int id;
	
	TACInstruction** instructions;
	BasicBlock** predecessors;
	BasicBlock** successors;

	OperandSet* use_set;
	OperandSet* def_set;
	OperandSet* in_set;
	OperandSet* out_set;

	LivenessTable* table;

} BasicBlock;

typedef struct {
	BasicBlock* head;
	int num_blocks;
	int blocks_capacity;
	BasicBlock** all_blocks;
	LivenessTable* table;
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

LivenessInfo* retrieve_livenessinfo(LivenessTable* table, int hash_key);
void init_operand_liveinfo(CompilerContext* ctx, CFG* cfg, Operand* operand);
void init_instruction_liveinfo(CompilerContext* ctx, CFG* cfg, TACInstruction* instruction);
void init_liveinfo_state(CompilerContext* ctx, CFG* cfg);
bool add_liveinfo_to_liveness_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* live_info);

void union_sets(CompilerContext* ctx, OperandSet* dest, OperandSet* src);
bool sets_equal(OperandSet* set1, OperandSet* set2);
OperandSet* difference_sets(CompilerContext* ctx, OperandSet* set1, OperandSet* set2);
OperandSet* copy_set(CompilerContext* ctx, OperandSet* original_set);
bool contains_operand(OperandSet* op_set, Operand* operand);
void populate_and_use_defs(CompilerContext* ctx, BasicBlock* block);
bool init_block_sets(CompilerContext* ctx, BasicBlock* block);


void add_to_op_set(CompilerContext* ctx, OperandSet* op_set, Operand* operand);
void populate_and_use_defs(CompilerContext* ctx, BasicBlock* block);
OperandSet* create_operand_set(CompilerContext* ctx);
bool init_block_sets(CompilerContext* ctx, BasicBlock* block);
bool is_operand_label_or_symbol(Operand* op);
void emit_liveness_info(TACInstruction* instruction);

void emit_blocks();
void emit_leaders();

BasicBlock* find_matching_label_block(CFG* cfg, char* target_name);
int find_label_index(TACTable* instructions, char* target_name, int current_index);

LivenessInfo* create_liveness_info(CompilerContext* ctx, LiveInfoVar var, bool is_live, int next_use);
LivenessTable* create_liveness_table(CompilerContext* ctx);

void determine_next_use(CompilerContext* ctx, CFG* cfg);

int hash_variable(LivenessTable* table, char* name);

void determine_instruction_liveness_info(CompilerContext* ctx, CFG* cfg, BasicBlock* block, int current_index);
void determine_operand_liveness_and_next_use(CompilerContext* ctx, CFG* cfg, BasicBlock* block, Operand* op, operand_role role, int instruction_index);

void operand_contain_variable(CompilerContext* ctx, Operand* op);
void instruction_contains_nonvirtual_variables(CompilerContext* ctx, BasicBlock* block, int current_index);
void store_nonvirtual_variables(CompilerContext* ctx);
void live_analysis(CompilerContext* ctx);

void add_leader(CompilerContext* ctx, int leader_idx);
void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions);
void mark_labels(CompilerContext* ctx, TACTable* instructions);
void find_leaders(CompilerContext* ctx, TACTable* instructions);

bool found_label(TACInstruction* instruction);
bool found_leader(TACInstruction* instruction);

void add_edges(CompilerContext* ctx, CFG* cfg, int index, BasicBlock* block);
bool add_label_to_entries(CompilerContext* ctx, TACLabel* label);
bool make_function_cfgs(CompilerContext* ctx, TACTable* instructions);
void link_function_cfgs(CompilerContext* ctx);
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