#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include "Symbols/symbols.h"
#include "Symbols/types.h"
#include "auxiliaries.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Expected two arguments\n");
		return 1;
	}

	char* filename = argv[1];

	size_t filename_length = sizeof(filename);
	char* start = filename;
	char* output = NULL;
	for (size_t i = 0; i < filename_length; i++) {
		if (filename[i] == '.') {
			int length = &filename[i] - start;
			output = malloc(length + 4);
			if (!output) {
				perror("Error: unable to allocate space for compact\n");
				return 1;
			}
			strncpy(output, filename, length);
			strcat(output, ".asm");
			break;
		}
	}


	Lexer* lexer = lex(filename);
	if (lexer) {
		print_tokens(lexer->tokens);
	}
	Node* root = parse(lexer->tokens, lexer->info);
	if (root) {
		// print_ast(root);
		resolve_tree(root);
		typecheck_tree(root);

	}

	free_stacks();
	free_ast(root);
	free_lexer(lexer);
	free(output);
	return 0;
}