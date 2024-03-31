#ifndef HGRAPH_TESTS_COMMON_H
#define HGRAPH_TESTS_COMMON_H

#include <hgraph/runtime.h>

typedef struct {
	hgraph_t* graph;
	hgraph_registry_t* registry;
} fixture_t;

fixture_t*
create_fixture(void);

void
destroy_fixture(fixture_t* fixture);

#endif
