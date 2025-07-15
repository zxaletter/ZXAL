#ifndef NAMERESOLUTION_H
#define NAMERESOLUTION_H

#include "Parser/node.h"
#include "errorsandcontext.h"
#include "types.h"
#include "symbols.h"

typedef struct CompilerContext CompilerContext;


typedef enum {
	LOCAL,
	GLOBAL, 
	FUNCTION 
} BindType;

typedef enum {
	PARAM_UPDATE,
	LOCAL_UPDATE
} UpdateNodeInfoType;

size_t get_num_bytes(Symbol* symbol);
TypeKind get_kind(struct Type* t);

int hash(int table_capacity, char* name);

bool scope_bind(CompilerContext* ctx, Symbol* symbol, int hash_key);
Symbol* lookup_symbol_in_all_scopes(CompilerContext* ctx, Symbol* symbol);
Symbol* lookup_symbol_in_current_scope(CompilerContext* ctx, Symbol* symbol);

context_t peek_context();
bool is_stack_empty(CompilerContext* ctx);
bool push_scope(CompilerContext* ctx);
void pop_scope(CompilerContext* ctx);

bool has_expression_context();

void update_node_info(Node* node, UpdateNodeInfoType kind, size_t size);
void update_total_local_bytes(size_t size);
void update_local_byte_offset(size_t size);
void update_global_parameter_variables(size_t size);
void set_total_local_bytes(Node* node);
void reset_offsets();

void resolve_expression(CompilerContext* ctx, Node* node);
void resolve_statement(CompilerContext* ctx, Node* node);
void resolve_params(CompilerContext* ctx, Node* param);
void resolve_globals(CompilerContext* ctx, Node* node);
void resolve_tree(CompilerContext* ctx, Node* root);

bool rehash_local_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity);
bool rehash_function_symbols(CompilerContext* ctx, Symbol** prev_symbols, int new_capacity, int prev_capacity);

bool variable_symbol_exists(CompilerContext* ctx, Symbol* symbol);
bool function_symbol_exists(CompilerContext* ctx, Symbol* symbol);
bool symbol_exists(CompilerContext* ctx, BindType kind, Symbol* symbol);

bool variable_symbol_kind(CompilerContext* ctx, Symbol* symbol, int hash_key);
bool function_symbol_bind(CompilerContext* ctx, Symbol* func_symbol, int hash_key);
bool bind(CompilerContext* ctx, BindType kind, Symbol* symbol, int hash_key);

Symbol* lookup_function_symbol(CompilerContext* ctx, Symbol* symbol);
void validate_node_signature(CompilerContext* ctx, Node* node);
void collect_function_symbols(CompilerContext* ctx, Node* root);
#endif