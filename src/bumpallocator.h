#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEM_BLOCK_CAPACITY 8000
#define INIT_NUM_MEM_BLOCKS 30
#define BYTE_ALIGNMENT 8

typedef enum {
	LEXER_ARENA,
	AST_ARENA,
	TYPE_ARENA,
	SYMBOL_ARENA,
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

void* arena_reallocate(Arena* A, void* prev_ptr, size_t prev_size, size_t req_size);
void* arena_allocate(Arena* A, size_t req_size);
MemoryBlock* create_memory_block(size_t req_size);
Arena* create_arena(arena_t type);
void free_arena(Arena* A);
#endif