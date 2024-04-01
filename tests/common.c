#include "common.h"
#include "plugin1.h"
#include "plugin2.h"
#include <stdlib.h>

void
fixture_init(fixture_t* fixture) {
	hgraph_registry_config_t reg_config = {
		.max_data_types = 32,
		.max_node_types = 32,
	};
	size_t mem_required = hgraph_registry_builder_init(NULL, &reg_config);
	hgraph_registry_builder_t* builder = malloc(mem_required);
	hgraph_registry_builder_init(builder, &reg_config);

	hgraph_plugin_api_t* plugin_api = hgraph_registry_builder_as_plugin_api(builder);
	plugin1_entry(plugin_api);
	plugin2_entry(plugin_api);

	mem_required = hgraph_registry_init(NULL, builder);
	hgraph_registry_t* registry = malloc(mem_required);
	hgraph_registry_init(registry, builder);
	free(builder);

	hgraph_config_t graph_config = {
		.registry = registry,
		.max_nodes = 32,
		.max_name_length = 64,
	};
	mem_required = hgraph_init(NULL, &graph_config);
	hgraph_t* graph = malloc(mem_required);
	hgraph_init(graph, &graph_config);

	*fixture = (fixture_t){
		.graph = graph,
		.registry = registry,
	};
}

void
fixture_cleanup(fixture_t* fixture) {
	free(fixture->graph);
	free(fixture->registry);
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
