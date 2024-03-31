#include "plugin1.h"
#include "plugin2.h"
#include "rktest.h"
#include <hgraph/runtime.h>
#include <stdlib.h>

typedef struct {
	hgraph_index_t num_items;
	bool seen_start;
	bool seen_mid;
	bool seen_end;
} iterator_state;

static bool
iterate_registry(const hgraph_node_type_t* node, void* userdata) {
	iterator_state* state = userdata;

	if (node == &plugin1_start) {
		state->seen_start = true;
	} else if (node == &plugin1_end) {
		state->seen_end = true;
	} else if (node == &plugin2_mid) {
		state->seen_mid = true;
	} else {
		FAIL("error: Unexpected type");
		return false;
	}

	++state->num_items;
	return true;
}

TEST(registry, reg) {
	hgraph_registry_config_t config = {
		.max_data_types = 32,
		.max_node_types = 32,
	};
	size_t mem_required = 0;
	hgraph_registry_builder_t* builder = hgraph_registry_builder_init(
		&config,
		NULL,
		&mem_required
	);
	ASSERT_TRUE(builder == NULL);

	void* builder_mem = malloc(mem_required);
	builder = hgraph_registry_builder_init(
		&config,
		builder_mem,
		&mem_required
	);
	ASSERT_TRUE(builder != NULL);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);
	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	mem_required = 0;
	hgraph_registry_t* registry = hgraph_registry_init(builder, NULL, &mem_required);
	ASSERT_TRUE(registry == NULL);

	void* registry_mem = malloc(mem_required);
	registry = hgraph_registry_init(builder, registry_mem, &mem_required);

	iterator_state i = { 0 };
	hgraph_registry_iterate(
		registry,
		iterate_registry,
		&i
	);
	ASSERT_EQ(i.num_items, 3);
	ASSERT_TRUE(i.seen_start);
	ASSERT_TRUE(i.seen_mid);
	ASSERT_TRUE(i.seen_end);

	free(registry_mem);
	free(builder_mem);
}
