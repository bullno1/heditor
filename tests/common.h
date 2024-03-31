#ifndef HGRAPH_TESTS_COMMON_H
#define HGRAPH_TESTS_COMMON_H

#include <hgraph/runtime.h>
#include <munit.h>

typedef struct {
	hgraph_t* graph;
	hgraph_registry_t* registry;
} fixture_t;

void*
setup_fixture(const MunitParameter params[], void* user_data);

void
tear_down_fixture(void* fixture);

#endif
