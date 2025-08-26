#ifndef TAC_H
#define TAC_H

#include "compilercontext.h"
#include "bumpallocator.h"
#include "symbols.h"
#include "Semantics/nameresolution.h"
#include "Parser/node.h"
#include "Parser/parser.h"
#include "types.h"
#include "errors.h"
#include <stdbool.h>

#define INITIAL_TABLE_CAPACITY 500 
#define INITIAL_TACCONTEXT_CAPACITY 100
#define INIT_OP_SET_CAPACITY 50

typedef enum {
	VIRTUAL, 
	ARG_LABEL,  
	PARAM_LABEL,
	REG_LABEL
} LabelKind;

// For Liveness Info
// ----------------
typedef enum {
	OP_RESULT,
	OP_USE
} operand_role;
// ----------------

typedef enum {
	OP_SYMBOL,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MODULO,
	OP_LESS,
	OP_GREATER,
	OP_LESS_EQUAL,
	OP_GREATER_EQUAL,
	OP_EQUAL, // 10
	OP_NOT_EQUAL,
	OP_NOT,
	OP_LOGICAL_AND,
	OP_LOGICAL_OR,

	OP_UNARY_ADD,
	OP_UNARY_SUB,
	OP_ARG,
	OP_INT_LITERAL,
	OP_LABEL,
	OP_SUBTYPE_STR,
	
	// for function calls, ints, bools, and chars
	OP_STORE, 
	// e.g. int x = 1;
	//      => t_i = 1
	//      => x = t_i
	//      
	//      CALL f
	//      => t_j = _RET
	//      where _RET is the return value of f,
 	//      provided f returns a value.
 	//      if the CALL f instruction is isolated, 
 	//  	we don't emit t_j = _RET
 	//

	OP_RETURN, // for _RET for functions call
	OP_UNKNOWN
} operand_t;

typedef union {
	Symbol* sym;
	int int_val;
	char* label_name;
} OperandValue;


typedef struct Operand {
	operand_t kind;
	OperandValue value;
	TypeKind type;
	struct Operand* link; 
	
	//---------
	// for live analysis
	bool is_live;
	int next_use;
	//----------

	int* restricted_regs;
	int restricted_regs_count;
	bool restricted;

	//----------
	// for reg allocation 
	bool precedes_conditional;
	//-----------

	int assigned_register;
	int temp_register;
	bool permanent_frame_position;
	size_t frame_byte_offset;

	void* interference_bundle;
	bool pre_colored;
} Operand;

typedef struct {
	Operand** elements;
	int size;
	int capacity;
} OperandSet;

typedef enum {
	TAC_INTEGER, // 0
	TAC_CHAR, // 1
	TAC_BOOL, // 2

	TAC_ADD, // 3
	TAC_SUB, // 4
	TAC_MUL,
	TAC_DIV,
	TAC_ADD_EQUAL,
	TAC_SUB_EQUAL,
	TAC_MUL_EQUAL, // 9
	TAC_DIV_EQUAL,
	TAC_MODULO,
	TAC_INCREMENT,
	TAC_DECREMENT,

	TAC_LESS, // 14
	TAC_GREATER,
	TAC_LESS_EQUAL,
	TAC_GREATER_EQUAL,
	TAC_NOT,
	TAC_EQUAL, // 19
	TAC_NOT_EQUAL,
	TAC_LOGICAL_AND,
	TAC_LOGICAL_OR,

	
	TAC_NAME, // 23
	TAC_BLOCK,
	TAC_ASSIGNMENT, // 25
	TAC_ARG, // 26
	TAC_PARAM,
	TAC_CALL,
	TAC_STORE,
	TAC_DEREFERENCE, 

	TAC_IF, // 31
	TAC_ELSE_IF,
	TAC_ELSE, // 33
	TAC_FOR,
	TAC_WHILE,
	TAC_BREAK, // 36
	TAC_RETURN,
	TAC_CONTINUE, // 38

	TAC_IF_FALSE,
	TAC_GOTO,
	TAC_LABEL,
	TAC_DEREFERENCE_AND_ASSIGN,
	TAC_UNARY_ADD,
	TAC_UNARY_SUB,
	TAC_FUNCTION_RET_VAL
} tac_t;

typedef struct {
	tac_t kind;
	int id;
	bool handled; 
	
	bool precedes_conditional; // for inteference graph

	Operand* result;
	Operand* op1;
	Operand* op2;
	
	OperandSet* live_out;
} TACInstruction;


typedef struct {
	int size;
	int capacity;
	TACInstruction** tacs;

	OperandSet* op_names;
} TACTable;

typedef struct TACContext {
	context_t type;
	bool root_chain_mem;
	char* next_label;
	char* end_label;
	char* update_label;
	struct TACContext* parent;
} TACContext;

typedef struct {
	int capacity;
	int top;
	TACContext** contexts;
} TACContextStack;

void add_to_local_table(CompilerContext* ctx, TACTable* local_table, TACInstruction* tac);

bool determine_if_next_conditional(Node* node, bool has_next_statement);
void emit_label(CompilerContext* ctx, char* label);
void emit_if_false(CompilerContext* ctx, Operand* condition, char* target);
void emit_goto(CompilerContext* ctx, char* target);

bool is_op(operand_t type);
operand_t node_to_operand_type(node_t type);
operand_t get_operand_type(Operand* op);
TypeKind node_to_type(node_t type);

int get_operand_index(OperandSet* op_set, Operand* operand);
void add_to_operand_set(CompilerContext* ctx, OperandSet* op_set, Operand* operand);
OperandSet* create_operand_set(CompilerContext* ctx);
Operand* create_operand(CompilerContext* ctx, operand_t kind, OperandValue value, TypeKind type);

char* convert_subtype_to_string(struct Type* subtype);
TACInstruction* build_tac_from_array_dagnode(CompilerContext* ctx, Node* array_identifier, Node* array_list);

 
TACContext* context_loop_lookup(TACContext* starting_context);
TACContext* peek_tac_context();
bool is_tac_context_stack_empty();
void pop_tac_context();
void push_tac_context(CompilerContext* ctx, TACContext* context);

TACContext* create_tac_context(CompilerContext* ctx, context_t type, 
	char* next_label, char* end_label, 
	char* update_label, bool root_chain_mem);
bool init_tac_context_stack(CompilerContext* ctx);
TACContextStack create_tac_context_stack(CompilerContext* ctx);

char* generate_label(CompilerContext* ctx, LabelKind kind);
char* tac_function_name(CompilerContext* ctx, char* name);
tac_t get_tac_type(node_t type);

TACInstruction* create_tac(CompilerContext* ctx, tac_t type, 
	Operand* result, Operand* op1, Operand* op2);

void add_tac_to_table(CompilerContext* ctx, TACInstruction* tac);
bool init_tac_table(CompilerContext* ctx);
TACTable* create_tac_table(CompilerContext* ctx);

void set_end_label_based_on_context(TACContext* context, char** end_label);

void reset_context_stack();
void reset_tac_indices();

Operand* find_operand_in_op_set(OperandSet* op_set, Operand* sym_op);

void build_tac_from_parameter_dag(CompilerContext* ctx, Node* node);
TACInstruction* build_tac_from_expression_dag(CompilerContext* ctx, Node* node);
void build_tac_from_statement_dag(CompilerContext* ctx, Node* node);
void build_tac_from_global_dag(CompilerContext* ctx, Node* node);
TACTable* build_tacs(CompilerContext* ctx, Node* node);

char* get_tac_op(TACInstruction* tac);
void emit_tac_instructions();
#endif