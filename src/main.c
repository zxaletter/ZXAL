#include "Lexer/lexer.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Expected two arguments\n");
		return 1;
	}

	char* name = argv[1];
	char* contents = get_file_contents(name);
	printf("\n\nHere is the file Contents: \n'%s'\n", contents);
	printf(" i got here\n");
	Token* tokens = lex(contents);
	if (!tokens) return NULL;
	printf("Now im here\n");
	print_tokens(tokens);

	free_tokens(tokens);
	
	return 0;
}