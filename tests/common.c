#include "common.h"
#include "plugin1.h"
#include "plugin2.h"
#include <stdlib.h>

static inline intptr_t
align_ptr(intptr_t ptr, size_t alignment) {
	return (((intptr_t)ptr + (intptr_t)alignment - 1) & -(intptr_t)alignment);
}

void
fixture_init(fixture_t* fixture) {
	fixed_arena arena;
	arena_init(&arena, 4096 * 1024);

	hgraph_registry_config_t reg_config = {
		.max_data_types = 32,
		.max_node_types = 32,
	};
	size_t mem_required = hgraph_registry_builder_init(NULL, &reg_config);
	hgraph_registry_builder_t* builder = arena_alloc(&arena, mem_required);
	hgraph_registry_builder_init(builder, &reg_config);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);
	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	mem_required = hgraph_registry_init(NULL, builder);
	hgraph_registry_t* registry = arena_alloc(&arena, mem_required);
	hgraph_registry_init(registry, builder);

	hgraph_config_t graph_config = {
		.registry = registry,
		.max_nodes = 32,
		.max_name_length = 64,
	};
	mem_required = hgraph_init(NULL, &graph_config);
	hgraph_t* graph = arena_alloc(&arena, mem_required);
	hgraph_init(graph, &graph_config);

	*fixture = (fixture_t){
		.arena = arena,
		.graph = graph,
		.registry = registry,
	};
}

void
fixture_cleanup(fixture_t* fixture) {
	hgraph_cleanup(fixture->graph);
	arena_cleanup(&fixture->arena);
}

void
create_start_mid_end_graph(hgraph_t* graph) {
	// Create the following:
	// |start| -> |mid| -> |end|
	hgraph_index_t start = hgraph_create_node(
		graph, &plugin1_start
	);
	hgraph_set_node_name(graph, start, HGRAPH_STR("start"));

	hgraph_index_t mid = hgraph_create_node(
		graph, &plugin2_mid
	);
	hgraph_set_node_name(graph, mid, HGRAPH_STR("mid"));

	hgraph_index_t end = hgraph_create_node(
		graph, &plugin1_end
	);
	hgraph_set_node_name(graph, end, HGRAPH_STR("end"));

	hgraph_connect(
		graph,
		hgraph_get_pin_id(graph, start, &plugin1_start_out_f32),
		hgraph_get_pin_id(graph, mid, &plugin2_mid_in_f32)
	);
	hgraph_connect(
		graph,
		hgraph_get_pin_id(graph, mid, &plugin2_mid_out_i32),
		hgraph_get_pin_id(graph, end, &plugin1_end_in_i32)
	);
}

void
arena_init(fixed_arena* arena, size_t size) {
	arena->start = arena->current = malloc(size);
	arena->end = arena->current + size;
}

void
arena_cleanup(fixed_arena* arena) {
	free(arena->start);
}

void*
arena_alloc(fixed_arena* arena, size_t size) {
	char* result = (char*)align_ptr((intptr_t)arena->current, _Alignof(max_align_t));
	arena->current = result + size;
	return arena->current <= arena->end ? result : NULL;
}
