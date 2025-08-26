#include "compilercontext.h"
#include "symbols.h"

char* keywords[KEYWORDS] = {"function", "let", "int", "char", "bool",
							"void", "struct", "enum", "if",
							"else", "for", "while",
						    "continue", "break", "return",
							"switch", "case", "true", "false", "str"
						   };

CompilerContext* create_compiler_context() {
	CompilerContext* ctx = malloc(sizeof(CompilerContext));
	if (!ctx) {
		perror("\033[31mError\033[0m: unable to allocate space for compiler context\n");
		return NULL;
	}

	ctx->keywords = keywords;
	ctx->lexer_arena = create_arena(LEXER_ARENA);
	if (!ctx->lexer_arena) {
		printf("could not create lexer arena\n");
		free(ctx);
		return NULL;
	}

	ctx->ast_arena = create_arena(AST_ARENA);
	if (!ctx->ast_arena) {
		printf("could not create lexer arena\n");
 		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->type_arena = create_arena(TYPE_ARENA);
	if (!ctx->type_arena) {
		printf("could not create type arena\n");
		free_arena(ctx->ast_arena);
		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->symbol_arena = create_arena(SYMBOL_ARENA);
	if (!ctx->symbol_arena) {
		printf("could not create symbol arena\n");
		free_arena(ctx->type_arena);
		free_arena(ctx->ast_arena);
		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	if (ctx->symbol_arena) {
		ctx->global_table = create_symbol_table(ctx);
		if (!ctx->global_table) {
			printf("could not create global symbol table\n");
			free_arena(ctx->symbol_arena);
			free_arena(ctx->type_arena);
			free_arena(ctx->ast_arena);
			free_arena(ctx->lexer_arena);
			free(ctx);
			return NULL;
		}

		ctx->symbol_stack = create_stack(ctx);
		if (!ctx->symbol_stack) {
			printf("could not create symbol stack\n");
			free_arena(ctx->symbol_arena);
			free_arena(ctx->type_arena);
			free_arena(ctx->ast_arena);
			free_arena(ctx->lexer_arena);
			free(ctx);
			return NULL;
		}
	}

	ctx->ir_arena = create_arena(IR_ARENA);
	if (!ctx->ir_arena) {
		free_arena(ctx->symbol_arena);
		free_arena(ctx->type_arena);
		free_arena(ctx->ast_arena);
		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->error_arena = create_arena(ERROR_ARENA);
	if (!ctx->error_arena) {
		free_arena(ctx->ir_arena);
		free_arena(ctx->symbol_arena);
		free_arena(ctx->type_arena);
		free_arena(ctx->ast_arena);
		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->codegen_arena = create_arena(CODEGEN_ARENA);
	if (!ctx->codegen_arena) {
		free_arena(ctx->error_arena);
		free_arena(ctx->ir_arena);
		free_arena(ctx->symbol_arena);
		free_arena(ctx->type_arena);
		free_arena(ctx->ast_arena);
		free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	return ctx;
}

void free_compiler_context(CompilerContext* ctx) {
	if (!ctx) return;

	free_arena(ctx->lexer_arena);
	free_arena(ctx->ast_arena);
	free_arena(ctx->type_arena);
	free_arena(ctx->symbol_arena);
	free_arena(ctx->ir_arena);
	free_arena(ctx->error_arena);
	free_arena(ctx->codegen_arena);
	free(ctx);
}