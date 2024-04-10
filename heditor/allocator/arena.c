#include "arena.h"
#include "../utils.h"
#include "../pico_log.h"
#include <stdint.h>

struct hed_arena_chunk_s {
	hed_arena_chunk_t* next;
	char* pos;
	char* end;
	char start[];
};

HED_PRIVATE intptr_t
hed_align_ptr_forward(intptr_t ptr, size_t alignment) {
	return (((intptr_t)ptr + (intptr_t)alignment - 1) & -(intptr_t)alignment);
}

HED_PRIVATE void*
hed_arena_alloc_from_chunk(hed_arena_chunk_t* chunk, size_t size, size_t alignment) {
	size_t space_available = chunk != NULL ? chunk->end - chunk->start : 0;
	if (space_available < size) { return NULL; }

	intptr_t result = hed_align_ptr_forward((intptr_t)chunk->pos, alignment);
	intptr_t new_pos = result + (ptrdiff_t)size;
	// Out of space due to alignment
	if (new_pos > (intptr_t)chunk->end) { return NULL; }

	chunk->pos = (char*)new_pos;
	return (void*)result;
}

HED_PRIVATE void*
hed_arena_alloc_chunk(hed_arena_t* arena, size_t chunk_size) {
	hed_arena_chunk_t* chunk;
	if (chunk_size > arena->chunk_size) {  // Oversized alloc
		chunk = hed_malloc(chunk_size, arena->alloc);
		chunk->end = chunk->start + chunk_size;
	} else if (arena->free_chunks != NULL) {  // Recycle chunk
		chunk = arena->free_chunks;
		arena->free_chunks = chunk->next;
	} else {  // New chunk
		chunk = hed_malloc(arena->chunk_size, arena->alloc);
		chunk->end = chunk->start + arena->chunk_size;
	}

	chunk->pos = chunk->start;
	return chunk;
}

void
hed_arena_init(hed_arena_t* arena, size_t chunk_size, hed_allocator_t* alloc) {
	*arena = (hed_arena_t){
		.alloc = alloc,
		.chunk_size = chunk_size,
	};
}

void
hed_arena_cleanup(hed_arena_t* arena) {
	hed_arena_reset(arena);

	hed_arena_chunk_t* itr = arena->free_chunks;
	while (itr != NULL) {
		hed_arena_chunk_t* chunk = itr;
		itr = itr->next;

		hed_free(chunk, arena->alloc);
	}
	arena->free_chunks = NULL;
}

void*
hed_arena_alloc(hed_arena_t* arena, size_t size, size_t alignment) {
	hed_arena_chunk_t* current_chunk = arena->current_chunk;

	void* result = hed_arena_alloc_from_chunk(current_chunk, size, alignment);
	if (result != NULL) { return result; }

	size_t chunk_size = HED_MAX(
		arena->chunk_size,
		sizeof(hed_arena_chunk_t) + size
	);
	hed_arena_chunk_t* new_chunk = hed_arena_alloc_chunk(arena, chunk_size);
	new_chunk->next = arena->current_chunk;
	arena->current_chunk = new_chunk;
	return hed_arena_alloc_from_chunk(new_chunk, size, alignment);
}

hed_arena_checkpoint_t
hed_arena_begin(hed_arena_t* arena) {
	return (hed_arena_checkpoint_t){
		.chunk = arena->current_chunk,
		.chunk_pos = arena->current_chunk != NULL
			? arena->current_chunk->pos
			: NULL,
	};
}

void
hed_arena_end(hed_arena_t* arena, hed_arena_checkpoint_t checkpoint) {
	hed_arena_chunk_t* itr = arena->current_chunk;
	while (itr != checkpoint.chunk) {
		hed_arena_chunk_t* chunk = itr;
		itr = itr->next;

		if ((size_t)(chunk->end - chunk->start) > arena->chunk_size) {
			hed_free(chunk, arena->alloc);
		} else {
			chunk->next = arena->free_chunks;
			arena->free_chunks = chunk;
		}
	}

	if (checkpoint.chunk != NULL) {
		checkpoint.chunk->pos = checkpoint.chunk_pos;
	}
	arena->current_chunk = checkpoint.chunk;
}
