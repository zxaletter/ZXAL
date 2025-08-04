#include <stdlib.h>
#include <stdio.h>
#include "compilercontext.h" 
#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include "Semantics/nameresolution.h"
#include "Semantics/typechecker.h"
#include "IR/tac.h"
#include "IR/cfg.h"
#include "RegAlloc/regalloc.h"
#include "Codegen/codegen.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Expected only two arguments\n");
		return 1;
	}

	CompilerContext* ctx = create_compiler_context();
	if (!ctx) {
		printf("compiler context is NULL\n");
		return 1;
	} 
	char* file = argv[1];
	Lexer* lexer = lex(ctx, file);
	// print_tokens(lexer->tokens);

	Node* ast_root = parse(ctx, lexer);
	resolve_tree(ctx, ast_root);
	typecheck_tree(ctx, ast_root);
	
	TACTable* tac_table = build_tacs(ctx, ast_root);
	FunctionList* function_list = build_cfg(ctx, tac_table);
	reg_alloc(ctx, function_list);
	codegen(ctx, function_list, file);
	

	free_compiler_context(ctx);
	return 0;
}