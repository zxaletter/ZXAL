#ifndef CFG_H
#define CFG_H

#include "compilercontext.h"
#include "tac.h"

#define INIT_BLOCKS_CAPACITY 100
#define INIT_FUNCTION_LIST_CAPACITY 40
#define INIT_TAC_INSTRUCTIONS_CAPACITY 50
#define INIT_PREDECESSOR_CAPACITY 20
#define INIT_SUCCESSORS_CAPACITY 20
#define INIT_LEADERS_CAPACITY 100
#define INIT_LIVENESS_TABLE_CAPACITY 50
#define INIT_INTERFERENCE_BUNDLE_CAPACITY 50

typedef struct {
	int size;
	int capacity;
	int* leaders;
} TACLeaders;

typedef union {
	Symbol* symbol;
	char* label_name;
} LiveInfoVar;

typedef struct LivenessInfo {
	operand_t type;
	LiveInfoVar var;
	bool is_live;
	int next_use;
	struct LivenessInfo* link;
} LivenessInfo;

typedef struct {
	LivenessInfo** liveness_infos;
	int size;
	int capacity;
} LivenessTable;

typedef enum {
	REG,
	STACK
} arg_location;

typedef struct {
	char* func_name;
	int start;
	int end;
	int instr_id;
} CallInstruction;

typedef struct {
	arg_location loc;
	TACInstruction* tac;
} ArgumentInfo;

typedef struct {
	int size;
	int capacity;
	ArgumentInfo** args;
	CallInstruction* c_instr;
} ArgumentList;

typedef struct {
	int call_instr_count;
	ArgumentList** lists;
} StructuredArgs;

typedef enum {
	REG_TO_FRAME,
	FRAME_TO_REG,
	PUSH_TO_STACK,
	POP_FROM_STACK
} direction_t;

typedef struct {
	int assigned_register;
	int frame_byte_offset;
	int push_index;
	int block_index;
	direction_t direction;
} Spill;

typedef struct {
	int size;
	Spill* spills;
} SpillBundle;

typedef struct {
	int size;
	int capacity;
	Spill* spills;
} SpillSchedule;

typedef struct {
	int assigned_register;
	int frame_byte_offset;
	int pop_index;
	int block_index;
	direction_t direction;
} Reload;

typedef struct {
	int size;
	Reload* reloads;
} ReloadBundle;

typedef struct {
	int size;
	int capacity;
	Reload* reloads;
} ReloadSchedule;

typedef struct BasicBlock {
	int id;
	int num_instructions;
	int num_predecessors;
	int num_successors;

	int num_instructions_capacity;
	int num_predecessors_capacity;
	int num_successors_capacity;
	
	bool visited;
	
	TACInstruction** instructions;
	struct BasicBlock** predecessors;
	struct BasicBlock** successors;

	OperandSet* use_set;
	OperandSet* def_set;
	OperandSet* in_set;
	OperandSet* out_set;

	StructuredArgs* sargs;
	SpillSchedule* spill_schedule; // for standard emission of push instructions
	ReloadSchedule* reload_schedule;
	ReloadBundle* reload_bundle; // for emission of pop instructions for callee regs in correct order
} BasicBlock;

typedef struct {
	BasicBlock* head;
	int num_blocks;
	int blocks_capacity;
	BasicBlock** all_blocks;
	LivenessTable* table;
	ArgumentList* args
} CFG;

typedef struct {
	Operand* operand;
	OperandSet* interferes_with;
	BasicBlock* associated_block;
} InterferenceBundle;

typedef struct {
	int size;
	int capacity;
	InterferenceBundle** bundles;
} InterferenceGraph;

typedef struct {
	int start;
	int end;
	TACInstruction* tac;
	int block_index;
} CallSite;

typedef struct {
	int size;
	int capacity;
	CallSite* sites;	
} CallGraph;

