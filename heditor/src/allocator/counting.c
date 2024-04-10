#include "counting.h"
#include "../utils.h"
#include "allocator.h"

typedef struct hed_counting_mem_chunk_s {
	size_t size;
	_Alignas(max_align_t) char mem[];
} hed_counting_mem_chunk_t;

HED_PRIVATE void*
hed_counting_allocator_realloc(void* ptr, size_t new_size, hed_allocator_t* ctx) {
	hed_counting_allocator_t* alloc = HED_CONTAINER_OF(ctx, hed_counting_allocator_t, impl);
	hed_counting_mem_chunk_t* chunk = ptr == NULL
		? NULL
		: HED_CONTAINER_OF(ptr, hed_counting_mem_chunk_t, mem);
	size_t chunk_size = chunk == NULL ? 0 : chunk->size;

	if (new_size == 0) {  // Free
		alloc->current_allocated -= chunk_size;
		hed_free(chunk, alloc->backing);
		return NULL;
	} else if (new_size > chunk_size) {  // Extend
		alloc->current_allocated += new_size - chunk_size;
		alloc->peak_allocated = HED_MAX(alloc->current_allocated, alloc->peak_allocated);

		hed_counting_mem_chunk_t* new_chunk = hed_realloc(
			chunk,
			sizeof(hed_counting_mem_chunk_t) + new_size,
			alloc->backing
		);
		new_chunk->size = new_size;

		return new_chunk->mem;
	} else {  // Shrink
		return ptr;
	}
}

hed_allocator_t*
hed_counting_allocator_init(
	hed_counting_allocator_t* alloc,
	hed_allocator_t* backing
) {
	*alloc = (hed_counting_allocator_t){
		.backing = backing,
		.impl.realloc = hed_counting_allocator_realloc,
	};

	return &alloc->impl;
}
