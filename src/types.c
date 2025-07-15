#include "types.h"

bool type_equals(struct Type* a, struct Type* b) {
	if (!a || !b) return false;

	if (a->kind != b->kind) return false;

	switch (a->kind) {
		case TYPE_BOOL:
		case TYPE_CHAR:
		case TYPE_INTEGER:
			return true;

		case TYPE_FUNCTION:
		case TYPE_ARRAY: {
			return type_equals(a->subtype, b->subtype);
		}
	}
}

struct Type* type_create(CompilerContext* ctx, TypeKind kind, struct Type* subtype) {
	struct Type* type = arena_allocate(ctx->type_arena, sizeof(struct Type));
	if (!type) {
		perror("Unable to allocate space for type\n");
		return NULL;
	}

	type->kind = kind;
	type->subtype = subtype;
	return type;
}