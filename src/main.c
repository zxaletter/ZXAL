#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include "IR/dag.h"
#include "IR/tac.h"
#include "IR/cfg.h"
// #include "RegAlloc/regalloc.h"
// #include "Codegen/codegen.h"
#include "compilercontext.h" 
#include "auxiliaries.h"
#include <stdlib.h>
#include <stdio.h>

char* make_output_string(CompilerContext* ctx, char* filename) {
	char* output = NULL;

	size_t filename_length = strlen(filename);
	for (size_t i = 0; i < filename_length; i++) {
		if (filename[i] == '.') {
			int length = &filename[i] - filename;
			// output = malloc(length + 5);
			output = arena_allocate(ctx->lexer_arena, length + 5);
			if (!output) {
				perror("Unable to allocate space for output string.\n");
				return NULL;
			}
			strncpy(output, filename, length);
			output[length] = '\0';
			strcat(output, ".asm");
		}
	}
	return output;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Expected only two arguments\n");
		return 1;
	}

	CompilerContext* ctx = create_compiler_context();
	if (!ctx) {
		return 1;
	} else {
		printf("We have context\n");
	}

	char* filename = argv[1];

	printf("now here\n");
	Lexer* lexer = lex(ctx, filename);
	printf("im here\n");
	print_tokens(lexer->tokens);

	Node* ast_root = parse(ctx, lexer);
	resolve_tree(ctx, ast_root);
	typecheck_tree(ctx, ast_root);
	
	DAGNode* dag_root = build_DAG(ctx, ast_root);
	TACTable* tac_table = build_tacs(ctx, dag_root);
	FunctionList* function_list = build_cfg(ctx, tac_table);
	// reg_alloc(ctx, function_list);
	

	// char* output = make_output_string(ctx, filename);
	free_compiler_context(ctx);
	return 0;
}