#ifndef COMPILER_CONTEXT_H
#define COMPILER_CONTEXT_H

#include "bumpallocator.h"
#include "phases.h"
typedef struct SymbolTable SymbolTable;
typedef struct SymbolStack SymbolStack;
typedef struct ErrorTable ErrorTable;


#define KEYWORDS 20
#define NUM_PHASES 7

typedef struct CompilerContext {
	Arena* lexer_arena;
	Arena* ast_arena;
	Arena* type_arena;
	Arena* symbol_arena;
	Arena* ir_arena;
	Arena* codegen_arena;
	Arena* error_arena;

	SymbolTable* global_table; // for having access to function symbols, say with CALL Nodes
	SymbolStack* symbol_stack; // for scopes

	char** keywords;

	phase_t phase;
	ErrorTable* error_tables;
} CompilerContext;

CompilerContext* create_compiler_context();
void free_compiler_context(CompilerContext* ctx); 
#endif