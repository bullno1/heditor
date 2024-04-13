#ifndef HEDITOR_COUNTING_ALLOCATOR_H
#define HEDITOR_COUNTING_ALLOCATOR_H

#include "allocator.h"

typedef struct hed_counting_allocator_s {
	hed_allocator_t impl;
	hed_allocator_t* backing;

	size_t current_allocated;
	size_t peak_allocated;
} hed_counting_allocator_t;

hed_allocator_t*
hed_counting_allocator_init(
	hed_counting_allocator_t* alloc,
	hed_allocator_t* backing
);

#endif
