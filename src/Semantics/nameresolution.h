#ifndef NAMERESOLUTION_H
#define NAMERESOLUTION_H

#include "Parser/node.h"
#include "types.h"
#include "symbols.h"

#define CONTEXT_CAPACITY 10

typedef struct CompilerContext CompilerContext;
typedef enum {
	LOCAL,
	GLOBAL, 
	FUNCTION 
} bind_t;

typedef enum {
	CONTEXT_OP,
	CONTEXT_IF,
	CONTEXT_ELSE_IF,
	CONTEXT_ELSE,
	CONTEXT_CALL,
	CONTEXT_RETURN,
	CONTEXT_AUG,
	CONTEXT_ASSIGNMENT,
	CONTEXT_SUBSCRIPT,
	CONTEXT_LOOP,
	CONTEXT_VOID_FUNCTION,
	CONTEXT_NONVOID_FUNCTION
} context_t;

typedef struct ContextStack {
	int top;
	int capacity;
	context_t* contexts;
} ContextStack;

bool context_lookup(context_t context);
void push_context(CompilerContext* ctx, context_t context);
void pop_context();
context_t peek_context();
ContextStack* create_context_stack(CompilerContext* ctx);

bool is_stack_empty(CompilerContext* ctx);
bool push_scope(CompilerContext* ctx);
void pop_scope(CompilerContext* ctx);

TypeKind get_kind(struct Type* t);
bool has_expression_context();
int peek_scope_level(CompilerContext* ctx);
bool distinct_from_keywords(CompilerContext* ctx, char* variable_name);

int hash(int table_capacity, char* name);
Symbol* lookup_symbol_in_all_scopes(CompilerContext* ctx, Symbol* symbol);
Symbol* lookup_symbol_in_current_scope(CompilerContext* ctx, Symbol* symbol);
bool rehash_local_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity);
bool rehash_function_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity);
bool variable_symbol_exists(CompilerContext* ctx, Symbol* symbol);
bool function_symbol_exists(CompilerContext* ctx, Symbol* symbol);
bool symbol_exists(CompilerContext* ctx, bind_t kind, Symbol* symbol);
bool variable_symbol_bind(CompilerContext* ctx, Symbol* symbol, int hash_key);
bool function_symbol_bind(CompilerContext* ctx, Symbol* func_symbol, int hash_key);
bool bind(CompilerContext* ctx, bind_t kind, Symbol* symbol, int hash_key);
Symbol* lookup_function_symbol(CompilerContext* ctx, Symbol* symbol);
void validate_node_signature(CompilerContext* ctx, Node* node);
void collect_function_symbols(CompilerContext* ctx, Node* root);

void resolve_expression(CompilerContext* ctx, Node* node);
void resolve_statement(CompilerContext* ctx, Node* node);
void resolve_params(CompilerContext* ctx, Node* param);
void resolve_globals(CompilerContext* ctx, Node* node);
void resolve_tree(CompilerContext* ctx, Node* root);
#endif