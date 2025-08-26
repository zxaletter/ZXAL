#ifndef COMPILER_CONTEXT_H
#define COMPILER_CONTEXT_H

#include "bumpallocator.h"
typedef struct SymbolTable SymbolTable;
typedef struct SymbolStack SymbolStack;

#define KEYWORDS 20
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
} CompilerContext;

CompilerContext* create_compiler_context();
void free_compiler_context(CompilerContext* ctx); 
#endif