typedef struct {
	Symbol* symbol;
	int total_frame_bytes;
	
	int tac_start_index;
	int tac_end_index;
	CFG* cfg;
	InterferenceGraph* graph;
} FunctionInfo;

typedef struct {
	int size;
	int capacity;
	FunctionInfo** infos;
} FunctionList;

void rmeove_from_operand_set(OperandSet* op_set, Operand* operand);
void compute_instruction_live_out(CompilerContext* ctx, CFG* cfg);

void add_bundle_to_interference_graph(CompilerContext* ctx, InterferenceGraph* graph, InterferenceBundle* bundle);
InterferenceBundle* create_interference_bundle(CompilerContext* ctx, Operand* operand, BasicBlock* associated_block);
InterferenceGraph* create_interference_graph(CompilerContext* ctx);

void build_interference_graph(CompilerContext* ctx);

LivenessInfo* retrieve_livenessinfo(LivenessTable* table, int hash_key, char* name, operand_t type);
void init_operand_liveinfo(CompilerContext* ctx, CFG* cfg, Operand* operand);
void init_instruction_liveinfo(CompilerContext* ctx, CFG* cfg, TACInstruction* instruction);
void init_liveinfo_state(CompilerContext* ctx, CFG* cfg);
bool add_liveinfo_to_liveness_table(CompilerContext* ctx, LivenessTable* table, LivenessInfo* live_info, int hash_key);

int get_operand_index(OperandSet* op_set, Operand* operand);
bool operands_equal(Operand* op1, Operand* op2);
bool contains_operand(OperandSet* op_set, Operand* operand);
OperandSet* copy_set(CompilerContext* ctx, OperandSet* original_set);
void union_sets(CompilerContext* ctx, OperandSet* dest, OperandSet* src);
bool sets_equal(OperandSet* set1, OperandSet* set2);
OperandSet* difference_sets(CompilerContext* ctx, OperandSet* set1, OperandSet* set2);
void populate_and_use_defs(CompilerContext* ctx, BasicBlock* block);
bool init_block_sets(CompilerContext* ctx, BasicBlock* block);


void add_to_operand_set(CompilerContext* ctx, OperandSet* op_set, Operand* operand);
void populate_and_use_defs(CompilerContext* ctx, BasicBlock* block);
bool init_block_sets(CompilerContext* ctx, BasicBlock* block);
bool is_operand_label_or_symbol(Operand* op);
void emit_liveness_info(TACInstruction* instruction);

void emit_blocks();
void emit_leaders();

BasicBlock* find_matching_label_block(CFG* cfg, char* target_name);
int find_label_index(TACTable* instructions, char* target_name, int current_index);

LivenessInfo* create_liveness_info(CompilerContext* ctx, operand_t type, LiveInfoVar var, bool is_live, int next_use);
LivenessTable* create_liveness_table(CompilerContext* ctx);


int hash_variable(LivenessTable* table, char* name);
void determine_operand_liveness_and_next_use(CompilerContext* ctx, LivenessTable* live_variables, Operand* op, operand_role role, int instruction_index);
void determine_instruction_liveness_info(CompilerContext* ctx, LivenessTable* live_variables, TACInstruction* instruction);
void determine_next_use(CompilerContext* ctx, CFG* cfg);
void live_analysis(CompilerContext* ctx);

void add_leader(CompilerContext* ctx, int leader_idx);
void mark_function_boundaries(CompilerContext* ctx, TACTable* instructions);
void find_leaders(CompilerContext* ctx, TACTable* instructions);

bool found_label(TACInstruction* instruction);
bool found_leader(TACInstruction* instruction);

void add_edges(CompilerContext* ctx, CFG* cfg, int index, BasicBlock* block);
bool make_function_cfgs(CompilerContext* ctx, TACTable* instructions);
void link_function_cfgs(CompilerContext* ctx);
TACLeaders crete_tac_leaders(CompilerContext* ctx);

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