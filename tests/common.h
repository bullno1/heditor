#ifndef HGRAPH_TESTS_COMMON_H
#define HGRAPH_TESTS_COMMON_H

#include <hgraph/runtime.h>

typedef struct {
	hgraph_t* graph;
	hgraph_registry_t* registry;
} fixture_t;

void
fixture_init(fixture_t* fixture);

void
fixture_cleanup(fixture_t* fixture);

#endif
