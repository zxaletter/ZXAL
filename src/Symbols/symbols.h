#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "Parser/parser.h"
#include "auxiliaries.h"
#include "errorsandcontext.h"
#define STACK_CAPACITY 100
#define TABLE_CAPACITY 300

typedef enum {
	SYMBOL_LOCAL,
	SYMBOL_PARAM,
	SYMBOL_GLOBAL
} symbol_t;

typedef struct Symbol {
	symbol_t kind;
	bool symbol_free;
	bool bind_symbol;
	char* name;
	struct type* type;	
	struct Symbol* link;
	struct Symbol* next;

	struct {
		size_t local_byte_offset;
		size_t total_bytes;
		size_t param_byte_offset;
		size_t actual_bytes;
	};

	int array_size;

} Symbol;

typedef struct SymbolTable {
	int level;
	int size;
	int capacity;
	Symbol** symbols;
	bool symboltable_free;
} SymbolTable;

typedef struct SymbolStack {
	int top;
	int size;
	int capacity;
	SymbolTable** tables;
} SymbolStack;

size_t get_num_bytes(Symbol* symbol);
data_t get_kind(struct type* t);

int hash(char* name);
SymbolTable* create_symbol_table();
Symbol* create_array_symbol(symbol_t kind, char* name, int element_count, struct type* type);
Symbol* create_symbol(symbol_t kind, char* name, struct type* type);
bool scope_bind(Symbol* symbol, int hash_key);
Symbol* scope_lookup(Symbol* symbol, int hash_key);
Symbol* scope_lookup_current(Symbol* symbol, int hash_key);
Symbol* symbol_copy(Symbol* original_symbol);

bool is_stack_empty();
bool push_scope();
bool pop_scope();
SymbolStack* create_stack();
void init_symbol_stack();
void resolve_expression(Node* node);
void resolve_statement(Node* node);
void resolve_params(Node* param);
void resolve_globals(Node* node);
void resolve_tree(Node* root);

void free_symbol(Symbol* symbol);
void free_table(SymbolTable* table);
void free_stacks();
#endif