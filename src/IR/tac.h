#ifndef TAC_H
#define TAC_H

#include "dag.h"
#include "Semantics/symbols.h"

#define INITIAL_TABLE_CAPACITY 500 
#define INITIAL_TACCONTEXT_CAPACITY 100
typedef enum {
	TAC_INTEGER,
	TAC_CHAR,
	TAC_BOOL,

	TAC_ADD,
	TAC_SUB,
	TAC_MUL,
	TAC_DIV,
	TAC_ADD_EQUAL,
	TAC_SUB_EQUAL,
	TAC_MUL_EQUAL,
	TAC_DIV_EQUAL,
	TAC_MODULO,
	TAC_INCREMENT,
	TAC_DECREMENT,

	TAC_LESS,
	TAC_GREATER,
	TAC_LESS_EQUAL,
	TAC_GREATER_EQUAL,
	TAC_NOT,
	TAC_EQUAL,
	TAC_NOT_EQUAL,
	TAC_LOGICAL_AND,
	TAC_LOGICAL_OR,

	TAC_DEF,
	TAC_DECL,
	TAC_AUG,
	TAC_NAME,
	TAC_BLOCK,
	TAC_ASSIGNMENT,
	TAC_ARG,
	TAC_PARAM,
	TAC_CALL,

	TAC_IF,
	TAC_ELSE_IF,
	TAC_ELSE,
	TAC_FOR,
	TAC_WHILE,
	TAC_BREAK,
	TAC_RETURN,
	TAC_CONTINUE,

	TAC_IF_FALSE,
	TAC_GOTO,
	TAC_LABEL
} tac_t;

typedef struct {
	tac_t type;
	char* name;
	char* op1;
	char* op2;
	char* op3;
	bool freed;
	Symbol* symbol;
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

TACContext* tac_context_lookup(tac_t type); 
TACContext* peek_tac_context();
bool is_tac_context_stack_empty();
void pop_tac_context();
void push_tac_context(TACContext* context);
TACContext* create_tac_context(tac_t type, char* next_label, char* end_label, char* update_label);
void init_tac_context_stack();
TACContextStack create_tac_context_stack();
void free_tac_context(TACContext* context);
void free_tac_context_stack();

char* generate_label();
char* tac_variable_create();
char* tac_function_argument_label();
char* tac_parameter_label();
char* tac_function_name(char* name);
tac_t get_tac_type(dagnode_t type);

TACInstruction* create_tac(tac_t type, char* name, char* op1, char* op2, char* op3, Symbol* symbol);
TACTable* create_tac_table();
void init_tac_table();
void add_tac_to_table(TACInstruction* tac);

void build_tac_from_parameter_dag(DAGNode* node);
TACInstruction* build_tac_from_expression_dag(DAGNode* node);
void build_tac_from_statement_dag(DAGNode* node);
void build_tac_from_global_dag(DAGNode* node);
TACTable* build_tacs(DAGNode* node);

void free_tac_instruction(TACInstruction* tac);
void free_tac_table();
#endif