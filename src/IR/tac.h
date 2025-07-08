#ifndef TAC_H
#define TAC_H

#include "compilercontext.h"
#include "Semantics/symbols.h"
#include "Parser/parser.h"
// #include "dag.h"
#include "auxiliaries.h"

#define INITIAL_TABLE_CAPACITY 500 
#define INITIAL_TACCONTEXT_CAPACITY 100

typedef enum {
	OP_RESULT,
	OP_USE
} operand_role;

typedef enum {
	OP_SYMBOL,
	OP_BINARY,
	OP_UNARY,
	OP_ARG,
	OP_INT_LITERAL,
	OP_LABEL,
	
	// for function calls, ints, bools, and chars
	OP_STORE, 
	// e.g. int x = 1;
	//      => t_i = 1
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

typedef struct {
	operand_t kind;
	OperandValue value;
	bool is_live;
	int next_use;
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

	
	TAC_NAME, // 23
	TAC_BLOCK,
	TAC_ASSIGNMENT,
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
	TAC_LABEL
} tac_t;

typedef struct {
	tac_t type;
	int id;
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
TACInstruction* build_tac_from_array_dagnode(CompilerContext* ctx, Node* array_identifier, Node* array_list);
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
tac_t get_tac_type(node_t type);

TACInstruction* create_tac(CompilerContext* ctx, tac_t type, Operand* result, Operand* op1, Operand* op2, Operand* op3);
TACTable* create_tac_table(CompilerContext* ctx);
void init_tac_table(CompilerContext* ctx);
void add_tac_to_table(CompilerContext* ctx, TACInstruction* tac);

void build_tac_from_parameter_dag(CompilerContext* ctx, Node* node);
TACInstruction* build_tac_from_expression_dag(CompilerContext* ctx, Node* node);
void build_tac_from_statement_dag(CompilerContext* ctx, Node* node);
void build_tac_from_global_dag(CompilerContext* ctx, Node* node);
TACTable* build_tacs(CompilerContext* ctx, Node* node);

char* get_tac_op(TACInstruction* tac);
void emit_tac_instructions();
#endif