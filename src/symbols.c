#include "symbols.h"
#include "bumpallocator.h"
#include "compilercontext.h"
#include "Parser/node.h"
#include "types.h"

Symbol* create_symbol(CompilerContext* ctx, symbol_t kind, 
	char* name, Node* params, struct Type* t) {
	Symbol* sym = arena_allocate(ctx->symbol_arena, sizeof(Symbol));
	if (!sym) return NULL;

	sym->kind = kind;
	sym->name = NULL;
	sym->type = t;
	sym->params = params;
	sym->link = NULL;
	sym->next = NULL;
	sym->local_byte_offset = 0;
	sym->total_bytes = 0;
	sym->param_byte_offset = 0;
	sym->actual_bytes = 0;

	if (name) {
		int length = strlen(name);
		sym->name = arena_allocate(ctx->symbol_arena, length + 1);
		if (!sym->name) return NULL; 
		strncpy(sym->name, name, length);
		sym->name[length] = '\0';
	} 

	return sym;
}

SymbolTable* create_symbol_table(CompilerContext* ctx) {
	SymbolTable* table = arena_allocate(ctx->symbol_arena, sizeof(SymbolTable));
	if (!table) {
		perror("Error: Unable to allocate space for symbol table\n");
		return NULL;
	}
	
	table->size = 0;
	table->capacity = INIT_TABLE_CAPACITY;
	table->symbols = arena_allocate(ctx->symbol_arena, sizeof(Symbol*) * table->capacity);
	if (!table->symbols) {
		perror("Error: Unable to allocate space for symbol table hash array\n");
		return NULL;
	}
	return table;
}

SymbolStack* create_stack(CompilerContext* ctx) {
	SymbolStack* symbol_stack = arena_allocate(ctx->symbol_arena, sizeof(SymbolStack));
	if (!symbol_stack) {
		perror("Error: Unable to allocate space for stack\n");
		return NULL;
	}

	symbol_stack->top = -1;
	symbol_stack->capacity = INIT_STACK_CAPACITY;
	symbol_stack->tables = arena_allocate(ctx->symbol_arena, sizeof(SymbolTable*) * symbol_stack->capacity);
	if (!symbol_stack->tables) {
		perror("Unable to allocate space for stack tables\n");
		return NULL;
	}

	return symbol_stack;
}
