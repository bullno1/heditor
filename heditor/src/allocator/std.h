#ifndef HEDITOR_STD_ALLOCATOR_H
#define HEDITOR_STD_ALLOCATOR_H

#include "allocator.h"
#include <stdlib.h>

static void*
hed_std_realloc(void* ptr, size_t size, hed_allocator_t* ctx) {
	(void)ctx;

	if (size == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, size);
	}
}

static inline hed_allocator_t
hed_std_allocator(void) {
	return (hed_allocator_t) {
		.realloc = hed_std_realloc,
	};
}

#endif
