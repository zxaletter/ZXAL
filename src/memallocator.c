#include "memallocator.h"

Arena* create_arena(arena_t type) {
	Arena* A = malloc(sizeof(Arena));
	if (!A) {
		perror("In 'create_arena', unable to allocate space for Arena.\n");
		return NULL;
	}

	A->type = type;
	A->head = create_memory_block();
	if (!A->head) {
		perror("In 'create_arena', received NULL mem block from 'create_memory_block'.\n");
		free(A);
		return NULL;
	} 
	A->current_block = head;
	return A;
}

MemoryBlock* create_memory_block() {
	MemoryBlock* mem_block = malloc(sizeof(MemoryBlock));
	if (!mem_block) {
		perror("In 'create_memory_block', unable to allocate space for memory block.\n");
		return NULL;
	}

	mem_block->capacity = MEM_BLOCK_CAPACITY;
	mem_block->offset = 0;
	mem_block->p = malloc(mem_block_size);
	if (!mem_block->p) {
		perror("In 'create_memory_block', unable to allocate space for 'mem_block->p'.\n");
		free(mem_block);
		return NULL;
	}
	mem_block->next = NULL;
	return mem_block;
}

void* arena_allocate(Arena* A, size_t req_size) {
	if (!A || req_size == 0) return NULL;

	size_t mem_diff = A->current_block->size + req_size - A->current_block->capacity; 
	if (mem_diff > 0) {
		MemoryBlock* new_mem_block = create_memory_block(MEM_BLOCK_CAPACITY);
		if (!new_mem_block) {
			perror("In 'allocate_memory', received NULL mem block from 'create_memory_block'.\n");
			return NULL;
		}
		A->current_block->next = new_mem_block;
		A->current_block = new_mem_block;		
	}

	A->current_block->offset += req_size;
	void* new_ptr = A->current_block->p + A->current_block->offset;
	return new_ptr;
}

void free_memory_block(MemoryBlock* block) {
	if (!block) return;
	if (block->p) free(block->p);
	free(block);
}

void free_arena(Arena* A) {
	if (!A) return;
	MemoryBlock* block = A->head;
	while (block) {
		free_memory_block(block);
		block = block->next;
	}
	free(A);
}