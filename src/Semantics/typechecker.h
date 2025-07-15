#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <stdbool.h>

typedef struct CompilerContext CompilerContext;
typedef struct Node Node;
typedef struct Symbol Symbol; 
typedef struct Type Type;

#define INIT_STACK_CAPACITY 100

typedef struct {
	int top;
	int capacity;
	Symbol** functions;
} TypeCheckerSymbolStack;

Symbol* peek_typechecker_symbol_stack();
void pop_func_symbol();
bool is_typecheck_stack_empty();
bool add_func_symbol(CompilerContext* ctx, Symbol* symbol);

bool init_typechecker_symbol_stack(CompilerContext* ctx);
TypeCheckerSymbolStack create_typechecker_symbol_stack(CompilerContext* ctx);

void typecheck_params(Node* params);
struct Type* typecheck_expression(CompilerContext* ctx, Node* expr);
void typecheck_statement(CompilerContext* ctx, Node* stmt);
void typecheck_globals(CompilerContext* ctx, Node* globals);
void typecheck_tree(CompilerContext* ctx, Node* root);
#endif