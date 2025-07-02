#ifndef COMPILER_CONTEXT_H
#define COMPILER_CONTEXT_H

#include "bumpallocator.h"

typedef struct {
	Arena* lexer_arena;
	Arena* ast_arena;
	Arena* type_arena;
	Arena* symbol_arena;
	Arena* ir_arena;
	Arena* codegen_arena;
	Arena* error_arena;
} CompilerContext;

CompilerContext* create_compiler_context();
void free_compiler_context(CompilerContext* ctx); 
#endif