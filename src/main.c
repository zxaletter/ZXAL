#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include "IR/dag.h"
#include "IR/tac.h"
// #include "Codegen/codegen.h"
#include "auxiliaries.h"
#include "memallocator.h"
#include <stdlib.h>
#include <stdio.h>

char* make_output_string(char* filename) {
	char* output = NULL;

	size_t filename_length = sizeof(filename);
	for (size_t i = 0; i < filename_length; i++) {
		if (filename[i] == '.') {
			int length = &filename[i] - filename;
			output = malloc(length + 5);
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

	char* filename = argv[1];
	char* output = make_output_string(filename);

	Lexer* lexer = lex(filename);
	print_tokens(lexer->tokens);

	Arena* ast_arena = create_arena(AST_ARENA);
	Node* ast_root = parse(ast_arena, lexer->tokens, lexer->info);
	free_arena(ast_arena);
	// resolve_tree(ast_root);
	// typecheck_tree(ast_root);
	
	// DAGNode* dag_root = build_DAG(ast_root);
	// TACTable* tac_table = build_tacs(dag_root);
	// free_tac_table(tac_table);
	// free_dag(dag_root);
	// free_ast(ast_root);
	free_lexer(lexer);
	free(output);
	return 0;
}