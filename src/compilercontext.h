#ifndef COMPILER_CONTEXT_H
#define COMPILER_CONTEXT_H

#include "bumpallocator.h"
typedef struct SymbolTable SymbolTable;
typedef struct SymbolStack SymbolStack;
typedef struct ErrorTableCollection ErrorTableCollection;

#define KEYWORDS 20
#define NUM_PHASES 7

typedef enum {
	PHASE_LEXER,
	PHASE_PARSER,
	PHASE_SYMBOL_RESOLUTION,
	PHASE_TYPECHECKER,
	PHASE_IR,
	PHASE_REGALLOC,
	PHASE_CODEGEN
} phase_t;

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
	ErrorTables** error_tables;
	ErrorTableCollection collection; 
} CompilerContext;

CompilerContext* create_compiler_context();
void free_compiler_context(CompilerContext* ctx); 
#endif