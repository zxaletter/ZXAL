#ifndef DAG_H
#define DAG_H

#include "auxiliaries.h"
#include "Semantics/symbols.h"
#include "compilercontext.h"
#include <string.h>
#define DAG_NODES 1000

typedef enum {
	DAG_INTEGER, // 0
	DAG_CHAR,
	DAG_BOOL,
	DAG_IF,
	DAG_ELSE_IF,
	DAG_ELSE, // 5
	DAG_FOR,
	DAG_WHILE,
	DAG_BREAK,
	DAG_CONTINUE,
	DAG_RETURN, // 10
	DAG_CALL,
	DAG_SUBSCRIPT,
	DAG_PARAM,
	DAG_ARG,
	DAG_ASSIGNMENT, // 15
	
	DAG_BLOCK,
	DAG_NAME,
	DAG_DECL,
	DAG_DEF,
	DAG_AUG, // 20
	
	DAG_ADD,
	DAG_SUB, // 22
	DAG_MUL,
	DAG_DIV,
	DAG_ADD_EQUAL, // 25
	DAG_SUB_EQUAL,
	DAG_MUL_EQUAL,
	DAG_DIV_EQUAL,
	DAG_MODULO, 
	DAG_LOGICAL_OR, // 30
	DAG_LOGICAL_AND,

	DAG_LESS,
	DAG_GREATER,
	DAG_LESS_EQUAL,
	DAG_GREATER_EQUAL, // 35
	DAG_NOT,
	DAG_EQUAL,
	DAG_NOT_EQUAL,
	DAG_INCREMENT, // 39
	DAG_DECREMENT,
	DAG_ARRAY_LIST,
	DAG_UNKNOWN
} dagnode_t;

typedef union {
	int val;
	char* name;
} DAGNodeValue;

typedef struct DAGNode DAGNode;
typedef struct DAGNode {
	dagnode_t kind;
	DAGNodeValue value;
	
	DAGNode* left;
	DAGNode* right;
	DAGNode* prev;
	DAGNode* next;
	DAGNode* params;
	Symbol* symbol;
	struct type* type;
	int dag_id;
	bool freed;
	char* name;
} DAGNode;

typedef struct DAGNodeEntry DAGNodeEntry;
struct DAGNodeEntry {
	char* key;
	DAGNode* node;
	DAGNodeEntry* link;
	bool freed;
};

typedef struct {
	int size;
	int capacity;
	DAGNodeEntry** entries;
} DAGNodeTable; 

dagnode_t get_dagnode_type(node_t type);
unsigned int hash_string(char* key);
DAGNode* hash_get_dagnode(unsigned int index, char* key);
DAGNode* get_or_bind_dagnode(CompilerContext* ctx, DAGNode* dagnode);
char* generate_hash_key(CompilerContext* ctx, DAGNode* dagnode);
bool hash_bind_dagnode(CompilerContext* ctx, DAGNode* node, unsigned int index, char* key);

DAGNode* copy_dagnode(CompilerContext* ctx, DAGNode* original_dagnode);

DAGNode* create_dagnode(CompilerContext* ctx, dagnode_t kind, DAGNode* left, DAGNode* right, DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type);
DAGNode* create_string_dagnode(CompilerContext* ctx, dagnode_t kind, char* name, DAGNode* left, DAGNode* right, DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type);
DAGNode* create_int_dagnode(CompilerContext* ctx, dagnode_t kind, int val, DAGNode* left, DAGNode* right, DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type);
DAGNodeTable* create_dagnode_table(CompilerContext* ctx);
DAGNodeEntry* create_dag_node_entry(CompilerContext* ctx, char* key, DAGNode* node, DAGNodeEntry* link);
void init_dag_node_table(CompilerContext* ctx);


DAGNode* build_expression_dagnode(CompilerContext* ctx, Node* node);
DAGNode* build_statement_dagnode(CompilerContext* ctx, Node* node);
DAGNode* build_params_dagnode(CompilerContext* ctx, Node* node);
DAGNode* build_global_dagnode(CompilerContext* ctx, Node* global_node);
DAGNode* build_DAG(CompilerContext* ctx, Node* root);

// void free_dagnode_table();
// void free_dag_node(DAGNode* dagnode);
// void free_dag_entry(DAGNodeEntry* entry);
// void free_dag(DAGNode* root);
#endif