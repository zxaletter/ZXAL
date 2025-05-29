#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Expected two arguments\n");
		return 1;
	}

	FILE* file = fopen(argv[1], "r");
	Token* tokens = lex(file);
	print_tokens(tokens);

	Node* root = parse(tokens, file);
	if (root) {
		printf("Got nodes\n");
	}

	free_ast(root);
	printf("FREED ROOT\n");
	free_tokens(tokens);
	fclose(file);
	
	return 0;
}