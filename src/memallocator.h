#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#define MEM_BLOCK_CAPACITY 8000

typedef enum {
	LEXER_ARENA,
	AST_ARENA,
	SEMANTIC_ARENA,
	IR_ARENA,
	CODEGEN_ARENA,
	ERROR_ARENA
} arena_t;

typedef struct MemoryBlock {
	size_t capacity;
	size_t offset;
	void* p;
	struct MemoryBlock* next;
} MemoryBlock;

typedef struct {
	arena_t type;
	MemoryBlock* head;
	MemoryBlock* current_block;
} Arena;

void* arena_allocate(Arena* A, size_t req_size);
MemoryBlock* create_memory_block();
Arena* create_arena(arena_t type);
void free_arena(Arena* A);
#endif