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
