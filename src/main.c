#include "Lexer/lexer.h"


int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Expected two arguments\n");
		return 1;
	}

	char* name = argv[1];
	char* contents = get_file_contents(name);
	printf("\n\nHere is the file Contents: \n'%s'\n", contents);

	Tokens* tokens = lex(contents);

	return 0;
}