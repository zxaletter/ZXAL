#include "bumpallocator.h"
#include <stdio.h>

Arena* create_arena(arena_t type) {
	Arena* A = malloc(sizeof(Arena));
	if (!A) {
		perror("In 'create_arena', unable to allocate space for Arena.\n");
		return NULL;
	}

	A->type = type;
	A->head = create_memory_block(MEM_BLOCK_CAPACITY);
	if (!A->head) {
		perror("In 'create_arena', received NULL mem block from 'create_memory_block'.\n");
		free(A);
		return NULL;
	} 
	A->current_block = A->head;
	return A;
}

MemoryBlock* create_memory_block(size_t req_size) {
	MemoryBlock* mem_block = malloc(sizeof(MemoryBlock));
	if (!mem_block) {
		perror("In 'create_memory_block', unable to allocate space for memory block.\n");
		return NULL;
	}

	mem_block->capacity = req_size;
	mem_block->offset = 0;
	mem_block->p = malloc(mem_block->capacity);
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

	// printf("=== arena_allocate DEBUG ===\n");
    // printf("Arena: %p\n", (void*)A);
    // printf("Current block: %p\n", (void*)A->current_block);
    // printf("Current block->p: %p\n", A->current_block->p);
    // printf("Current block->offset: %zu\n", A->current_block->offset);
    // printf("Current block->capacity: %zu\n", A->current_block->capacity);
    // printf("Requested size: %zu\n", req_size);
    		
	uintptr_t current_address = (uintptr_t)A->current_block->p + A->current_block->offset;
	// printf("Current address: 0x%lx\n", current_address);
	
	size_t padding = (BYTE_ALIGNMENT - (current_address % BYTE_ALIGNMENT)) % BYTE_ALIGNMENT; 
	// printf("Padding: %zu\n", padding);

	size_t total_needed = req_size + padding;
	// printf("Total needed: %zu\n", total_needed);

	if (A->current_block->offset + total_needed > A->current_block->capacity) {
		// printf("Need new block\n");
		size_t mem_block_size = total_needed > MEM_BLOCK_CAPACITY ? total_needed : MEM_BLOCK_CAPACITY;
		// printf("New block size: %zu\n", mem_block_size);
		
		MemoryBlock* new_mem_block = create_memory_block(mem_block_size);
		if (!new_mem_block) {
			perror("In 'arena_allocate', received NULL mem block from 'create_memory_block'.\n");
			return NULL;
		}

		// printf("New block created: %p\n", (void*)new_mem_block);
        // printf("New block->p: %p\n", new_mem_block->p);
		
		A->current_block->next = new_mem_block;
		A->current_block = new_mem_block;		
		
		current_address = (uintptr_t)A->current_block->p;
		padding = (BYTE_ALIGNMENT - (current_address % BYTE_ALIGNMENT)) % BYTE_ALIGNMENT; 

		// printf("After new block - current_address: 0x%lx\n", current_address);
        // printf("After new block - padding: %zu\n", padding);
	}

	A->current_block->offset += padding;
	// printf("Offset after adding padding: %zu\n", A->current_block->offset);

	void* new_ptr = (char*)A->current_block->p + A->current_block->offset;
	// printf("Calculated new_ptr: %p\n", new_ptr);

	// printf("Testing write to new_ptr...\n");
    // *((char*)new_ptr) = 0;
    // printf("Write test passed\n");

	A->current_block->offset += req_size;
	// printf("Final offset: %zu\n", A->current_block->offset);

	memset(new_ptr, 0, req_size);
	// printf("memset completed\n");
    
    // printf("=== arena_allocate returning %p ===\n", new_ptr);
	return new_ptr;
}

// - - - - - - - -

void* arena_reallocate(Arena* A, void* prev_ptr, size_t bytes_to_copy, size_t req_size) {
	if (!A || !prev_ptr || req_size == 0) return NULL;
	void* new_ptr = arena_allocate(A, req_size);
	if (!new_ptr) return NULL;
	memcpy(new_ptr, prev_ptr, bytes_to_copy);

	if (req_size > bytes_to_copy) {
		void* remaining_space = (char*)new_ptr + bytes_to_copy;
		memset(remaining_space, 0, req_size - bytes_to_copy);
	}
	return new_ptr;
}

void free_arena(Arena* A) {
	if (!A) return;
	MemoryBlock* block = A->head;
	while (block) {
		MemoryBlock* next_block = block->next;
		if (block->p) {
			free(block->p);
		}
		free(block);

		block = next_block;
	}
	free(A);
}