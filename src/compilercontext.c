#include "compilercontext.h"

CompilerContext* create_compiler_context() {
	CompilerContext* ctx = malloc(sizeof(CompilerContext));
	if (!ctx) {
		perror("In 'create_compiler_context', unable to allocate space for compiler context.\n");
		return NULL;
	}
	ctx->lexer_arena = create_arena(LEXER_ARENA);
	if (!ctx->lexer_arena) {
		perror("Unable to create lexer arena.\n");
		free(ctx);
		return NULL;
	}

	ctx->ast_arena = create_arena(AST_ARENA);
	if (!ctx->ast_arena) {
		perror("Unable to create ast_arena.\n");
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->type_arena = create_arena(TYPE_ARENA);
	if (!ctx->type_arena) {
		if (ctx->ast_arena) free_arena(ctx->ast_arena);
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->symbol_arena = create_arena(SYMBOL_ARENA);
	if (!ctx->symbol_arena) {
		perror("Unable to create symbol arena.\n");
		if (ctx->type_arena) free_arena(ctx->type_arena);
		if (ctx->ast_arena) free_arena(ctx->ast_arena);
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->ir_arena = create_arena(IR_ARENA);
	if (!ctx->ir_arena) {
		perror("Unable to create IR arena.\n");
		if (ctx->symbol_arena) free_arena(ctx->symbol_arena);
		if (ctx->type_arena) free_arena(ctx->type_arena);
		if (ctx->ast_arena) free_arena(ctx->ast_arena);
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->error_arena = create_arena(ERROR_ARENA);
	if (!ctx->error_arena) {
		perror("Unable to create error arena.\n");
		if (ctx->ir_arena) free_arena(ctx->ir_arena);
		if (ctx->symbol_arena) free_arena(ctx->symbol_arena);
		if (ctx->type_arena) free_arena(ctx->type_arena);
		if (ctx->ast_arena) free_arena(ctx->ast_arena);
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	ctx->codegen_arena = create_arena(CODEGEN_ARENA);
	if (!ctx->codegen_arena) {
		perror("Unable to create codegen arena\n");
		if (ctx->error_arena) free_arena(ctx->error_arena);
		if (ctx->ir_arena) free_arena(ctx->ir_arena);
		if (ctx->symbol_arena) free_arena(ctx->symbol_arena);
		if (ctx->type_arena) free_arena(ctx->type_arena);
		if (ctx->ast_arena) free_arena(ctx->ast_arena);
		if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
		free(ctx);
		return NULL;
	}

	return ctx;
}

void free_compiler_context(CompilerContext* ctx) {
	if (!ctx) return;

	if (ctx->lexer_arena) free_arena(ctx->lexer_arena);
	if (ctx->ast_arena) free_arena(ctx->ast_arena);
	if (ctx->type_arena) free_arena(ctx->type_arena);
	if (ctx->symbol_arena) free_arena(ctx->symbol_arena);
	if (ctx->ir_arena) free_arena(ctx->ir_arena);
	if (ctx->error_arena) free_arena(ctx->error_arena);
	if (ctx->codegen_arena) free_arena(ctx->codegen_arena);
	free(ctx);
}