#ifndef TAC_H
#define TAC_H

#include "dag.h"
#include "Semantics/symbols.h"
#include "compilercontext.h"

#define INITIAL_TABLE_CAPACITY 500 
#define INITIAL_TACCONTEXT_CAPACITY 100

typedef enum {
	OP_SYMBOL,
	OP_INT_LITERAL,
	OP_LABEL,
	OP_UNKNOWN
} operand_t;

typedef union {
	Symbol* sym;
	int int_val;
	char* label_name;
} OperandValue;

typedef struct {
	operand_t kind;
	OperandValue value;
} Operand;

typedef enum {
	TAC_INTEGER,
	TAC_CHAR,
	TAC_BOOL,

	TAC_ADD,
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

	// TAC_DEF,
	// TAC_DECL, 
	// TAC_AUG, 
	TAC_NAME, // 26
	TAC_BLOCK,
	TAC_ASSIGNMENT,
	TAC_ARG, // 29
	TAC_PARAM,
	TAC_CALL,
	// TAC_ARRAY_DECL,
	TAC_STORE,
	TAC_DEREFERENCE, 

	TAC_IF, // 35
	TAC_ELSE_IF,
	TAC_ELSE, // 37
	TAC_FOR,
	TAC_WHILE,
	TAC_BREAK, // 40
	TAC_RETURN,
	TAC_CONTINUE, // 42

	TAC_IF_FALSE,
	TAC_GOTO,
	TAC_LABEL
} tac_t;

typedef struct {
	tac_t type;
	Operand* result;
	Operand* op1;
	Operand* op2;
	Operand* op3;
} TACInstruction;

typedef struct {
	int size;
	int capacity;
	int tac_index;
	TACInstruction** tacs;
} TACTable;

typedef struct {
	tac_t type;
	char* next_label;
	char* end_label;
	char* update_label;
} TACContext;

typedef struct {
	int size;
	int capacity;
	int top;
	TACContext** contexts;
} TACContextStack;

operand_t get_operand_type(Operand* op);
Operand* create_operand(CompilerContext* ctx, operand_t kind, OperandValue value);

char* convert_subtype_to_string(struct type* subtype);
TACInstruction* build_tac_from_array_dagnode(CompilerContext* ctx, DAGNode* array_identifier, DAGNode* array_list);
TACContext* tac_context_lookup(tac_t* target_types, size_t length); 
TACContext* peek_tac_context();
bool is_tac_context_stack_empty();
void pop_tac_context();
void clear_tac_contexts(tac_t target_type);
void push_tac_context(CompilerContext* ctx, TACContext* context);
TACContext* create_tac_context(CompilerContext* ctx, tac_t type, char* next_label, char* end_label, char* update_label);
bool init_tac_context_stack(CompilerContext* ctx);
TACContextStack create_tac_context_stack(CompilerContext* ctx);

char* generate_label(CompilerContext* ctx);
char* tac_variable_create(CompilerContext* ctx);
char* tac_function_argument_label(CompilerContext* ctx);
char* tac_parameter_label(CompilerContext* ctx);
char* tac_function_name(CompilerContext* ctx, char* name);
tac_t get_tac_type(dagnode_t type);

TACInstruction* create_tac(CompilerContext* ctx, tac_t type, Operand* result, Operand* op1, Operand* op2, Operand* op3);
TACTable* create_tac_table(CompilerContext* ctx);
void init_tac_table(CompilerContext* ctx);
void add_tac_to_table(CompilerContext* ctx, TACInstruction* tac);

void build_tac_from_parameter_dag(CompilerContext* ctx, DAGNode* node);
TACInstruction* build_tac_from_expression_dag(CompilerContext* ctx, DAGNode* node);
void build_tac_from_statement_dag(CompilerContext* ctx, DAGNode* node);
void build_tac_from_global_dag(CompilerContext* ctx, DAGNode* node);
TACTable* build_tacs(CompilerContext* ctx, DAGNode* node);

char* get_tac_op(TACInstruction* tac);
void emit_tac_instructions();
#endif