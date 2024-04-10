#ifndef HEDITOR_ALLOCATOR_SOKOL_H
#define HEDITOR_ALLOCATOR_SOKOL_H

#include "allocator.h"
#include <sokol_app.h>

static inline void*
hed_sokol_alloc(size_t size, void* user_data) {
	return hed_malloc(size, user_data);
}

static inline void
hed_sokol_free(void* ptr, void* user_data) {
	hed_free(ptr, user_data);
}

static inline sapp_allocator
hed_allocator_to_sokol(hed_allocator_t* alloc) {
	return (sapp_allocator){
		.alloc_fn = hed_sokol_alloc,
		.free_fn = hed_sokol_free,
		.user_data = alloc,
	};
}

#endif
