#ifndef HGRAPH_TESTS_COMMON_H
#define HGRAPH_TESTS_COMMON_H

#include <hgraph/runtime.h>

typedef struct {
	char* start;
	char* end;
	char* current;
} fixed_arena;

typedef struct {
	hgraph_t* graph;
	hgraph_registry_t* registry;
	fixed_arena arena;
} fixture_t;

void
fixture_init(fixture_t* fixture);

void
fixture_cleanup(fixture_t* fixture);

void
create_start_mid_end_graph(hgraph_t* graph);

void
arena_init(fixed_arena* arena, size_t size);

void
arena_cleanup(fixed_arena* arena);

void*
arena_alloc(fixed_arena* arena, size_t size);

#endif
