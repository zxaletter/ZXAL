#include "lexer.h"

char* get_file_contents(char* name) {
	if (!name) return NULL;

	FILE* file = fopen(name, "r");
	if (!file) {
		perror("Error in opening file\n");
		return NULL;
	}

	fpos_t position;
	fgetpos(file, &position);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	printf("Size of %s is: %ld\n", name, size);

	fsetpos(file, &position);
	char* buffer = malloc(size + 1);
	if (!buffer) {
		perror("Error: Unable to allocate buffer for file contents\n");
		return NULL;
	}

	for (long i = 0; i < size; i++) {
		char c = fgetc(file);
		printf("%c", c);
		buffer[i] = c;
		if (feof(file)) { break; }
	}

	buffer[size] = '\0';

	fclose(file);
	return buffer;
}

Lexer* initialize_lexer(char* src) {
	struct Lexer* lexer = malloc(sizeof(struct Lexer));
	if (!lexer) {
		fprintf(stderr, "Error: Failed to allocate space for lexer\n");
		return NULL;
	}

	lexer->start = src;
	lexer->end = src;
	lexer->line = 1;
	lexer->column = 1;

	return lexer;
}

Token* lex(char* contents) {
	Lexer* lexer = initialize_lexer(contents);

	
	while ()
}

