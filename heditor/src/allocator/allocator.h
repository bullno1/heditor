#ifndef HEDITOR_ALLOCATOR_H
#define HEDITOR_ALLOCATOR_H

#include <stddef.h>

typedef struct hed_allocator_s {
	void* (*realloc)(void* ptr, size_t size, struct hed_allocator_s* alloc);
} hed_allocator_t;

static inline void*
hed_realloc(void* ptr, size_t size, hed_allocator_t* alloc) {
	return alloc->realloc(ptr, size, alloc);
}

static inline void*
hed_malloc(size_t size, hed_allocator_t* alloc) {
	return hed_realloc(NULL, size, alloc);
}

static inline void
hed_free(void* ptr, hed_allocator_t* alloc) {
	hed_realloc(ptr, 0, alloc);
}

#endif
