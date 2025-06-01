#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Expected two arguments\n");
		return 1;
	}

	printf("here in main\n");
	char* filename = argv[1];
	printf("filename\n");
	Lexer* lexer = lex(filename);
	if (lexer) {
		print_tokens(lexer->tokens);
	}

	Node* root = parse(lexer->tokens, lexer->info);
	// if (root) {
	// 	print_ast(root);
	// }

	// free_ast(root);
	free_lexer(lexer);
	
	return 0;
}