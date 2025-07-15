#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdlib.h>
#include <string.h>

typedef struct CompilerContext CompilerContext;
typedef struct Node Node;
typedef struct Type Type;

#define INIT_STACK_CAPACITY 100
#define INIT_TABLE_CAPACITY 300 

typedef enum {
	SYMBOL_LOCAL,
	SYMBOL_PARAM,
	SYMBOL_GLOBAL
} symbol_t;

typedef struct Symbol {
	symbol_t kind;
	
	char* name;
	struct Type* type;	
	struct Symbol* link;
	struct Symbol* next;
	Node* params;

	size_t local_byte_offset;
	size_t total_bytes;
	size_t param_byte_offset;
	size_t actual_bytes;
	
} Symbol;

typedef struct SymbolTable {
	int size;
	int capacity;
	Symbol** symbols;
} SymbolTable;

typedef struct SymbolStack {
	int top;
	int capacity;
	SymbolTable** tables;
} SymbolStack;

Symbol* create_symbol(CompilerContext* ctx, symbol_t kind, char* name, Node* params, struct Type* type);
SymbolTable* create_symbol_table(CompilerContext* ctx);
SymbolStack* create_stack(CompilerContext* ctx);

#endif