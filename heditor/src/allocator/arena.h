#ifndef HEDITOR_ARENA_H
#define HEDITOR_ARENA_H

#include "allocator.h"

#define HED_WITH_ARENA(ARENA) \
	for ( \
		struct { \
			int i; \
			hed_arena_checkpoint_t checkpoint; \
		} itr = { 0, hed_arena_begin(ARENA) }; \
		itr.i < 1; \
		++itr.i, hed_arena_end(ARENA, itr.checkpoint) \
	) \

#define HED_ARENA_ALLOC_TYPE(ARENA, TYPE) \
	HED_ARENA_ALLOC_ARRAY(ARENA, TYPE, 1)

#define HED_ARENA_ALLOC_ARRAY(ARENA, TYPE, SIZE) \
	hed_arena_alloc(ARENA, sizeof(TYPE) * SIZE, _Alignof(TYPE))

typedef struct hed_arena_chunk_s hed_arena_chunk_t;

typedef struct hed_arena_checkpoint_s {
	char* chunk_pos;
	hed_arena_chunk_t* chunk;
} hed_arena_checkpoint_t;

typedef struct hed_arena_s {
	hed_allocator_t* alloc;
	hed_allocator_t impl;
	size_t chunk_size;
	hed_arena_chunk_t* current_chunk;
	hed_arena_chunk_t* free_chunks;
} hed_arena_t;

void
hed_arena_init(hed_arena_t* arena, size_t chunk_size, hed_allocator_t* alloc);

void
hed_arena_cleanup(hed_arena_t* arena);

void*
hed_arena_alloc(hed_arena_t* arena, size_t size, size_t alignment);

hed_arena_checkpoint_t
hed_arena_begin(hed_arena_t* arena);

void
hed_arena_end(hed_arena_t* arena, hed_arena_checkpoint_t checkpoint);

void
hed_arena_reset(hed_arena_t* arena);

hed_allocator_t*
hed_arena_as_allocator(hed_arena_t* arena);

#endif
