#ifndef TYPES_H
#define TYPES_H

#include "compilercontext.h"
#include <stdlib.h>
#include <stdbool.h>

typedef enum TypeKind {
	TYPE_INTEGER,
	TYPE_CHAR,
	TYPE_BOOL,
	TYPE_VOID,
	TYPE_STRING,
	TYPE_ARRAY,
	TYPE_FUNCTION,
	TYPE_STRUCT,
	TYPE_ENUM,
	TYPE_UNKNOWN
} TypeKind;

typedef struct Type {
	TypeKind kind;
	struct Type* subtype;
} Type;

bool type_equals(struct Type* a, struct Type* b);
struct Type* type_create(CompilerContext* ctx, TypeKind main_type, struct Type* subtype);
#endif 